/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file company_colour.cpp Company RGB colour parsing, shading and sprite recolouring. */

#include "stdafx.h"
#include "company_colour.h"
#include "company_base.h"
#include "table/sprites.h"
#include <deque>
#include <map>
#include <mutex>

std::optional<uint32_t> ParseCompanyRGBColour(std::string_view text)
{
	if (text.starts_with('#')) text.remove_prefix(1);
	if (text.size() != 6) return std::nullopt;
	uint32_t value = 0;
	for (char ch : text) {
		uint digit;
		if (ch >= '0' && ch <= '9') digit = ch - '0';
		else if (ch >= 'a' && ch <= 'f') digit = ch - 'a' + 10;
		else if (ch >= 'A' && ch <= 'F') digit = ch - 'A' + 10;
		else return std::nullopt;
		value = (value << 4) | digit;
	}
	return value;
}

Colour GetCompanyRGBShade(uint32_t rgb, Shade shade)
{
	/* Normal is the exact entered colour; preserve detail in darker and lighter sprite pixels. */
	static constexpr int shade_levels[] = {40, 64, 96, 128, 160, 192, 224, 255};
	auto channel = [&](uint value) -> uint8_t {
		int level = shade_levels[to_underlying(shade)];
		return level <= 128 ? value * level / 128 : value + (255 - value) * (level - 128) / 127;
	};
	return Colour(channel((rgb >> 16) & 255), channel((rgb >> 8) & 255), channel(rgb & 255));
}

/* IDs are stable for the lifetime of the process, including viewport worker draws.
 * Intern company/base-palette pairs rather than RGB values, so repeated edits reuse IDs. */
struct CompanyRGBPalette {
	PaletteID base;
	CompanyID company;
	bool primary;
	bool secondary;
	bool repair_unused;
};
static std::mutex _company_rgb_mutex;
static std::deque<CompanyRGBPalette> _company_rgb_palettes;
static std::map<uint64_t, PaletteID> _company_rgb_ids;

static CompanyRGBPalette LookupCompanyRGBPalette(PaletteID palette)
{
	std::lock_guard lock(_company_rgb_mutex);
	return _company_rgb_palettes.at(palette & PALETTE_MASK);
}

PaletteID GetCompanyRGBPalette(PaletteID palette, CompanyID company, bool primary, bool secondary, bool repair_unused)
{
	const Company *c = Company::GetIfValid(company);
	if (palette == PAL_NONE) return palette;
	primary &= c != nullptr && c->colour_rgb != COMPANY_RGB_DEFAULT;
	secondary &= c != nullptr && c->colour2_rgb != COMPANY_RGB_DEFAULT;
	if (!primary && !secondary && !repair_unused) return palette;
	uint64_t key = uint64_t(palette) | (uint64_t(company.base()) << 32) | (uint64_t(primary) << 40) | (uint64_t(secondary) << 41) | (uint64_t(repair_unused) << 42);
	std::lock_guard lock(_company_rgb_mutex);
	auto [it, added] = _company_rgb_ids.try_emplace(key, static_cast<PaletteID>(_company_rgb_palettes.size()));
	if (added) _company_rgb_palettes.push_back({palette, company, primary, secondary, repair_unused});
	return COMPANY_RGB_PALETTE | it->second;
}

PaletteID GetCompanyRGBBasePalette(PaletteID palette)
{
	if ((palette & COMPANY_RGB_PALETTE) == 0) return palette;
	return LookupCompanyRGBPalette(palette).base | (palette & ~PALETTE_MASK & ~COMPANY_RGB_PALETTE);
}

PixelColour GetCompanyPixelColour(CompanyID company, Shade shade)
{
	const Company *c = Company::GetIfValid(company);
	if (c == nullptr) return GetColourGradient(Colours::Grey, shade);
	PixelColour pixel = GetColourGradient(c->colour, shade);
	if (c->colour_rgb != COMPANY_RGB_DEFAULT) pixel.rgb = GetCompanyRGBShade(c->colour_rgb, shade).data;
	return pixel;
}

bool BuildCompanyRGBRemap(PaletteID palette, const uint8_t *base, std::array<uint8_t, 256> &fallback, std::array<Colour, 256> &rgb)
{
	if ((palette & COMPANY_RGB_PALETTE) == 0 || base == nullptr) return false;
	auto entry = LookupCompanyRGBPalette(palette);
	const Company *c = Company::GetIfValid(entry.company);
	if (c == nullptr && !entry.repair_unused) return false;
	std::copy_n(base, 256, fallback.begin());
	rgb.fill(Colour(0, 0, 0, 0));
	auto set_ramp = [&](uint start, uint32_t colour) {
		if (colour == COMPANY_RGB_DEFAULT) return;
		for (uint i = 0; i < 8; ++i) {
			Colour shade = GetCompanyRGBShade(colour, static_cast<Shade>(i));
			rgb[start + i] = shade;
			/* Never use index 0 (transparent) or animated water/fire colours. */
			if (fallback[start + i] == 0 || fallback[start + i] >= PALETTE_ANIM_START) fallback[start + i] = 1;
		}
	};
	if (entry.primary && c != nullptr) set_ramp(0xC6, c->colour_rgb);
	if (entry.secondary && c != nullptr) set_ramp(0x50, c->colour2_rgb);
	if (entry.repair_unused) {
		/* Guard vehicle draws against unused D7..E2 magenta placeholders left in the remap.
		 * The valid pink company-colour ramp and deliberate NewGRF recolouring are preserved. */
		uint8_t replacement = base[0xC9];
		if (replacement == 0 || (replacement >= 0xD7 && replacement <= 0xE2) || replacement >= PALETTE_ANIM_START) replacement = 1;
		for (uint i = 1; i < 256; ++i) {
			if (fallback[i] < 0xD7 || fallback[i] > 0xE2 || rgb[i].a != 0) continue;
			fallback[i] = replacement;
			if (entry.primary && c != nullptr) rgb[i] = GetCompanyRGBShade(c->colour_rgb, Shade::Normal);
		}
	}
	return true;
}

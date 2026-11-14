/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file company_colour.h Company colours with optional 24-bit RGB overrides. */
#ifndef COMPANY_COLOUR_H
#define COMPANY_COLOUR_H

#include "company_type.h"
#include "palette_func.h"
#include <optional>
#include <string_view>

static constexpr uint32_t COMPANY_RGB_DEFAULT = UINT32_MAX;
static constexpr uint32_t COMPANY_RGB_PALETTE = 1U << 29;

std::optional<uint32_t> ParseCompanyRGBColour(std::string_view text);
Colour GetCompanyRGBShade(uint32_t rgb, Shade shade);
PixelColour GetCompanyPixelColour(CompanyID company, Shade shade);
PaletteID GetCompanyRGBPalette(PaletteID palette, CompanyID company, bool primary = true, bool secondary = false);
PaletteID GetCompanyRGBBasePalette(PaletteID palette);
/** Build optional RGB entries and the corresponding non-animated 8bpp fallback. */
bool BuildCompanyRGBRemap(PaletteID palette, const uint8_t *base, std::array<uint8_t, 256> &fallback, std::array<Colour, 256> &rgb);

#endif

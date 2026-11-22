/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 */

/** @file company_240.cpp Regression coverage for company IDs beyond the original packed fields. */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../company_type.h"
#include "../road_map.h"
#include "../bridge_map.h"
#include "../tunnel_map.h"
#include "../station_map.h"
#include "../water_map.h"
#include "../cargomonitor.h"
#include "../vehiclelist.h"
#include "../core/serialisation.hpp"
#include "../window_gui.h"
#include "../core/backup_type.hpp"
#include "../framerate_type.h"
#include "../company_base.h"
#include "../company_colour.h"
#include "../sprite.h"
#include "../company_cmd.h"
#include "../company_gui.h"
#include "../infrastructure_func.h"
#include "../train.h"
#include "../roadveh.h"
#include "../station_base.h"
#include "../blitter/32bpp_simple.hpp"
#include "../blitter/32bpp_anim.hpp"
#include "../video/video_driver_base.hpp"
#include "../scope.h"
#include "../gfx_layout.h"
#include "../strings_func.h"
#include "mock_environment.h"
#include <set>

/** A real animation buffer without a window or GPU, for the 40bpp drawing paths. */
class CompanyRGBVideoDriver final : public VideoDriverBase {
	std::array<uint8_t, 64 * 64> animation{};
public:
	const char *Start(const StringList &) override { this->anim_buffer = this->animation.data(); return nullptr; }
	void Stop() override {}
	void MakeDirty(int, int, int, int) override {}
	void MainLoop() override {}
	bool ChangeResolution(int, int) override { return true; }
	bool ToggleFullscreen(bool) override { return true; }
	bool HasAnimBuffer() override { return true; }
	const char *GetName() const override { return "company-rgb-test"; }
};

class CompanyRGBVideoFactory final : public DriverFactoryBase {
public:
	CompanyRGBVideoFactory() : DriverFactoryBase(Driver::Type::Video, 0, "company-rgb-test", "Company colour rendering tests") {}
	~CompanyRGBVideoFactory() override = default;
	std::unique_ptr<Driver> CreateInstance() const override { return std::make_unique<CompanyRGBVideoDriver>(); }
};

TEST_CASE("240 companies - masks and network words", "[company240]")
{
	CompanyMask mask;
	for (uint id = 0; id < MAX_COMPANIES; ++id) {
		CompanyID company(id);
		CHECK_FALSE(mask.Test(company));
		mask.Set(company);
		CHECK(mask.Count() == id + 1);
	}
	CHECK(mask.All());
	CHECK_FALSE(mask.Test(COMPANY_SPECTATOR));
	uint expected = 0;
	for (CompanyID company : mask.IterateSetBits()) CHECK(company.base() == expected++);
	CHECK(expected == MAX_COMPANIES);
	for (uint id = 0; id < MAX_COMPANIES; id += 2) mask.Reset(CompanyID(id));
	CHECK(mask.Count() == 120);
	CHECK((mask | ~mask).All());
	CHECK((mask & ~mask).None());
	std::vector<uint8_t> packet;
	BufferSerialisationRef writer(packet);
	for (auto word : mask.bits) writer.Send_uint16(word);
	CHECK(packet.size() == 30);
	DeserialisationBuffer reader(packet.data(), packet.size());
	CompanyMask received;
	for (auto &word : received.bits) word = reader.Recv_uint16();
	CHECK(received == mask);
	CHECK_FALSE(reader.error);
}

TEST_CASE("240 companies - independent tile road and tram owners", "[company240]")
{
	AllocateMap(64, 64);
	ResetRoadTypes();
	TileIndex tile{0};
	MakeRoadBridgeRamp(tile, CompanyID(239), CompanyID(128), CompanyID(63), BridgeType{0}, DiagDirection::NE, ROADTYPE_ROAD, ROADTYPE_TRAM);
	const uint16_t m8 = _me[tile].m8;
	const uint8_t m6 = _me[tile].m6;
	SB(_m[tile].m1, 5, 3, 5);
	SB(_me[tile].m7, 5, 3, 3);
	for (uint id = 0; id < OWNER_END.base(); ++id) {
		Owner owner(id);
		SetTileOwner(tile, owner);
		CHECK(GetTileOwner(tile) == owner);
		CHECK(GB(_m[tile].m1, 5, 3) == 5);
		CHECK(GetRoadOwner(tile, RoadTramType::Road) == CompanyID(128));
		CHECK(GetRoadOwner(tile, RoadTramType::Tram) == CompanyID(63));
		SetRoadOwner(tile, RoadTramType::Road, owner);
		SetRoadOwner(tile, RoadTramType::Tram, owner);
		CHECK(GetRoadOwner(tile, RoadTramType::Road) == owner);
		CHECK(GB(_me[tile].m7, 5, 3) == 3);
		CHECK(GetRoadOwner(tile, RoadTramType::Tram) == owner);
		CHECK(_me[tile].m8 == m8);
		CHECK(_me[tile].m6 == m6);
		SetRoadOwner(tile, RoadTramType::Road, CompanyID(128));
		SetRoadOwner(tile, RoadTramType::Tram, CompanyID(63));
	}
	MakeRoadNormal(tile, ROAD_X, ROADTYPE_ROAD, INVALID_ROADTYPE, TownID{0}, CompanyID(239), OWNER_NONE);
	CHECK(GetTileOwner(tile) == CompanyID(239));
	CHECK(GetRoadOwner(tile, RoadTramType::Road) == CompanyID(239));
	MakeRailTunnel(tile, CompanyID(200), TunnelID{0}, DiagDirection::NE, RailType{0});
	CHECK(GetTileOwner(tile) == CompanyID(200));
	MakeStation(tile, CompanyID(239), StationID{0}, StationType::Rail, 0);
	CHECK(GetTileOwner(tile) == CompanyID(239));
	for (WaterClass water_class : {WaterClass::Sea, WaterClass::Canal, WaterClass::River}) {
		MakeWater(tile, CompanyID(239), water_class, 0);
		for (uint id = 0; id < OWNER_END.base(); ++id) {
			SetTileOwner(tile, Owner(id));
			CHECK(GetWaterClass(tile) == water_class);
		}
	}
	DeallocateMap();
}

TEST_CASE("240 companies - legacy packed owners", "[company240]")
{
	AllocateMap(64, 64);
	ResetRoadTypes();
	TileIndex tile{0};
	MakeRoadBridgeRamp(tile, OWNER_NONE, OWNER_TOWN, OWNER_NONE, BridgeType{0}, DiagDirection::NE, ROADTYPE_ROAD, INVALID_ROADTYPE);
	_legacy_company_owners = true;
	_me[tile].m9 = _me[tile].m10 = 0;
	SB(_m[tile].m1, 0, 5, 16);
	SB(_me[tile].m7, 0, 5, 15);
	SB(_m[tile].m3, 4, 4, 15);
	CHECK(GetTileOwner(tile) == OWNER_NONE);
	CHECK(GetRoadOwner(tile, RoadTramType::Road) == OWNER_TOWN);
	CHECK(GetRoadOwner(tile, RoadTramType::Tram) == OWNER_NONE);
	SetTileOwner(tile, OWNER_WATER);
	CHECK(GB(_m[tile].m1, 0, 5) == 17);
	SetRoadOwner(tile, RoadTramType::Tram, OWNER_NONE);
	CHECK(GB(_m[tile].m3, 4, 4) == 15);
	_legacy_company_owners = false;
	DeallocateMap();
}

TEST_CASE("240 companies - cargo monitors and vehicle list identities", "[company240]")
{
	std::set<CargoMonitorID> monitors;
	std::set<uint32_t> lists;
	for (uint id = 0; id < MAX_COMPANIES; ++id) {
		CompanyID company(id);
		for (uint cargo = 0; cargo < NUM_CARGO; ++cargo) {
			auto monitor = EncodeCargoTownMonitor(company, CargoType(cargo), TownID{65534});
			CHECK(monitors.insert(monitor).second);
			CHECK(DecodeMonitorCompany(monitor) == company);
		}
		for (VehicleListType type : {VehicleListType::Company, VehicleListType::Station, VehicleListType::Depot, VehicleListType::Group, VehicleListType::Slot}) {
			for (VehicleType vtype : {VehicleType::Train, VehicleType::Road, VehicleType::Ship, VehicleType::Aircraft}) {
				VehicleListIdentifier original(type, vtype, company, 65535);
				auto packed = original.Pack();
				CHECK(lists.insert(packed).second);
				VehicleListIdentifier result;
				REQUIRE(result.UnpackIfValid(packed));
				CHECK(result.company == company);
				CHECK(result.vtype == vtype);
				CHECK(result.type == type);
				CHECK(result.index == 65535);
			}
		}
	}
}

TEST_CASE("240 companies - scrolling selector reaches the last company", "[company240]")
{
	MockEnvironment::Instance();
	AutoRestoreBackup dimensions(WidgetDimensions::scaled, WidgetDimensions::unscaled);
	WindowDesc desc(__FILE__, __LINE__, WindowPosition::Automatic, nullptr, 0, 0, WindowClass::None, WindowClass::None, {}, {});
	struct SelectorTestWindow : Window {
		using Window::Window;
		~SelectorTestWindow() override = default;
	} window(desc);
	window.window_class = WindowClass::Invalid;
	constexpr WidgetID SCROLLBAR = 240;
	auto root = MakeScrollableCompanyPanel(MakeCompanyButtonRows(0, 239, Colours::Brown, 8, STR_NULL), SCROLLBAR, Colours::Brown, 20);
	root->FillWidgetLookup(window.widget_lookup);
	root->AdjustPaddingForZoom();
	root->SetupSmallestSize(&window);
	root->AssignSizePosition(SizingType::Smallest, 0, 0, root->smallest_x, root->smallest_y, false);
	Scrollbar *scrollbar = window.GetScrollbar(SCROLLBAR);
	CHECK(scrollbar->GetCount() > scrollbar->GetCapacity());
	CHECK(root->smallest_y <= 20);
	NWidgetCore *first = window.GetWidget<NWidgetCore>(0);
	CHECK(root->GetWidgetFromPos(first->pos_x, first->pos_y) == first);
	scrollbar->SetPosition(scrollbar->GetCount());
	/* Hit testing updates positions after scrolling, before drawing. */
	root->GetWidgetFromPos(0, 0);
	NWidgetCore *last = window.GetWidget<NWidgetCore>(239);
	CHECK(last->pos_y < int(root->current_y));
	CHECK(last->pos_y + int(last->current_y) > 0);
	CHECK(root->GetWidgetFromPos(last->pos_x, last->pos_y) == last);
	CHECK(root->GetWidgetFromPos(last->pos_x, root->current_y + 1) == nullptr);
}

TEST_CASE("240 companies - AI performance tracking", "[company240]")
{
	CHECK(to_underlying(PerformanceElement::End) - to_underlying(PerformanceElement::AI0) == MAX_COMPANIES);
	for (uint id = 0; id < MAX_COMPANIES; ++id) {
		auto element = static_cast<PerformanceElement>(to_underlying(PerformanceElement::AI0) + id);
		{
			PerformanceMeasurer measure(element);
		}
		PerformanceMeasurer::SetInactive(element);
	}
}

TEST_CASE("NewGRF layout palettes do not alias company RGB palettes", "[company240][rgb]")
{
	const SpriteID image = 1U << PALETTE_MODIFIER_COLOUR;
	const PaletteID custom_palette = (1U << SPRITE_MODIFIER_CUSTOM_SPRITE) | 12345;
	CHECK(SpriteLayoutPaletteTransform(image, custom_palette, PAL_NONE) == 12345);
	CHECK(GroundSpritePaletteTransform(image, custom_palette, PAL_NONE) == 12345);
	CHECK(GetCompanyRGBBasePalette(SpriteLayoutPaletteTransform(image, custom_palette, PAL_NONE)) == 12345);
	CHECK(GetCompanyRGBBasePalette(GroundSpritePaletteTransform(image, custom_palette, PAL_NONE)) == 12345);
	/* Only layout flags are stripped: the default may be a real RGB palette. */
	const PaletteID rgb_palette = COMPANY_RGB_PALETTE | 42;
	CHECK(SpriteLayoutPaletteTransform(image, PAL_NONE, rgb_palette) == rgb_palette);
	CHECK(GroundSpritePaletteTransform(image, PAL_NONE, rgb_palette) == rgb_palette);
	CHECK(SpriteLayoutPaletteTransform(image, 775, rgb_palette) == 775);
	CHECK(GroundSpritePaletteTransform(image, 775, rgb_palette) == 775);
	CHECK(SpriteLayoutPaletteTransform(0, custom_palette, rgb_palette) == PAL_NONE);
	CHECK(GroundSpritePaletteTransform(0, custom_palette, rgb_palette) == PAL_NONE);
}

TEST_CASE("Company RGB hex parsing and exact rendering", "[company240][rgb]")
{
	MockEnvironment::Instance();
	CHECK(ParseCompanyRGBColour("#123aBc") == 0x123ABC);
	CHECK(ParseCompanyRGBColour("000000") == 0);
	CHECK(ParseCompanyRGBColour("FFFFFF") == 0xFFFFFF);
	for (const char *invalid : {"", "#", "12345", "1234567", "12GG34", "0x123456", " 123456"}) CHECK_FALSE(ParseCompanyRGBColour(invalid));
	Company *c = Company::CreateAtIndex(CompanyID(239));
	auto cleanup = scope_guard([] { _company_pool.CleanPool(); });
	c->colour_rgb = 0x123ABC;
	c->colour2_rgb = 0xF01234;
	PaletteID palette = GetCompanyRGBPalette(775, c->index, true, true);
	CHECK(palette != 775);
	CHECK(GetCompanyRGBBasePalette(palette) == 775);
	CHECK(GetCompanyRGBPalette(775, c->index, true, true) == palette);
	std::array<uint8_t, 256> base{}, fallback;
	std::array<Colour, 256> rgb;
	for (uint i = 0; i < 256; ++i) base[i] = i;
	REQUIRE(BuildCompanyRGBRemap(palette, base.data(), fallback, rgb));
	CHECK(rgb[0xC9].data == Colour(0x12, 0x3A, 0xBC).data);
	CHECK(rgb[0x53].data == Colour(0xF0, 0x12, 0x34).data);
	CHECK(rgb[0xC5].a == 0);
	CHECK(rgb[0xCE].a == 0);
	CHECK(fallback[0] == 0);
	CHECK(GetCompanyPixelColour(c->index, Shade::Normal).rgb == Colour(0x12, 0x3A, 0xBC).data);
	/* Exercise an actual 32bpp sprite pixel, including pure black and alpha. */
	Blitter_32bppSimple blitter;
	struct TestPixel { uint8_t r, g, b, a, m, v; } source{0, 0, 0, 255, 0xC9, 128};
	Colour destination(255, 255, 255);
	Blitter::BlitterParams bp{};
	bp.sprite = &source;
	bp.dst = &destination;
	bp.remap = fallback.data();
	bp.remap_rgb = rgb.data();
	bp.width = bp.height = bp.sprite_width = bp.sprite_height = bp.pitch = 1;
	blitter.Draw(&bp, BlitterMode::ColourRemap, ZoomLevel::Min);
	CHECK(destination.data == Colour(0x12, 0x3A, 0xBC).data);
	c->colour_rgb = 0;
	REQUIRE(BuildCompanyRGBRemap(palette, base.data(), fallback, rgb));
	blitter.Draw(&bp, BlitterMode::ColourRemap, ZoomLevel::Min);
	CHECK(destination.data == Colour(0, 0, 0).data);
	CHECK(fallback[0xC9] != 0);
	CHECK(fallback[0xC9] < PALETTE_ANIM_START);
	CHECK(CmdSetCompanyRGBColour({}, true, 0x1000000).Failed());
	CHECK_FALSE(CmdSetCompanyRGBColour({}, true, 0xFFFFFF).Failed());
}

TEST_CASE("Vehicle liveries repair unused magenta without changing legitimate pink", "[company240][rgb][livery]")
{
	MockEnvironment::Instance();
	Company *c = Company::CreateAtIndex(CompanyID(239));
	auto cleanup = scope_guard([] { _company_pool.CleanPool(); });
	c->colour_rgb = c->colour2_rgb = COMPANY_RGB_DEFAULT;
	std::array<uint8_t, 256> base{}, fallback;
	std::array<Colour, 256> rgb;
	for (uint i = 0; i < base.size(); ++i) base[i] = i;
	base[42] = 0xDD; // A custom vehicle palette also maps an ordinary source pixel to an unused entry.
	PaletteID palette = GetCompanyRGBPalette(775, c->index, true, true, true);
	REQUIRE(BuildCompanyRGBRemap(palette, base.data(), fallback, rgb));
	CHECK(GetCompanyRGBBasePalette(palette) == 775);
	CHECK(fallback[0] == 0);
	CHECK(fallback[42] == base[0xC9]);
	for (uint i = 0xD7; i <= 0xE2; ++i) CHECK(fallback[i] == base[0xC9]);
	for (uint i = 0x50; i <= 0x57; ++i) CHECK(fallback[i] == base[i]);
	/* Render the actual bad palette index with and without the livery guard. */
	AutoRestoreBackup restore_palette(_cur_palette, _cur_palette);
	_cur_palette.palette[0xD7] = Colour(212, 0, 212);
	_cur_palette.palette[0xC9] = Colour(20, 60, 180);
	Blitter_32bppSimple blitter;
	struct TestPixel { uint8_t r, g, b, a, m, v; } source{0, 0, 0, 255, 0xD7, 128};
	Colour destination;
	Blitter::BlitterParams bp{};
	bp.sprite = &source; bp.dst = &destination; bp.remap = base.data();
	bp.width = bp.height = bp.sprite_width = bp.sprite_height = bp.pitch = 1;
	blitter.Draw(&bp, BlitterMode::ColourRemap, ZoomLevel::Min);
	CHECK(destination.data == Colour(212, 0, 212).data);
	bp.remap = fallback.data(); bp.remap_rgb = rgb.data();
	blitter.Draw(&bp, BlitterMode::ColourRemap, ZoomLevel::Min);
	CHECK(destination.data == Colour(20, 60, 180).data);
	c->colour_rgb = 0x123ABC;
	palette = GetCompanyRGBPalette(775, c->index, true, false, true);
	REQUIRE(BuildCompanyRGBRemap(palette, base.data(), fallback, rgb));
	CHECK(rgb[0xD7].data == Colour(0x12, 0x3A, 0xBC).data);
	/* A NewGRF that deliberately recolours an unused input to a valid colour is preserved. */
	base[0xD7] = 0x50;
	REQUIRE(BuildCompanyRGBRemap(palette, base.data(), fallback, rgb));
	CHECK(fallback[0xD7] == 0x50);
	CHECK(rgb[0xD7].a == 0);
}

TEST_CASE("RGB company sprites across 32bpp blitters", "[company240][rgb]")
{
	MockEnvironment::Instance();
	if (VideoDriverBase::GetInstance() == nullptr) DriverFactoryBase::SelectDriver("null", Driver::Type::Video);
	Company *c = Company::CreateAtIndex(CompanyID(239));
	auto cleanup = scope_guard([] { _company_pool.CleanPool(); });
	c->colour_rgb = 0x123ABC;
	c->colour2_rgb = 0xF01234;
	PaletteID palette = GetCompanyRGBPalette(775, c->index, true, true);
	std::array<uint8_t, 256> base{}, fallback;
	std::array<Colour, 256> rgb;
	base.fill(230); // Force RGB fallbacks away from animated colours.
	base[0] = 0;
	REQUIRE(BuildCompanyRGBRemap(palette, base.data(), fallback, rgb));
	std::array<Colour, 8> destination{};
	AutoRestoreBackup screen(_screen, _screen);
	AutoRestoreBackup disable_anim(_screen_disable_anim, _screen_disable_anim);
	AutoRestoreBackup zoom_min(_settings_client.gui.zoom_min, ZoomLevel::Min);
	_screen.width = _screen.pitch = static_cast<int>(destination.size());
	_screen.height = 1;
	_screen.dst_ptr = destination.data();
	for (const char *name : {"32bpp-simple", "32bpp-optimized", "32bpp-anim", "32bpp-sse2", "32bpp-ssse3", "32bpp-sse4", "32bpp-sse2-anim", "32bpp-sse4-anim", "40bpp-anim"}) {
		CAPTURE(name);
		auto *factory = BlitterFactory::GetBlitterFactory(name);
		if (factory == nullptr) continue; // CPU-specific blitters need not exist on every build.
		auto blitter = factory->CreateInstance();
		_screen_disable_anim = std::string_view(name) == "40bpp-anim";
		blitter->PostResize();
		for (uint8_t alpha : {uint8_t(255), uint8_t(128)}) {
			std::array<SpriteLoader::CommonPixel, 8> pixels{};
			for (uint i = 0; i < pixels.size(); ++i) pixels[i] = {128, 128, 128, alpha, uint8_t(i % 2 == 0 ? 0xC9 : 0x53)};
			SpriteLoader::SpriteCollection sprites;
			for (ZoomLevel z : EnumRange(ZoomLevel::SpriteEnd)) {
				sprites[z] = {1, 8, 0, 0, {SpriteComponent::RGB, SpriteComponent::Alpha, SpriteComponent::Palette}, pixels.data()};
			}
			UniquePtrSpriteAllocator allocator;
			Sprite *sprite = blitter->Encode(SpriteType::Normal, sprites, allocator);
			REQUIRE(sprite != nullptr);
			for (int skip : {0, 1}) {
				destination.fill(Colour(255, 255, 255));
				Blitter::BlitterParams bp{};
				bp.sprite = sprite->data;
				bp.dst = destination.data();
				bp.remap = fallback.data();
				bp.remap_rgb = rgb.data();
				bp.skip_left = skip;
				bp.width = skip == 0 ? 8 : 5;
				bp.height = bp.sprite_height = 1;
				bp.sprite_width = bp.pitch = 8;
				blitter->Draw(&bp, BlitterMode::ColourRemap, ZoomLevel::Min);
				for (int i = 0; i < bp.width; ++i) {
					Colour expected = Blitter_32bppBase::ComposeColourPA(rgb[pixels[i + skip].m], alpha, Colour(255, 255, 255));
					/* Existing SIMD blitters use alpha+1 and different integer rounding. */
					int tolerance = alpha == 255 ? 0 : 2;
					CHECK(std::abs(int(destination[i].r) - expected.r) <= tolerance);
					CHECK(std::abs(int(destination[i].g) - expected.g) <= tolerance);
					CHECK(std::abs(int(destination[i].b) - expected.b) <= tolerance);
				}
			}
		}
		/* Palette animation must not replace RGB company pixels with their 8bpp fallback. */
		if (dynamic_cast<Blitter_32bppAnim *>(blitter.get()) != nullptr) {
			Palette changed = _cur_palette;
			changed.first_dirty = PALETTE_ANIM_START;
			changed.count_dirty = 256 - PALETTE_ANIM_START;
			auto before = destination;
			blitter->PaletteAnimate(changed);
			for (uint i = 0; i < destination.size(); ++i) CHECK(destination[i].data == before[i].data);
		}
	}
}

TEST_CASE("Company RGB graph lines label frames title bars and text", "[company240][rgb]")
{
	MockEnvironment::Instance();
	CompanyRGBVideoFactory video_factory;
	AutoRestoreBackup screen(_screen, _screen);
	AutoRestoreBackup dpi(_cur_dpi, &_screen);
	AutoRestoreBackup disable_anim(_screen_disable_anim, false);
	AutoRestoreBackup dimensions(WidgetDimensions::scaled, WidgetDimensions::unscaled);
	std::string previous_blitter = BlitterFactory::GetCurrentBlitter()->GetName();
	DriverFactoryBase::SelectDriver("company-rgb-test", Driver::Type::Video);
	Company *company = Company::CreateAtIndex(CompanyID(239));
	company->colour = Colours::White;
	_company_colours[company->index] = company->colour;
	auto cleanup = scope_guard([&] {
		Layouter::ResetFontCache(FontSize::Normal);
		BlitterFactory::SelectBlitter(previous_blitter);
		DriverFactoryBase::SelectDriver("null", Driver::Type::Video);
		_company_pool.CleanPool();
	});
	std::array<Colour, 64 * 64> destination{};
	_screen = {};
	_screen.dst_ptr = destination.data();
	_screen.width = _screen.height = _screen.pitch = 64;
	_screen.zoom = ZoomLevel::Min;
	for (const char *name : {"32bpp-simple", "32bpp-anim", "40bpp-anim"}) {
		CAPTURE(name);
		Blitter *blitter = BlitterFactory::SelectBlitter(name);
		REQUIRE(blitter != nullptr);
		blitter->PostResize();
		UniquePtrSpriteAllocator allocator;
		SpriteLoader::CommonPixel source{128, 128, 128, 255, 1};
		SpriteLoader::SpriteCollection sprites;
		sprites[ZoomLevel::Min] = {1, 1, 0, 0, {SpriteComponent::RGB, SpriteComponent::Alpha, SpriteComponent::Palette}, &source};
		const Sprite *glyph = blitter->Encode(SpriteType::Font, sprites, allocator);
		AutoRestoreBackup test_glyph(MockFontCache::glyph, glyph);
		for (uint32_t rgb : {0xA12F83u, 0u, 0xFFFFFFu}) {
			CAPTURE(rgb);
			company->colour_rgb = rgb;
			PixelColour colour = GetCompanyPixelColour(company->index, Shade::Normal);
			for (const auto &[start, end] : {std::pair{Point{3, 3}, Point{56, 3}}, std::pair{Point{4, 2}, Point{4, 57}}, std::pair{Point{-4, 6}, Point{55, 55}}}) {
				destination.fill(Colour(0, 0, 0, 0));
				std::fill_n(VideoDriverBase::GetInstance()->GetAnimBuffer(), destination.size(), 255);
				GfxDrawLine(blitter, &_screen, start.x, start.y, end.x, end.y, colour, 2, 3);
				uint count = 0;
				for (uint i = 0; i < destination.size(); ++i) {
					if (destination[i].data == 0) continue;
					CHECK(destination[i].data == colour.rgb);
					if (std::string_view(name) == "40bpp-anim") CHECK(VideoDriverBase::GetInstance()->GetAnimBuffer()[i] == 0);
					++count;
				}
				CHECK(count > 0);
			}
			destination.fill(Colour(0, 0, 0, 0));
			DrawCompanyFrameRect({2, 2, 30, 18}, company->index, {});
			CHECK(destination[10 * 64 + 10].data == GetCompanyPixelColour(company->index, Shade::Light).rgb);
			DrawCaption({2, 24, 50, 42}, Colours::Grey, company->index, TextColour::White, {}, {AlignmentH::Centre, AlignmentV::Middle}, FontSize::Normal);
			CHECK(destination[30 * 64 + 10].data == colour.rgb);
			ExtendedTextColour text = GetDrawStringCompanyColour(company->index);
			CHECK(text.rgb == colour.rgb);
			destination.fill(Colour(0, 0, 0, 0));
			DrawString(2, 60, 2, "A", text);
			CHECK(std::count_if(destination.begin(), destination.end(), [&](Colour pixel) { return pixel.data == colour.rgb; }) > 0);
		}
		Layouter::ResetFontCache(FontSize::Normal);
	}
	PixelColour background{0};
	background.rgb = Colour(255, 255, 255).data;
	CHECK(GetContrastColour(background) == TextColour::Black);
	background.rgb = Colour(0, 0, 0).data;
	CHECK(GetContrastColour(background) == TextColour::White);
}

TEST_CASE("240 companies - sharing charges revenue and vehicle profit", "[company240][sharing]")
{
	MockEnvironment::Instance();
	AllocateMap(64, 64);
	AutoRestoreBackup settings(_settings_game.economy, _settings_game.economy);
	_settings_game.economy.sharing_payment_in_debt = true;
	_settings_game.economy.sharing_fee[to_underlying(VehicleType::Train)] = 1000;
	_settings_game.economy.sharing_fee[to_underlying(VehicleType::Road)] = DAY_TICKS / 2;
	for (uint id = 0; id < MAX_COMPANIES; ++id) Company::CreateAtIndex(CompanyID(id));
	Train *train = Train::Create();
	RoadVehicle *road = RoadVehicle::Create();
	Station *station = Station::Create();
	auto cleanup = scope_guard([] {
		_vehicle_pool.CleanPool();
		_station_pool.CleanPool();
		_company_pool.CleanPool();
		DeallocateMap();
	});
	train->tile = TileIndex(0);
	train->gcache.cached_weight = 1000;
	train->running_ticks = DAY_TICKS;
	for (uint id = 0; id < MAX_COMPANIES; ++id) {
		CompanyID payer_id(id), recipient_id((id + 127) % MAX_COMPANIES);
		Company *payer = Company::Get(payer_id), *recipient = Company::Get(recipient_id);
		for (Company *c : {payer, recipient}) {
			c->money = 100000;
			c->money_fraction = 0;
			c->current_loan = 0;
			c->yearly_expenses = {};
			c->cur_economy = {};
		}
		train->owner = payer_id;
		train->profit_this_year = 0;
		MakeRailNormal(train->tile, recipient_id, TRACK_BIT_X, RailType(0));
		PayDailyTrackSharingFee(train);
		CHECK(payer->money == 99000);
		CHECK(recipient->money == 101000);
		CHECK(payer->yearly_expenses[0][ExpensesType::SharingCost] == 1000);
		CHECK(recipient->yearly_expenses[0][ExpensesType::SharingRevenue] == -1000);
		CHECK(payer->cur_economy.expenses == -1000);
		CHECK(recipient->cur_economy.income == 1000);
		CHECK(train->profit_this_year == Money(-(1000LL << 8)));
		road->owner = payer_id;
		road->profit_this_year = 0;
		station->owner = recipient_id;
		PayStationSharingFee(road, station);
		PayStationSharingFee(road, station);
		CHECK(payer->money == 98999);
		CHECK(recipient->money == 101001);
		CHECK(road->profit_this_year == -256);
		CHECK(payer->money_fraction == 0);
		CHECK(recipient->money_fraction == 0);
	}
	Company *payer = Company::Get(CompanyID(239));
	Company *recipient = Company::Get(CompanyID(15));
	train->owner = payer->index;
	MakeRailNormal(train->tile, recipient->index, TRACK_BIT_X, RailType(0));
	_settings_game.economy.sharing_payment_in_debt = false;
	payer->money = 500;
	payer->current_loan = 499;
	payer->money_fraction = 0;
	recipient->money = 0;
	recipient->money_fraction = 0;
	train->profit_this_year = 0;
	PayDailyTrackSharingFee(train);
	CHECK(payer->money == 499);
	CHECK(recipient->money == 1);
	CHECK(train->profit_this_year == -256);
	PayDailyTrackSharingFee(train);
	CHECK(payer->money == 499);
	CHECK(recipient->money == 1);
	_settings_game.economy.sharing_payment_in_debt = true;
	for (Owner owner : {OWNER_NONE, OWNER_TOWN, OWNER_WATER, payer->index}) {
		SetTileOwner(train->tile, owner);
		PayDailyTrackSharingFee(train);
		CHECK(payer->money == 499);
	}
}

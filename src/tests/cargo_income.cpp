/* This file is part of OpenTTD. Distributed under the GNU GPL, version 2. */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../cargo_income_func.h"

TEST_CASE("Cargo payments retain large map distances and packet counts", "[cargo][income]")
{
	for (uint distance : {1U, 100U, 4000U, 12000U, 32766U}) {
		for (uint count : {1U, 500U, 4000U, 65535U}) {
			uint64_t units = uint64_t(distance) * count * 255;
			int64_t expected = static_cast<int64_t>(units * 3185 >> 21);
			CHECK(ScaleCargoIncome(units, Money{3185}, 21).base() == expected);
			CHECK(ScaleCargoIncome(units, Money{3185}, 21).base() >= 0);
		}
	}
	CHECK(ScaleCargoIncome(uint64_t{32766} * 65535 * 255, Money{3185}, 21).base() > INT32_MAX / 4);
}

TEST_CASE("Cargo income handles inflated rates before fixed point scaling", "[cargo][income]")
{
	CHECK(ScaleCargoIncome(uint64_t{1} << 40, Money{int64_t{1} << 40}, 25).base() == (int64_t{1} << 55));
	CHECK(ScaleCargoIncome(1, Money::max(), 21).base() == (INT64_MAX >> 21));
	CHECK(ScaleCargoIncome(UINT64_MAX, Money{1}, 31).base() == static_cast<int64_t>(UINT64_MAX >> 31));
	CHECK(ScaleCargoIncome(UINT64_MAX, Money::max(), 0) == Money::max());
	CHECK(ScaleCargoIncome(0, Money::max(), 21) == 0);
	CHECK(ScaleCargoIncome(UINT64_MAX, Money{0}, 21) == 0);
}

TEST_CASE("Cargo aging keeps a speed premium and a long-distance floor", "[cargo][income]")
{
	for (int first : {0, 10, 255}) for (int second : {0, 24, 255}) {
		uint previous = CargoIncomeTimeFactor(0, first, second, 25);
		CHECK(previous == 255 * 16);
		for (uint age : {16U, 64U, 128U, 255U, 1024U, 65535U}) {
			uint factor = CargoIncomeTimeFactor(age, first, second, 25);
			CHECK(factor <= previous);
			CHECK(factor >= 64 * 16);
			previous = factor;
		}
		CHECK(previous == 64 * 16);
		CHECK(CargoIncomeTimeFactor(65535, first, second, 0) == 1);
		CHECK(CargoIncomeTimeFactor(65535, first, second, 100) == 255 * 16);
		/* Default-off behavior remains identical to the previous formula. */
		for (uint age : {0U, 31U, 112U, 255U, 1024U, 65535U}) {
			int a = std::max(int(age) - first, 0), b = std::max(a - second, 0);
			int over = -224 + (second > 224 ? int(age) - first : 2 * (int(age) - first) - second);
			uint expected = over > 0 ? std::max(2 * 31 * 16 * 16 / (over + 32), 1) : std::max(255 - a - b, 31) * 16;
			CHECK(CargoIncomeTimeFactor(age, first, second, 0) == expected);
		}
	}
}

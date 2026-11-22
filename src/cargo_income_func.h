/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License, version 2.
 */

/** @file cargo_income_func.h Wide fixed-point cargo payment arithmetic. */
#ifndef CARGO_INCOME_FUNC_H
#define CARGO_INCOME_FUNC_H

#include "money_type.h"

/** Payment time factor with four fractional bits. Zero floor preserves the original decay curve. */
inline uint CargoIncomeTimeFactor(uint periods, int first_period, int second_period, uint8_t floor_percent)
{
	const int first = std::max<int>(static_cast<int>(periods) - first_period, 0);
	const int second = std::max<int>(first - second_period, 0);
	const int over_max = second_period > 224 ? static_cast<int>(periods) - first_period - 224 : 2 * (static_cast<int>(periods) - first_period) - second_period - 224;
	const uint factor = over_max > 0 ? std::max(2 * 31 * 16 * 16 / (over_max + 2 * 16), 1) : std::max(255 - first - second, 31) * 16;
	return std::max<uint>(factor, ((255 * floor_percent + 99) / 100) * 16);
}

/** Multiply non-negative cargo units by a payment rate, then shift without intermediate overflow. */
inline Money ScaleCargoIncome(uint64_t units, Money rate, uint8_t shift)
{
	assert(rate >= 0 && shift <= 31);
	const uint64_t maximum = INT64_MAX;
	const uint64_t payment = rate.base();
	const uint64_t mask = (uint64_t{1} << shift) - 1;
	auto multiply = [maximum](uint64_t a, uint64_t b) { return a != 0 && b > maximum / a ? maximum : a * b; };
	auto add = [maximum](uint64_t a, uint64_t b) { return a > maximum - b ? maximum : a + b; };
	/* Splitting both operands also handles an inflated rate whose unshifted product exceeds 64 bits. */
	uint64_t whole = multiply(units >> shift, payment);
	uint64_t remainder = add(multiply(units & mask, payment >> shift), ((units & mask) * (payment & mask)) >> shift);
	return static_cast<int64_t>(add(whole, remainder));
}

#endif /* CARGO_INCOME_FUNC_H */

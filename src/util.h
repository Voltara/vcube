/* This file is part of vcube.
 *
 * Copyright (C) 2018 Andrew Skalski
 *
 * vcube is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * vcube is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with vcube.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef VCUBE_UTIL_H
#define VCUBE_UTIL_H

#include <array>
#include <string>
#include <vector>
#include "types.h"

namespace vcube {

namespace {
	template<typename T, int N, int R>
	constexpr auto init_NcR_rank() {
		auto tbl = std::array<T, 1 << N>();
		T rank = 0;
		for (int32_t i = (1 << N) - 1; i >= 0; i--) {
			if (__builtin_popcountl(i) == R) {
				tbl[i] = rank++;
			}
		}
		return tbl;
	}

	template<typename T, int N, int R, size_t ArraySize>
	constexpr auto init_NcR_unrank() {
		auto tbl = std::array<T, ArraySize>();
		T rank = 0;
		size_t idx = 0;
		for (int32_t i = (1 << N) - 1; i >= 0; i--) {
			if (__builtin_popcountl(i) == R) {
				tbl[idx++] = i;
			}
		}
		return tbl;
	}

	template<typename T, int N, int R>
	extern inline T rank_nCr(T bits) {
		static constexpr auto tbl = init_NcR_rank<T, N, R>();
		return tbl[bits];
	}

	template<typename T, int N, int R, size_t ArraySize>
	extern inline T unrank_nCr(T idx) {
		static constexpr auto tbl = init_NcR_unrank<T, N, R, ArraySize>();
		return tbl[idx];
	}

	static constexpr auto init_4perm_rank_oddeven() {
		/* To accommodate 'movemask' extraction of the permutation,
		 * the lookup table index has "odd-even" bit order: 75316420
		 */
		constexpr int fc[] = { 6, 2, 1 };
		std::array<uint8_t, 256> tbl = { };
		for (int perm = 0; perm < 24; perm++) {
			uint32_t table = 0x3210;
			uint8_t index = 0;
			for (int i = 0; i < 3; i++) {
				int shift = perm / fc[i] % (4 - i) * 4;
				uint8_t value = (table >> shift) & 3;
				table ^= (table ^ (table >> 4)) & (0xffffU << shift);
				index |= (value & 1) << i;
				index |= (value & 2) << (i + 3);
			}
			index |= (table & 1) << 3;
			index |= (table & 2) << 6;
			tbl[index] = perm;
		}
		return tbl;
	}
}

extern inline auto rank_8C4(uint8_t bits) {
	return rank_nCr<uint8_t, 8, 4>(bits);
}

extern inline auto rank_12C4(uint16_t bits) {
	return rank_nCr<uint16_t, 12, 4>(bits);
}

extern inline auto unrank_8C4(uint8_t idx) {
	return unrank_nCr<uint8_t, 8, 4, 70>(idx);
}

extern inline auto unrank_12C4(uint16_t idx) {
	return unrank_nCr<uint16_t, 12, 4, 495>(idx);
}

/* Set the orientation of the 12th edge based on parity of the other 11 */
extern inline eorient_t set_eorient_parity(eorient_t eorient) {
	// Faster than popcnt because of instruction latency
	eorient_t parity = eorient ^ (eorient << 6);
	parity ^= parity << 3;
	parity ^= (parity << 2) ^ (parity << 1);
	return eorient ^ (parity & 0x800);
}

extern inline auto rank_4perm_oddeven(uint8_t bits) {
	constexpr auto tbl = init_4perm_rank_oddeven();
	return tbl[bits];
}

struct moveseq_t : public std::vector<uint8_t> {
	enum style_t { SINGMASTER, FIXED };

	using std::vector<uint8_t>::vector;

	static moveseq_t parse(const std::string &);

	moveseq_t canonical() const;
	std::string to_string(style_t style = SINGMASTER) const;
};

}

#endif

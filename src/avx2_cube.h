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

#ifndef VCUBE_AVX2_CUBE_H
#define VCUBE_AVX2_CUBE_H

#include <stdint.h>
#include <x86intrin.h>
#include "types.h"
#include "util.h"

namespace vcube {

/* constexpr redeclarations of intrinsics */
namespace cx {
	extern inline constexpr __m256i mm256_set_epi64x( // avx
		int64_t a, int64_t b, int64_t c, int64_t d)
	{
		return (__m256i) { d, c, b, a };
	}
}

struct avx2 {
	static constexpr __m256i identity = cx::mm256_set_epi64x(
			0x0f0e0d0c0b0a0908, 0x0706050403020100,
			0x0f0e0d0c0b0a0908, 0x0706050403020100);

	static constexpr __m256i literal(
			uint64_t corners, uint64_t edges_high, uint64_t edges_low)
	{
		return cx::mm256_set_epi64x(
				0x0f0e0d0c0b0a0908, corners,
				0x0f0e0d0c00000000 | edges_high, edges_low);
	};

	static uint32_t bitmask(__m256i v, int b) {
		return _mm256_movemask_epi8(_mm256_slli_epi32(v, 7 - b));
	}

	static bool equals(__m256i a, __m256i b) {
		return _mm256_movemask_epi8(_mm256_cmpeq_epi8(a, b)) == -1;
	}

	static bool less_than(__m256i a, __m256i b) {
#if 1
		// One fewer instruction, except when used together with equals()
		uint32_t gt = _mm256_movemask_epi8(_mm256_cmpgt_epi8(a, b));
		uint32_t lt = _mm256_movemask_epi8(_mm256_cmpgt_epi8(b, a));
		return gt < lt;
#else
		/* Check that the most-significant 1-bit in "lt" is
		 * preceded by all 1-bits in "eq" by testing whether
		 * addition causes a rollover.
		 */
		uint32_t eq = _mm256_movemask_epi8(_mm256_cmpeq_epi8(a, b));
		uint32_t lt = _mm256_movemask_epi8(_mm256_cmpgt_epi8(b, a));
		// Invariant: (eq & lt) == 0
		uint32_t sum = (lt << 1) + eq;
		return sum < lt;
#endif
	}

	static __m256i compose(__m256i a, __m256i b, bool mirror = false) {
		const __m256i vcarry = _mm256_set_epi64x(
				0x3030303030303030, 0x3030303030303030,
				0x2020202020202020, 0x2020202020202020);

		// Permute edges and corners
		__m256i vperm = _mm256_shuffle_epi8(a, b);

		// Compose edge and corner orientations
		__m256i vorient = _mm256_and_si256(b, _mm256_set1_epi8(0xf0));
		if (mirror) {
			// Corner orientations are subtracted
			vperm = _mm256_sub_epi8(vperm, vorient);
			vperm = _mm256_min_epu8(vperm, _mm256_add_epi8(vperm, vcarry));
		} else {
			// Corner orientations are added
			vperm = _mm256_add_epi8(vperm, vorient);
			vperm = _mm256_min_epu8(vperm, _mm256_sub_epi8(vperm, vcarry));
		}

		return vperm;
	}

	static __m256i xor_edge_orient(__m256i v, eorient_t eorient) {
		__m256i vorient = _mm256_shuffle_epi8(
				_mm256_set1_epi32(eorient),
				_mm256_set_epi64x(-1, -1, 0xffffffff01010101, 0));
		vorient = _mm256_or_si256(vorient, _mm256_set1_epi64x(~0x8040201008040201));
		vorient = _mm256_cmpeq_epi8(vorient, _mm256_set1_epi64x(-1));
		vorient = _mm256_and_si256(vorient, _mm256_set1_epi8(0x10));
		return _mm256_xor_si256(v, vorient);
	}

	static corient_t corner_orient_raw(__m256i v) {
		__m256i vorient = _mm256_unpacklo_epi8(
				_mm256_slli_epi32(v, 3),
				_mm256_slli_epi32(v, 2));
		return corient_t(_mm256_movemask_epi8(vorient)) >> 16;
	}

	static __m256i invert(__m256i v) {
		// Split the cube into separate perm and orient vectors
		__m256i vperm = _mm256_and_si256(v, _mm256_set1_epi8(0x0f));
		__m256i vorient = _mm256_xor_si256(v, vperm);

#if 0
		// "Brute force" the inverse of the permutation
		__m256i vi = _mm256_set_epi64x(
				0x0f0e0d0c00000000, 0x0000000000000000,
				0x0f0e0d0c00000000, 0x0000000000000000);
		for (int i = 0; i < 12; i++) {
			__m256i vtrial = _mm256_set1_epi8(i);
			__m256i vcorrect = _mm256_cmpeq_epi8(identity, _mm256_shuffle_epi8(vperm, vtrial));
			vi = _mm256_or_si256(vi, _mm256_and_si256(vtrial, vcorrect));
		}
#else
		/* 27720 (11*9*8*7*5) is the LCM of all possible cycle decompositions,
		 * so we can invert the permutation by raising it to the 27719th power.
		 * This method tested faster (9-10%) than the above "brute force".
		 * The addition chain for 27719 was generated using the calculator
		 * provided by Achim Flammenkamp on his website:
		 *   http://wwwhomes.uni-bielefeld.de/achim/addition_chain.html
		 */
		__m256i vp3 = _mm256_shuffle_epi8(vperm, vperm); // 2
		vp3 = _mm256_shuffle_epi8(vp3, vperm);           // 3 (+1)
		__m256i vi = _mm256_shuffle_epi8(vp3, vp3);      // 6
		vi = _mm256_shuffle_epi8(vi, vi);                // 12
		vi = _mm256_shuffle_epi8(vi, vi);                // 24
		vi = _mm256_shuffle_epi8(vi, vp3);               // 27 (+3)
		vi = _mm256_shuffle_epi8(vi, vi);                // 54
		vi = _mm256_shuffle_epi8(vi, vi);                // 108
		vi = _mm256_shuffle_epi8(vi, vi);                // 216
		vi = _mm256_shuffle_epi8(vi, vi);                // 432
		vi = _mm256_shuffle_epi8(vi, vperm);             // 433 (+1)
		vi = _mm256_shuffle_epi8(vi, vi);                // 866
		vi = _mm256_shuffle_epi8(vi, vi);                // 1732
		vi = _mm256_shuffle_epi8(vi, vi);                // 3464
		vi = _mm256_shuffle_epi8(vi, vi);                // 6928
		vi = _mm256_shuffle_epi8(vi, vi);                // 13856
		vi = _mm256_shuffle_epi8(vi, vp3);               // 13859 (+3)
		vi = _mm256_shuffle_epi8(vi, vi);                // 27718
		vi = _mm256_shuffle_epi8(vi, vperm);             // 27719 (+1)
#endif

		// Invert the corner orientations
		const __m256i vcarry_corners = _mm256_set_epi64x(
				0x3030303030303030, 0x3030303030303030,
				0x1010101010101010, 0x1010101010101010);
		vorient = _mm256_add_epi8(vorient, vorient);
		vorient = _mm256_min_epu8(vorient, _mm256_sub_epi8(vorient, vcarry_corners));

		// Permute the edge and corner orientations
		vorient = _mm256_shuffle_epi8(vorient, vi);

		// Combine the new perm and orient
		return _mm256_or_si256(vi, vorient);
	}

	static uint64_t edges_low(__m256i v) {
		return _mm256_extract_epi64(v, 0);
	}

	static uint64_t edges_high(__m256i v) {
		return _mm256_extract_epi64(v, 1);
	}

	static uint64_t corners(__m256i v) {
		return _mm256_extract_epi64(v, 2);
	}

	static uint64_t unrank_corner_orient(corient_t corient) {
		/* 16-bit mulhi is lower latency than 32-bit, but has two disadvantages:
		 * - Requires two different shift widths
		 * - The multiplier for the 3^0 place is 65536
		 */
		static const __m256i vpow3_reciprocal = _mm256_set_epi32(
				1439, 4316, 12946, 38837, 7282, 21846, 0, 0);
		static const __m256i vshift = _mm256_set_epi32(
				4, 4, 4, 4, 0, 0, 0, 0);

		// Divide by powers of 3 (1, 3, 9, ..., 729)
		__m256i vcorient = _mm256_set1_epi32(corient);
		__m256i vco = _mm256_mulhi_epu16(vcorient, vpow3_reciprocal);
		vco = _mm256_srlv_epi32(vco, vshift);

		// fixup 3^0 place; reuse vcorient instead of inserting
		vco = _mm256_blend_epi32(vco, vcorient, 1 << 1);

		// Compute the remainder mod 3
		__m256i div3 = _mm256_mulhi_epu16(vco, _mm256_set1_epi32(21846)); // 21846/65536 ~ 1/3
		vco = _mm256_add_epi32(vco, div3);
		vco = _mm256_sub_epi32(vco, _mm256_slli_epi32(div3, 2));

		// Convert the results to a scalar
		vco = _mm256_shuffle_epi8(vco, _mm256_set_epi32(
					-1, -1, 0x0c080400, -1,
					-1, -1, -1, 0x0c080400));
		uint64_t co =
			_mm256_extract_epi64(vco, 2) |
			_mm256_extract_epi64(vco, 0);

		// Determine the last corner's orientation
		uint64_t sum = co + (co >> 32);
		sum += sum >> 16;
		sum += sum >> 8;

		// Insert the last corner
		co |= (0x4924924924924924 >> sum) & 3;

		return co << 4;
	}

	/* Return the parity of the edge+corner permutations */
	static bool parity(__m256i v) {
		v = _mm256_and_si256(v, _mm256_set1_epi8(0xf));

		__m256i a, b, c, d, e, f, g, h;
		a = _mm256_bslli_epi128(v, 1);    // shift left 1 byte
		b = _mm256_bslli_epi128(v, 2);    // shift left 2 bytes
		c = _mm256_bslli_epi128(v, 3);    // shift left 3 bytes
		d = _mm256_bslli_epi128(v, 4);    // shift left 4 bytes
		e = _mm256_bslli_epi128(v, 8);    // shift left 8 bytes
		f = _mm256_alignr_epi8(v, v, 11); // rotate left 5 bytes
		g = _mm256_alignr_epi8(v, v, 10); // rotate left 6 bytes
		h = _mm256_alignr_epi8(v, v, 9);  // rotate left 7 bytes
		// Test for inversions in the permutation
		a = _mm256_xor_si256(_mm256_cmpgt_epi8(a, v), _mm256_cmpgt_epi8(b, v));
		c = _mm256_xor_si256(_mm256_cmpgt_epi8(c, v), _mm256_cmpgt_epi8(d, v));
		e = _mm256_xor_si256(_mm256_cmpgt_epi8(e, v), _mm256_cmpgt_epi8(f, v));
		// Xor all the tests together
		__m256i parity = _mm256_xor_si256(_mm256_xor_si256(a, c), e);
		parity = _mm256_xor_si256(parity, _mm256_cmpgt_epi8(g, v));
		parity = _mm256_xor_si256(parity, _mm256_cmpgt_epi8(h, v));
		// The 0x5f corrects for the circular shifts, which cause
		// certain pairs of values to be compared out-of-order
		return _popcnt32(_mm256_movemask_epi8(parity) ^ 0x5f005f) & 1;
	}
};

}

#endif

/* This file is part of vcube.
 *
 * Copyright (C) 2018-2019 Andrew Skalski
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
	extern inline constexpr __m128i mm_set_epi64x( // sse2
		int64_t a, int64_t b)
	{
		return (__m128i) { b, a };
	}
}

struct avx2 {
#ifndef __AVX2__
	static __m256i mm256_set_m128( // avx
		__m128i a, __m128i b)
	{
		union {
			__m256i m256;
			__m128i m128[2];
		};
		m128[0] = b;
		m128[1] = a;
		return m256;
	}
#endif

	static constexpr __m256i identity = cx::mm256_set_epi64x(
			0x0f0e0d0c0b0a0908, 0x0706050403020100,
			0x0f0e0d0c0b0a0908, 0x0706050403020100);
	static constexpr __m128i identity128 = cx::mm_set_epi64x( //XXX
			0x0f0e0d0c0b0a0908, 0x0706050403020100);

	static constexpr __m256i literal(
			uint64_t corners, uint64_t edges_high, uint64_t edges_low)
	{
		return cx::mm256_set_epi64x(
				0x0f0e0d0c0b0a0908, corners,
				0x0f0e0d0c00000000 | edges_high, edges_low);
	};

	static uint32_t bitmask(__m256i v, int b) {
#ifdef __AVX2__
		return _mm256_movemask_epi8(_mm256_slli_epi32(v, 7 - b));
#else
		auto x = reinterpret_cast<__m128i *>(&v);
		return _mm_movemask_epi8(_mm_slli_epi32(x[0], 7 - b)) |
		       _mm_movemask_epi8(_mm_slli_epi32(x[0], 7 - b)) << 16;
#endif
	}

	static bool equals(__m256i a, __m256i b) {
#ifdef __AVX2__
		return _mm256_movemask_epi8(_mm256_cmpeq_epi8(a, b)) == -1;
#else
		auto x = reinterpret_cast<__m128i *>(&a);
		auto y = reinterpret_cast<__m128i *>(&b);
		return (_mm_movemask_epi8(_mm_cmpeq_epi8(x[0], y[0])) &
		        _mm_movemask_epi8(_mm_cmpeq_epi8(x[1], y[1]))) == 0xffff;
#endif
	}

	static bool less_than(__m256i a, __m256i b) {
#ifdef __AVX2__
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
#else
		auto x = reinterpret_cast<__m128i *>(&a);
		auto y = reinterpret_cast<__m128i *>(&b);
		uint32_t gt = _mm_movemask_epi8(_mm_cmpgt_epi8(x[0], y[0])) |
		              _mm_movemask_epi8(_mm_cmpgt_epi8(x[1], y[1])) << 16;
		uint32_t lt = _mm_movemask_epi8(_mm_cmpgt_epi8(y[0], x[0])) |
		              _mm_movemask_epi8(_mm_cmpgt_epi8(y[1], x[1])) << 16;
		return gt < lt;
#endif
	}

	static __m256i compose(__m256i a, __m256i b, bool mirror = false) {
#ifdef __AVX2__
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
#else
		auto x = reinterpret_cast<__m128i *>(&a);
		auto y = reinterpret_cast<__m128i *>(&b);

		const __m128i vcarry = _mm_set_epi64x(
				0x3030303030303030, 0x3030303030303030);

		// Permute edges and corners
		__m128i vperm_edges   = _mm_shuffle_epi8(x[0], y[0]);
		__m128i vperm_corners = _mm_shuffle_epi8(x[1], y[1]);

		// Compose edge and corner orientations
		__m128i vorient_edges   = _mm_and_si128(y[0], _mm_set1_epi8(0xf0));
		__m128i vorient_corners = _mm_and_si128(y[1], _mm_set1_epi8(0xf0));

		vperm_edges = _mm_xor_si128(vperm_edges, vorient_edges);

		if (mirror) {
			// Corner orientations are subtracted
			vperm_corners = _mm_sub_epi8(vperm_corners, vorient_corners);
			vperm_corners = _mm_min_epu8(vperm_corners, _mm_add_epi8(vperm_corners, vcarry));
		} else {
			// Corner orientations are added
			vperm_corners = _mm_add_epi8(vperm_corners, vorient_corners);
			vperm_corners = _mm_min_epu8(vperm_corners, _mm_sub_epi8(vperm_corners, vcarry));
		}

		return mm256_set_m128(vperm_corners, vperm_edges);
#endif
	}

	static __m256i xor_edge_orient(__m256i v, eorient_t eorient) {
#ifdef __AVX2__
		__m256i vorient = _mm256_shuffle_epi8(
				_mm256_set1_epi32(eorient),
				_mm256_set_epi64x(-1, -1, 0xffffffff01010101, 0));
		vorient = _mm256_or_si256(vorient, _mm256_set1_epi64x(~0x8040201008040201));
		vorient = _mm256_cmpeq_epi8(vorient, _mm256_set1_epi64x(-1));
		vorient = _mm256_and_si256(vorient, _mm256_set1_epi8(0x10));
		return _mm256_xor_si256(v, vorient);
#else
		auto x = reinterpret_cast<__m128i *>(&v);
		__m128i vorient = _mm_shuffle_epi8(
				_mm_set1_epi32(eorient),
				_mm_set_epi64x(0xffffffff01010101, 0));
		vorient = _mm_or_si128(vorient, _mm_set1_epi64x(~0x8040201008040201));
		vorient = _mm_cmpeq_epi8(vorient, _mm_set1_epi64x(-1));
		vorient = _mm_and_si128(vorient, _mm_set1_epi8(0x10));
		return mm256_set_m128(x[1], _mm_xor_si128(x[0], vorient));
#endif
	}

	static corient_t corner_orient_raw(__m256i v) {
#ifdef __AVX2__
		__m256i vorient = _mm256_unpacklo_epi8(
				_mm256_slli_epi32(v, 3),
				_mm256_slli_epi32(v, 2));
		return corient_t(_mm256_movemask_epi8(vorient)) >> 16;
#else
		auto x = reinterpret_cast<__m128i *>(&v);
		__m128i vorient = _mm_unpacklo_epi8(
				_mm_slli_epi32(x[1], 3),
				_mm_slli_epi32(x[1], 2));
		return corient_t(_mm_movemask_epi8(vorient));
#endif
	}

	static __m256i invert(__m256i v) {
#ifdef __AVX2__
		// Split the cube into separate perm and orient vectors
		__m256i vperm = _mm256_and_si256(v, _mm256_set1_epi8(0x0f));
		__m256i vorient = _mm256_xor_si256(v, vperm);

		// "Brute force" the inverse of the permutation
		__m256i vi = _mm256_set_epi64x(
				0x0f0e0d0c00000000, 0x0000000000000000,
				0x0f0e0d0c00000000, 0x0000000000000000);
		for (int i = 0; i < 12; i++) {
			__m256i vtrial = _mm256_set1_epi8(i);
			__m256i vcorrect = _mm256_cmpeq_epi8(identity, _mm256_shuffle_epi8(vperm, vtrial));
			vi = _mm256_or_si256(vi, _mm256_and_si256(vtrial, vcorrect));
		}

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
#else
		// TODO Test performance and optimize
		auto x = reinterpret_cast<__m128i *>(&v);

		// Split the cube into separate perm and orient vectors
		__m128i vperm_edges     = _mm_and_si128(x[0], _mm_set1_epi8(0x0f));
		__m128i vperm_corners   = _mm_and_si128(x[1], _mm_set1_epi8(0x0f));
		__m128i vorient_edges   = _mm_xor_si128(x[0], vperm_edges);
		__m128i vorient_corners = _mm_xor_si128(x[1], vperm_corners);

		// "Brute force" the inverse of the permutation
		__m128i vi_edges = _mm_set_epi64x(
				0x0f0e0d0c00000000, 0x0000000000000000);
		__m128i vi_corners = vi_edges;
		for (int i = 0; i < 12; i++) {
			__m128i vtrial = _mm_set1_epi8(i);
			__m128i vcorrect = _mm_cmpeq_epi8(identity128, _mm_shuffle_epi8(vperm_edges, vtrial));
			vi_edges = _mm_or_si128(vi_edges, _mm_and_si128(vtrial, vcorrect));

			vcorrect = _mm_cmpeq_epi8(identity128, _mm_shuffle_epi8(vperm_corners, vtrial));
			vi_corners = _mm_or_si128(vi_corners, _mm_and_si128(vtrial, vcorrect));
		}

		// Invert the corner orientations
		const __m128i vcarry_corners = _mm_set_epi64x(
				0x3030303030303030, 0x3030303030303030);
		vorient_corners = _mm_add_epi8(vorient_corners, vorient_corners);
		vorient_corners = _mm_min_epu8(vorient_corners, _mm_sub_epi8(vorient_corners, vcarry_corners));

		// Permute the edge and corner orientations
		vorient_edges   = _mm_shuffle_epi8(vorient_edges, vi_edges);
		vorient_corners = _mm_shuffle_epi8(vorient_corners, vi_corners);

		// Combine the new perm and orient
		return mm256_set_m128(
				_mm_or_si128(vi_corners, vorient_corners),
				_mm_or_si128(vi_edges, vorient_edges));
#endif
	}

	static uint64_t edges_low(__m256i v) {
#ifdef __AVX2__
		return _mm256_extract_epi64(v, 0);
#else
		auto x = reinterpret_cast<__m128i *>(&v);
		return _mm_extract_epi64(x[0], 0);
#endif
	}

	static uint64_t edges_high(__m256i v) {
#ifdef __AVX2__
		return _mm256_extract_epi64(v, 1);
#else
		auto x = reinterpret_cast<__m128i *>(&v);
		return _mm_extract_epi64(x[0], 1);
#endif
	}

	static uint64_t corners(__m256i v) {
#ifdef __AVX2__
		return _mm256_extract_epi64(v, 2);
#else
		auto x = reinterpret_cast<__m128i *>(&v);
		return _mm_extract_epi64(x[1], 0);
#endif
	}

	static uint64_t unrank_corner_orient(corient_t corient) {
#ifdef __AVX2__
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
		co |= (0444444444444444444444 >> sum) & 3;

		return co << 4;
#else
		// TODO Test performance and optimize

		/* 16-bit mulhi is lower latency than 32-bit, but has two disadvantages:
		 * - Requires two different shift widths
		 * - The multiplier for the 3^0 place is 65536
		 */
		static const __m128i vpow3_reciprocal_hi = _mm_set_epi32(
				1439, 4316, 12946, 38837);
		static const __m128i vpow3_reciprocal_lo = _mm_set_epi32(
				7282, 21846, 0, 0);

		// Divide by powers of 3 (1, 3, 9, ..., 729)
		__m128i vcorient = _mm_set1_epi32(corient);
		__m128i vco_hi = _mm_mulhi_epu16(vcorient, vpow3_reciprocal_hi);
		__m128i vco_lo = _mm_mulhi_epu16(vcorient, vpow3_reciprocal_lo);
		vco_hi = _mm_srli_epi32(vco_hi, 4);
		vco_lo = _mm_srli_epi32(vco_lo, 0);

		// fixup 3^0 place; reuse vcorient instead of inserting
		vco_lo = _mm_blend_epi32(vco_lo, vcorient, 1 << 1);

		// Compute the remainder mod 3
		__m128i div3_hi = _mm_mulhi_epu16(vco_hi, _mm_set1_epi32(21846)); // 21846/65536 ~ 1/3
		__m128i div3_lo = _mm_mulhi_epu16(vco_lo, _mm_set1_epi32(21846)); // 21846/65536 ~ 1/3
		vco_hi = _mm_add_epi32(vco_hi, div3_hi);
		vco_lo = _mm_add_epi32(vco_lo, div3_lo);
		vco_hi = _mm_sub_epi32(vco_hi, _mm_slli_epi32(div3_hi, 2));
		vco_lo = _mm_sub_epi32(vco_lo, _mm_slli_epi32(div3_lo, 2));

		// Convert the results to a scalar
		vco_hi = _mm_shuffle_epi8(vco_hi, _mm_set_epi32(
					-1, -1, 0x0c080400, -1));
		vco_lo = _mm_shuffle_epi8(vco_lo, _mm_set_epi32(
					-1, -1, -1, 0x0c080400));
		uint64_t co =
			_mm_extract_epi64(vco_hi, 0) |
			_mm_extract_epi64(vco_lo, 0);

		// Determine the last corner's orientation
		uint64_t sum = co + (co >> 32);
		sum += sum >> 16;
		sum += sum >> 8;

		// Insert the last corner
		co |= (0444444444444444444444 >> sum) & 3;

		return co << 4;
#endif
	}

	/* Return the parity of the edge+corner permutations */
	static bool parity(__m256i v) {
#ifdef __AVX2__
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
#else
		auto x = reinterpret_cast<__m128i *>(&v);

		__m128i vperm_edges   = _mm_and_si128(x[0], _mm_set1_epi8(0x0f));
		__m128i vperm_corners = _mm_and_si128(x[1], _mm_set1_epi8(0x0f));
		auto array_parity = [](uint8_t *arr, int size) -> bool {
			bool seen[12] = { }, p = false;
			for (int i = 0; i < size; i++) {
				if (seen[i]) {
					p ^= 1;
				} else {
					for (int j = arr[i]; j != i; j = arr[j]) {
						seen[j] = true;
					}
				}
			}
			return p;
		};

		return array_parity(reinterpret_cast<uint8_t *>(&vperm_edges),  12) ^
		       array_parity(reinterpret_cast<uint8_t *>(&vperm_corners), 8);
#endif
	}
};

}

#endif

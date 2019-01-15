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

#ifndef VCUBE_SSE_CUBE_H
#define VCUBE_SSE_CUBE_H

#include <stdint.h>
#include <x86intrin.h>
#include "types.h"
#include "util.h"

namespace vcube {

struct sse {
	//static const __m128i identity = _mm_set_epi64x(
	//		0x0f0e0d0c0b0a0908, 0x0706050403020100);
	static constexpr __m128i identity = (__m128i) {
		0x0706050403020100, 0x0f0e0d0c0b0a0908
	};

	static uint32_t bitmask(__m128i v, int b) {
		return _mm_movemask_epi8(_mm_slli_epi32(v, 7 - b));
	}

	static bool equals(__m128i a, __m128i b) {
		return _mm_movemask_epi8(_mm_cmpeq_epi8(a, b)) == -1;
	}

	static bool less_than(__m128i a, __m128i b) {
		// See avx2_cube.h
#if 1
		uint32_t gt = _mm_movemask_epi8(_mm_cmpgt_epi8(a, b));
		uint32_t lt = _mm_movemask_epi8(_mm_cmpgt_epi8(b, a));
		return gt < lt;
#else
		uint32_t eq = _mm_movemask_epi8(_mm_cmpeq_epi8(a, b));
		uint32_t lt = _mm_movemask_epi8(_mm_cmpgt_epi8(b, a));
		uint32_t sum = (lt << 1) + eq;
		return sum < lt;
#endif
	}

	static __m128i edge_compose(__m128i a, __m128i b) {
		__m128i vperm = _mm_shuffle_epi8(a, b);
		__m128i vorient = _mm_and_si128(b, _mm_set1_epi8(0xf0));
		return _mm_xor_si128(vperm, vorient);
	}

	static __m128i xor_edge_orient(__m128i v, eorient_t eorient) {
		__m128i vorient = _mm_shuffle_epi8(
				_mm_set1_epi32(eorient),
				_mm_set_epi64x(0xffffffff01010101, 0));
		vorient = _mm_or_si128(vorient, _mm_set1_epi64x(~0x8040201008040201));
		vorient = _mm_cmpeq_epi8(vorient, _mm_set1_epi64x(-1));
		vorient = _mm_and_si128(vorient, _mm_set1_epi8(0x10));
		return _mm_xor_si128(v, vorient);
	}

	static corient_t corner_orient(__m128i v) {
		// Mask the corner orientation bits and convert to 16-bit vector
		auto vorient = _mm_and_si128(v, _mm_set1_epi8(0x30));
		vorient = _mm_unpacklo_epi8(vorient, _mm_setzero_si128());

		// Multiply each corner by its place value, add adjacent pairs
		vorient = _mm_madd_epi16(vorient, _mm_set_epi16(729, 243, 81, 27, 9, 3, 1, 0));

		// Finish the horizontal sum
		uint64_t r = _mm_extract_epi64(vorient, 0) + _mm_extract_epi64(vorient, 1);
		r += r >> 32;

		// Shift the result into the correct position
		return r >> 4;
	}
};

}

#endif

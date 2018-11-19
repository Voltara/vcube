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

#ifndef VCUBE_CUBE_H
#define VCUBE_CUBE_H

#include <array>
#include <string>
#include <stdint.h>
#include <x86intrin.h>

#include "sse_cube.h"
#include "avx2_cube.h"
#include "types.h"
#include "util.h"

namespace vcube {

template<typename T>
class cube_base {
    public:
	/* Test cubes for inequality */
	bool operator != (T o) const {
		return !(static_cast<const T&>(*this) == o);
	}

	/* Composition */
	T operator * (T o) const {
		return compose(o);
	}

	T& operator *= (T o) {
		return static_cast<T&>(*this) = compose(o);
	}

	/* Composition variant for mirrored (S_LR2) left operand.
	 * Corner twist is subtracted rather than added.
	 */
	T operator % (T o) const {
		return compose(o, true);
	}

	T& operator %= (T o) {
		return static_cast<T&>(*this) = compose(o, true);
	}

	/* Get edge orientation without parity reduction 0..4095 */
	eorient_t getEdgeOrientRaw() const {
		return edge_bitmask(4);
	}

	/* Get edge orientation coordinate 0..2047 */
	eorient_t getEdgeOrient() const {
		return getEdgeOrientRaw() & 0x7ff;
	}

	/* Get U/D slice edge orientation coordinate 0..255 */
	eorient_t getEdge8Orient() const {
		auto e_layer = edge_bitmask(3);
		return _pext_u32(getEdgeOrientRaw(), ~e_layer);
	}

	/* Get equatorial slice edge orientation coordinate 0..15 */
	eorient_t getEdge4Orient() const {
		auto e_layer = edge_bitmask(3);
		return _pext_u32(getEdgeOrientRaw(), e_layer);
	}

	/* Set edge orientation coordinate 0..2047 */
	T& setEdgeOrient(eorient_t eorient) {
		xor_edge_orient(set_eorient_parity(eorient ^ getEdgeOrientRaw()));
		return static_cast<T&>(*this);
	}

	/* Set U/D slice edge orientation from coordinate 0..255 */
	T& setEdge8Orient(e8orient_t e8orient) {
		e8orient_t parity = e8orient ^ (e8orient >> 4);
		parity = (0x6996 >> (parity & 0xf)) & 1;
		auto e_layer = edge_bitmask(3);
		auto ori = _pdep_u32(e8orient, ~e_layer) | _pdep_u32(parity, e_layer);
		xor_edge_orient(ori ^ getEdgeOrientRaw());
		return static_cast<T&>(*this);
	}

	/* Set equatorial slice edge orientation from coordinate 0..15 */
	T& setEdge4Orient(e4orient_t e4orient) {
		e4orient_t parity = (0x6996 >> e4orient) & 1;
		auto e_layer = edge_bitmask(3);
		auto ori = _pdep_u32(e4orient, e_layer) | _pdep_u32(parity, ~e_layer);
		xor_edge_orient(ori ^ getEdgeOrientRaw());
		return static_cast<T&>(*this);
	}

	/* Get corner orientation without parity reduction; returns a 16-bit
	 * packed integer with a 2-bit field for each corner orientation (0..2)
	 */
	corient_t getCornerOrientRaw() const {
		return corner_orient_raw();
	}

	/* Get corner orientation coordinate 0..2186 */
	corient_t getCornerOrient() const {
		return corner_orient();
	}

	/* Get the 4! equatorial permutation 0..23 */
	e4perm_t getEdge4Perm() const {
		// The 0xf000 and 0xa000 XORs correct for the 0xfedc pseudo-edges
		uint32_t e_layer = edge_bitmask(3);
		e_layer ^= (e_layer << 12) ^ 0xf000;
		uint32_t e = edge_bitmask(0) ^ (edge_bitmask(1) << 12) ^ 0xa000;
		return rank_4perm_oddeven(_pext_u32(e, e_layer));
	}

	/* Get the 8C4 U/D face edge combination 0..69 */
	eud4comb_t getEdgeUD4Comb() const {
		uint32_t e_layer = edge_bitmask(3);
		uint32_t d_layer = edge_bitmask(2);
		return rank_8C4(_pext_u32(d_layer, e_layer ^ 0xfff) & 0xff);
	}

    private:
	T compose(T o, bool mirror = false) const {
		return static_cast<const T*>(this)->compose(o, mirror);
	}

	uint32_t edge_bitmask(int bit) const {
		return static_cast<const T*>(this)->edge_bitmask(bit);
	}

	uint32_t corner_bitmask(int bit) const {
		return static_cast<const T*>(this)->corner_bitmask(bit);
	}

	void xor_edge_orient(eorient_t eorient) {
		static_cast<T*>(this)->xor_edge_orient(eorient);
	}

	corient_t corner_orient_raw() const {
		return static_cast<const T*>(this)->corner_orient_raw();
	}

	corient_t corner_orient() const {
		return static_cast<const T*>(this)->corner_orient();
	}
};

class cube : public cube_base<cube> {
	friend class cube_base<cube>;

    public:
	constexpr cube() : v(avx2::identity) {
	}

	constexpr cube(uint64_t corners, uint64_t edges_high, uint64_t edges_low) :
		v(avx2::literal(corners, edges_high, edges_low))
	{
	}

	/* Parse a move sequence */
	static cube from_moves(const std::string &s);
	static cube from_movev(const std::vector<uint8_t> &v);

	/* Parse a cube position in Singmaster's notation */
	static cube from_singmaster(std::string s);

	/* Parse a cube position in Speffz cycles, corners first
	 * Corners and edges are delimited by the "." character.
	 * Lowercase letters cycle the edge/corner sticker with the buffer,
	 * and uppercase letters twist the edge/corner in place.
	 * Uppercase edges always flip (A and Q are equivalent)
	 * Uppercase corners twist the U/D sticker into the specified position
	 * (P twists the URF corner clockwise; J twists it counterclockwise)
	 */
	static cube from_speffz(const std::string &s, int corner_buffer = 'A', int edge_buffer = 'A');

	/* Test cubes for equality */
	bool operator == (const cube &o) const {
		return avx2::equals(v, o.v);
	}

	/* Ordering operator for use with std::set, std::map, etc. */
	bool operator < (const cube &o) const {
		return avx2::less_than(v, o.v);
	}

	/* Convert to __m256i */
	operator const __m256i & () const {
		return v;
	}

	/* Invert the cube (88M/sec, 120M/sec pipelined) */
	cube operator ~ () const {
		return avx2::invert(v);
	}

	/* Compose with another cube (1080M/sec, 2517M/sec pipelined) */
	cube compose(const cube &o, bool mirror = false) const {
		return avx2::compose(v, o.v, mirror);
	}

	/* Parity of the edge+corner permutation */
	bool parity() const {
		return avx2::parity(v);
	}

	/* Move */
	cube move(int m) const;

	/* Premove */
	cube premove(int m) const;

	/* Conjugate with a symmetry */
	cube symConjugate(int s) const;

	/* Set corner orientation from coordinate 0..2186 */
	cube & setCornerOrient(corient_t corient);

	/* Set full edge permutation coordinate 0..479001599 and reset edge orientation */
	cube & setEdgePerm(eperm_t eperm) {
		constexpr uint32_t fc[] = { 39916800, 3628800, 362880, 40320, 5040, 720, 120, 24, 6, 2, 1 };
		uint64_t table = 0xba9876543210;

		uint8_t *edge = reinterpret_cast<uint8_t*>(&ev());

		// Special case, first iteration does not need "% 12"
		for (int i = 0; i < 1; i++) {
			int shift = eperm / fc[i] * 4;
			edge[i] = _bextr_u64(table, shift, 4);
			table ^= (table ^ (table >> 4)) & (int64_t(-1) << shift);
		}

		for (int i = 1; i < 11; i++) {
			int shift = eperm / fc[i] % (12 - i) * 4;
			edge[i] = _bextr_u64(table, shift, 4);
			table ^= (table ^ (table >> 4)) & (int64_t(-1) << shift);
		}

		edge[11] = table;

		return *this;
	}

	/* Get full edge permutation coordinate 0..479001599 */
	eperm_t getEdgePerm() const {
		uint64_t table = 0xba9876543210;
		eperm_t eperm = 0;

		uint64_t e = avx2::edges_low(v) << 2;
		for (int i = 0; i < 8; i++) {
			int shift = e & 0x3c;
			eperm = eperm * (12 - i) + _bextr_u64(table, shift, 4);
			table -= 0x111111111110LL << shift;
			e >>= 8;
		}

		e = avx2::edges_high(v) << 2;
		for (int i = 8; i < 11; i++) {
			int shift = e & 0x3c;
			eperm = eperm * (12 - i) + _bextr_u64(table, shift, 4);
			table -= 0x111111111110LL << shift;
			e >>= 8;
		}

		return eperm;
	}

	/* Set full corner permutation coordinate 0..40319 and reset corner orientation */
	cube & setCornerPerm(cperm_t cperm) {
		constexpr uint32_t fc[] = { 5040, 720, 120, 24, 6, 2, 1 };
		uint32_t table = 0x76543210;

		uint8_t *corner = reinterpret_cast<uint8_t*>(&cv());

		for (int i = 0; i < 7; i++) {
			int shift = cperm / fc[i] % (8 - i) * 4;
			corner[i] = _bextr_u64(table, shift, 4);
			table ^= (table ^ (table >> 4)) & (-1L << shift);
		}

		corner[7] = table;

		return *this;
	}

	/* Get full corner permutation coordinate 0..40319 */
	cperm_t getCornerPerm() const {
		uint32_t table = 0x76543210;
		cperm_t cperm = 0;

		uint64_t c = avx2::corners(v) << 2;
		for (int i = 0; i < 7; i++) {
			int shift = c & 0x3c;
			cperm = cperm * (8 - i) + _bextr_u64(table, shift, 4);
			table -= 0x11111110 << shift;
			c >>= 8;
		}

		return cperm;
	}

	/* Set a representative 8C4 U/D face corner combination 0..69 */
	cube & setCorner4Comb(c4comb_t c4comb);

	/* Get the 8C4 U/D face corner combination 0..69 */
	c4comb_t getCorner4Comb() const {
		// SSE tested faster than AVX, and makes it compatible with getCornerOrient
		uint32_t d_layer = sse::bitmask(cv(), 2);
		return rank_8C4(d_layer & 0xff);
	}

	/* Set a representative 12C4 equatorial / non-equatorial combination 0..494 */
	cube & setEdge4Comb(e4comb_t e4comb);

	/* Get the 12C4 equatorial / non-equatorial combination 0..494 */
	e4comb_t getEdge4Comb() const {
		uint32_t e_layer = sse::bitmask(ev(), 3);
		return rank_12C4(e_layer & 0xfff);
	}

	/* Set a representative 4! equatorial permutation 0..23 */
	cube & setEdge4Perm(e4perm_t e4perm) {
		return setEdgePerm(e4perm);
	}

	/* Set a representative 8C4 U/D face edge combination 0..69 */
	cube & setEdgeUD4Comb(eud4comb_t eud4comb);

    private:
	friend class edgecube;

	/* Low 128-bit lane:
	 *   4 U-face edges
	 *   4 D-face edges
	 *   4 E-slice edges "equatorial"
	 *   4 (unused)
	 *
	 * High 128-bit lane:
	 *   4 U-face corners
	 *   4 D-face corners
	 *   8 (unused)
	 *
	 * Edge values (8 bits):
	 *   ---OEEEE
	 *   - = unused (zero)
	 *   O = orientation
	 *   E = edge index (0..11)
	 *
	 * Corner values (8 bits):
	 *   --OO-CCC
	 *   - = unused (zero)
	 *   O = orientation (0..2)
	 *   C = corner index (0..7)
	 */
	__m256i v;

	constexpr cube(__m256i v) : v(v) {
	}

	__m128i & ev() {
		return reinterpret_cast<__m128i *>(&v)[0];
	}

	__m128i ev() const {
		return reinterpret_cast<const __m128i *>(&v)[0];
	}

	__m128i & cv() {
		return reinterpret_cast<__m128i *>(&v)[1];
	}

	__m128i cv() const {
		return reinterpret_cast<const __m128i *>(&v)[1];
	}

	uint64_t * u64() {
		return reinterpret_cast<uint64_t *>(&v);
	}

	/* cube_base */

	uint32_t edge_bitmask(int bit) const {
		return avx2::bitmask(v, bit) & 0xffff;
	}

	uint32_t corner_bitmask(int bit) const {
		return avx2::bitmask(v, bit) >> 16;
	}

	void xor_edge_orient(eorient_t eorient) {
		v = avx2::xor_edge_orient(v, eorient);
	}

	corient_t corner_orient() const {
		return sse::corner_orient(cv());
	}

	corient_t corner_orient_raw() const {
		return avx2::corner_orient_raw(v);
	}
};

/* Fundamental symmetries:
 *   S_URF3 - 120-degree clockwise rotation on URF-DBL axis (x y)
 *   S_U4   - 90-degree clockwise rotation on U-D axis (y)
 *   S_LR2  - Reflection left to right
 */
static constexpr cube S_URF3 = {0x1226172321152410,0x12161410,0x0a170b1309150811};
static constexpr cube S_U4   = {0x0605040702010003,0x1a19181b,0x0605040702010003};
static constexpr cube S_LR2  = {0x0607040502030001,0x0a0b0809,0x0704050603000102};

/* Fundamental moves:
 *   M_U - 90-degree clockwise twist of the U face
 */
static constexpr cube M_U    = {0x0706050402010003,0x0b0a0908,0x0706050402010003};

#ifndef GENERATE_CONST_CUBES
/* Generated moves and symmetries*/
#include "const-cubes.h"
#endif

extern inline cube cube::move(int m) const {
	return compose(moves[m]);
}

extern inline cube cube::premove(int m) const {
	return moves[m].compose(*this);
}

extern inline cube cube::symConjugate(int s) const {
	return sym[sym_inv[s]].compose(*this, s & 1).compose(sym[s], s & 1);
}

/* Edges-only specialization of the cube that can take advantage of faster composition */
class edgecube : public cube_base<edgecube> {
	friend class cube_base<edgecube>;
    public:
	constexpr edgecube() : v(sse::identity) {
	}

	constexpr edgecube(const cube &c) : v(c.ev()) {
	}

	/* Convert to __m128i */
	operator const __m128i & () const {
		return v;
	}

	/* Compose */
	edgecube compose(const edgecube &o, bool = false) const {
		return sse::edge_compose(v, o.v);
	}

	/* Move */
	edgecube move(int m) const {
		auto ec_m = reinterpret_cast<const edgecube *>(&moves[m]);
		return compose(*ec_m);
	}

	/* Premove */
	edgecube premove(int m) const {
		auto ec_m = reinterpret_cast<const edgecube *>(&moves[m]);
		return ec_m->compose(*this);
	}

	/* Conjugate with a symmetry */
	edgecube symConjugate(int s) const {
		auto ec_symi = reinterpret_cast<const edgecube *>(&sym[sym_inv[s]]);
		auto ec_sym  = reinterpret_cast<const edgecube *>(&sym[s]);
		return ec_symi->compose(*this).compose(*ec_sym);
	}

    private:
	__m128i v;

	constexpr edgecube(__m128i v) : v(v) {
	}

	/* cube_base */

	uint32_t edge_bitmask(int bit) const {
		return sse::bitmask(v, bit);
	}

	void xor_edge_orient(eorient_t eorient) {
		v = sse::xor_edge_orient(v, eorient);
	}
};

}

#endif

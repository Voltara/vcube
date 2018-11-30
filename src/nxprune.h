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

#ifndef VCUBE_NXPRUNE_H
#define VCUBE_NXPRUNE_H

#include <array>
#include <vector>
#include <tuple>
#include <cstdio>
#include "cube.h"
#include "cube6.h"
#include "sse_cube.h"

/* This implements the pruning tables used by Tomas Rokicki's nxopt.
 * See https://github.com/rokicki/cube20src for a better description
 * of how it works.
 */

namespace vcube::nx {

/* Number of corner sym-coordinates */
static constexpr uint32_t N_CORNER_SYM = 9930;

/* EP variants preserve varying information about the edge permutation.
 *
 * EP1 distinguishes between E-layer and non-E-layer edges
 * EP2 is EP1 + preserves the relative position of E-layer edges
 * EP3 is EP1 + distinguishes between U-layer and D-layer edges
 * EP4 is EP1 + EP2 + EP3
 *
 * U-layer: UR UF UL UB (up)
 * E-layer: FR FL BL BR (equatorial)
 * D-layer: DR DF DL DB (down)
 *
 *      DF FR UF BR FL DB UB DR DL BL UL UR
 * EP1: -- EE -- EE EE -- -- -- -- EE -- --
 * EP2: -- FR -- BR FL -- -- -- -- BL -- --
 * EP3: DD EE UU EE EE DD UU DD DD EE UU UU
 * EP4: DD FR UU BR FL DD UU DD DD BL UU UU
 */
enum EPvariant {
	EP1, // 12C4            ==    495
	EP2, // 12C4       * 4! ==  11880
	EP3, // 12C4 * 8C4      ==  34650
	EP4  // 12C4 * 8C4 * 4! == 831600
};

/* EO variants preserve varying information about the edge orientation.
 *
 * EO4  E-layer edges
 * EO8  U-layer and D-layer edges
 * EO12 All edges
 */
enum EOvariant {
	EO4, // 2^4  ==   16
	EO8, // 2^8  ==  256
	EO12 // 2^11 == 2048
};

/* Edge coordinate.  Instantiate this with the desired combination
 * of EP and EO variant
 */
template<EPvariant EP, EOvariant EO>
class ecoord {
	static constexpr std::array<uint32_t, 4> N_EP = { 1, 24, 70, 1680 };
	static constexpr std::array<uint32_t, 3> N_EO = { 16, 256, 2048 };
    public:
	static constexpr uint32_t N_ECOORD = N_EP[EP] * N_EO[EO] * 512;

	ecoord() : coord() {
	}

	ecoord(const edgecube &ec, int sym = 0) {
		__m128i ve = ec.symConjugate(sym);

		// 0x10: edge orient
		uint32_t eorient = sse::bitmask(ve, 4);
		// 0x08: equatorial layer
		uint32_t e_layer = sse::bitmask(ve, 3) & 0xfff;
		// 0x04: down layer
		uint32_t d_layer = sse::bitmask(ve, 2);

		uint32_t dcomb = rank_8C4(_pext_u32(d_layer, e_layer ^ 0xfff));
		uint32_t ecomb = rank_12C4(e_layer);

		uint32_t e4 = sse::bitmask(ve, 0) ^ (sse::bitmask(ve, 1) << 12) ^ 0xa000;
		uint32_t e4perm = rank_4perm_oddeven(_pext_u32(e4, e_layer | (e_layer << 12)));

		/* Insert gaps into the 0..494 numbering to make room for
		 * the 4-bit min-of-62 pruning values.
		 * Note: 33/2048 approximates 1/62 efficiently
		 */
		ecomb += (ecomb + 63) * 33 / 2048 * 2;

		switch (EP) {
		    case EP1:
			coord = 0;
			break;
		    case EP2:
			coord = e4perm;
			break;
		    case EP3:
			coord = dcomb;
			break;
		    case EP4:
			coord = N_E4PERM * dcomb + e4perm;
			break;
		}

		uint32_t eo;
		switch (EO) {
		    case EO4:
			eo = _pext_u32(eorient, e_layer);
			coord = (coord << 13) | (eo << 9) | ecomb;
			break;
		    case EO8:
			eo = _pext_u32(eorient, e_layer ^ 0xfff);
			coord = (coord << 17) | (eo << 9) | ecomb;
			break;
		    case EO12:
			eo = eorient & 0x7ff;
			coord = (coord << 20) | (eo << 9) | ecomb;
			break;
		}
	}

	operator uint32_t () const {
		return coord;
	}

	/* Apply a representative edge orientation to a cube */
	static void applyEO(edgecube &c, eorient_t eorient) {
		switch (EO) {
		    case EO4:
			c.setEdge4Orient(eorient);
			break;
		    case EO8:
			c.setEdge8Orient(eorient);
			break;
		    case EO12:
			c.setEdgeOrient(eorient);
			break;
		}
	}

	/* Returns a vector of representative cubes for the permutation
	 * variant specified in the template parameter.  These cubes do not
	 * include the "ecomb" (equatorial 12C4) part of the coordinate.
	 */
	static auto getEPnCubes() {
		std::vector<edgecube> c_EPn;
		switch (EP) {
		    case EP1:
			c_EPn.resize(1);
			break;
		    case EP2:
			c_EPn.resize(N_E4PERM);
			for (e4perm_t i = 0; i < N_E4PERM; i++) {
				c_EPn[i] = cube().setEdge4Perm(i);
			}
			break;
		    case EP3:
			c_EPn.resize(N_EUD4COMB);
			for (eud4comb_t i = 0; i < N_EUD4COMB; i++) {
				c_EPn[i] = cube().setEdgeUD4Comb(i);
			}
			break;
		    case EP4:
			c_EPn.resize(N_EUD4COMB * N_E4PERM);
			for (e4perm_t i = 0; i < N_E4PERM; i++) {
				c_EPn[i] = cube().setEdge4Perm(i);
			}
			for (eud4comb_t i = 1; i < N_EUD4COMB; i++) {
				uint32_t base = i * N_E4PERM;
				c_EPn[base] = cube().setEdgeUD4Comb(i);
				for (e4perm_t i = 1; i < N_E4PERM; i++) {
					// The e4perm and ud4comb coordinates are orthogonal
					c_EPn[base + i] = c_EPn[base] * c_EPn[i];
				}
			}
			break;
		}
		return c_EPn;
	}

	/* Decodes the high part of the coordinate (shifted right 9 bits) */
	static auto decode(uint32_t coord) {
		uint32_t high, low, eo;
		low = coord & 0x1ff;
		coord >>= 9;
		switch (EO) {
		    case EO4:
			eo = coord & 0xf;
			high = coord >> 4;
			break;
		    case EO8:
			eo = coord & 0xff;
			high = coord >> 8;
			break;
		    case EO12:
			eo = coord & 0x7ff;
			high = coord >> 11;
			break;
		}
		return std::make_tuple(high, low, eo);
	}

    private:
	uint32_t coord;
};

/* Edge coordinate representative cube generator */
class ecoord_rep {
    public:
	template<typename ECoord>
	void init() {
		for (e4comb_t i = 0; i < N_E4COMB; i++) {
			e4comb_t adj = (i + 63) * 33 / 2048 * 2;
			c_EP1[i + adj] = cube().setEdge4Comb(i);
		}
		c_EPn = ECoord::getEPnCubes();
	}

	template<typename ECoord>
	edgecube get(uint32_t high, uint32_t low, uint32_t eo) {
		edgecube ec = c_EPn[high] * c_EP1[low];
		ECoord::applyEO(ec, eo);
		return ec;
	}

    private:
	// Representatives for the 512 EP1 coordinates (same for all EP variants)
	std::array<edgecube, 512> c_EP1;
	// Representatives for the rest of the EP coordinate (varies)
	std::vector<edgecube> c_EPn;
};

/* Raw corner coordinate */
class ccoord {
    public:
	ccoord(const cube &c) {
		coord = (c.getCornerOrientRaw() << 8) | c.getCorner4Comb();
	}

	operator uint32_t () const {
		return coord;
	}

	/* Returns a representative cube for 16-way symmetry, and the
	 * symmetry index used to make that transformation
	 */
	static cube rep(const cube &c, uint8_t &sym) {
		sym = 0;
		cube rep = c;
		ccoord best = c;
		for (int s = 1; s < 16; s++) {
			cube c_s = c.symConjugate(s);
			ccoord coord(c_s);
			if (coord < best) {
				sym = s;
				rep = c_s;
				best = coord;
			}
		}
		return rep;
	}

    private:
	uint32_t coord;
};

/* The pruning table is indexed first by corner sym-coordinate.
 * This class is responsible for the mapping of raw corners to
 * sym corners
 */
class prune_base {
	struct offset_sym_t {
		uint8_t offset; // offset from base sym-coordinate
		uint8_t sym;    // conjugate -> representative

		bool operator == (const offset_sym_t &o) const {
			return offset == o.offset && sym == o.sym;
		}
	};

	struct index_t {
		uint16_t base;    // lowest sym-coordinate for this corient
		offset_sym_t *os; // offsets and symmetries for each c4comb
		uint8_t *prune;   // pointer to pruning table entries
	};

    public:
	size_t size() const {
		return stride * N_CORNER_SYM;
	}

	bool save(const std::string &filename) const;
	bool load(const std::string &filename);
	bool loadShared(uint32_t key, const std::string &filename = "");

    protected:
	prune_base(size_t stride);

	void setPrune(decltype(index_t::prune) p);

	std::vector<cube> getCornerRepresentatives() const;

	uint8_t * getPruneRow(uint32_t corner_sym) {
		return index[0].prune + corner_sym * stride;
	}

	uint16_t sym_coord(const cube &c) const {
		auto &idx = index[c.getCornerOrient()];
		auto &os = idx.os[c.getCorner4Comb()];
		return idx.base + os.offset;
	}

	uint8_t get_sym(const cube &c) const {
		auto &idx = index[c.getCornerOrient()];
		auto &os = idx.os[c.getCorner4Comb()];
		return os.sym;
	}

	/* The offset_sym array, when deduplicated, has 139 distinct patterns */
	using os_unique_t = std::array<offset_sym_t, N_C4COMB>;
	static constexpr uint32_t N_UNIQUE_OFFSET_SYM = 139;
	std::array<os_unique_t, N_UNIQUE_OFFSET_SYM> os_unique;

	std::array<index_t, N_CORNER_SYM> index;

	size_t stride;
};

template<typename ECoord, int Base>
class prune : public prune_base {
	template<typename Prune> friend class prune_generator;

	struct prefetch_t {
		uint32_t edge;
		const uint8_t *stripe;
		uint8_t fetch() const {
			auto &octet = stripe[(edge / 4) % 16];
			auto shift = (edge % 4) * 2;
			auto val = (octet >> shift) & 3;
			return val ? (BASE + val) : (stripe[0] & 0xf);
		}
	};

	prefetch_t prefetch(const cube &c) const {
		auto &idx = index[c.getCornerOrient()];
		auto &os = idx.os[c.getCorner4Comb()];
		auto edge = ECoord(c, os.sym);
		auto stripe = &idx.prune[16 * (N_EDGE_STRIPE * os.offset + edge / 64)];
		_mm_prefetch(stripe, _MM_HINT_T0);
		return { edge, stripe };
	}

    public:
	using ecoord = ECoord;
	static constexpr uint64_t N_EDGE_STRIPE = ecoord::N_ECOORD / 64;
	static constexpr int BASE = Base;

	prune() : prune_base(16 * N_EDGE_STRIPE) {
	}

	uint8_t lookup(const cube6 &c6, uint8_t limit, uint32_t &prune_vals, int skip, int val, uint8_t &axis_mask) const {
		prefetch_t pre[6];
		if (skip != 0) pre[0] = prefetch(c6[0]);
		if (skip != 1) pre[1] = prefetch(c6[1]);
		if (skip != 2) pre[2] = prefetch(c6[2]);
		if (skip != 3) pre[3] = prefetch(c6[3]);
		if (skip != 4) pre[4] = prefetch(c6[4]);
		if (skip != 5) pre[5] = prefetch(c6[5]);

		uint8_t prune[6];
		if (skip != 0xff) {
			prune[skip] = val;
		}

		if (0 != skip) {
			prune[0] = pre[0].fetch();
			if (prune[0] > limit) {
				return prune[0];
			}
		}
		if (1 != skip) {
			prune[1] = pre[1].fetch();
			if (prune[1] > limit) {
				return prune[1];
			}
		}
		if (2 != skip) {
			prune[2] = pre[2].fetch();
			if (prune[2] > limit) {
				return prune[2];
			}
		}

		prune_vals = (prune[0] << 0) | (prune[1] << 4) | (prune[2] << 8);
		if (!prune_vals) {
			return 0;
		}

		uint32_t prune_cmp0 = (1 << prune[0]) | (1 << prune[1]) | (1 << prune[2]);
		prune_cmp0 |= _blsi_u32(prune_cmp0) << 1;
		uint8_t prune0 = 31 - _lzcnt_u32(prune_cmp0);
		if (prune0 > limit) {
			return prune0;
		}

		if (3 != skip) {
			prune[3] = pre[3].fetch();
			if (prune[3] > limit) {
				return prune[3];
			}
		}
		if (4 != skip) {
			prune[4] = pre[4].fetch();
			if (prune[4] > limit) {
				return prune[4];
			}
		}
		if (5 != skip) {
			prune[5] = pre[5].fetch();
			if (prune[5] > limit) {
				return prune[5];
			}
		}

		prune_vals |= (prune[3] << 12) | (prune[4] << 16) | (prune[5] << 20);

		uint32_t prune_cmp1 = (1 << prune[3]) | (1 << prune[4]) | (1 << prune[5]);
		prune_cmp1 |= _blsi_u32(prune_cmp1) << 1;
		uint8_t prune1 = 31 - _lzcnt_u32(prune_cmp1);
		if (prune1 > limit) {
			return prune1;
		}

		axis_mask =
			((prune[0] == limit) << 0) |
			((prune[1] == limit) << 1) |
			((prune[2] == limit) << 2) |
			((prune[3] == limit) << 3) |
			((prune[4] == limit) << 4) |
			((prune[5] == limit) << 5);

		return (prune0 < prune1) ? prune1 : prune0;
	}

	uint8_t initial_depth(const cube6 &c6) const {
		uint32_t prune_vals;
		uint8_t axis_mask;
		return lookup(c6, 0xff, prune_vals, -1, 0, axis_mask);
	}

    private:
	void init(uint8_t *mem) {
		setPrune(mem);
	}
};

}

#endif

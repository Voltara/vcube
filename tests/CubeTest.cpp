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

#include "cube.h"

#include <set>
#include "test_util.h"
#include "CppUTest/TestHarness.h"

using namespace vcube;

TEST_GROUP(Cube) {
	static void check_edge_orient(const cube &c) {
		int eorient_sum = 0;
		for (auto eo = c.getEdgeOrientRaw(); eo; eo >>= 1) {
			eorient_sum += eo & 1;
		}
		// The sum of all edge orientations must be divisible by 2
		LONGS_EQUAL(0, eorient_sum % 2);
	}

	static void check_edge_permutation(const cube &c) {
		uint32_t edge_mask = 0;
		for (int i = 0; i < 12; i++) {
			int edge_idx = t::to_array(c)[i] & 0xf;
			edge_mask |= 1 << edge_idx;
		}
		// Each edge cubie should occur exactly once
		LONGS_EQUAL(0xfff, edge_mask);
	}

	static void check_corner_orient(const cube &c) {
		int corient_sum = 0;
		for (auto co = c.getCornerOrientRaw(); co; co >>= 2) {
			int corient = co & 3;
			// Corners may only be oriented 0, 1, or 2
			CHECK(corient < 3);
			corient_sum += corient;
		}
		// The sum of all corner orientations must be divisible by 3
		LONGS_EQUAL(0, corient_sum % 3);
	}

	static void check_corner_permutation(const cube &c) {
		uint32_t corner_mask = 0;
		for (int i = 16; i < 24; i++) {
			int corner_idx = t::to_array(c)[i] & 0x7;
			corner_mask |= 1 << corner_idx;
		}
		// Each corner cubie should occur exactly once
		LONGS_EQUAL(0xff, corner_mask);
	}

	static void check_invariants(const cube &c) {
		uint8_t edge_bits = 0;
		for (int i = 0; i < 12; i++) {
			edge_bits |= t::to_array(c)[i];
		}
		CHECK((edge_bits & 0xe0) == 0);

		uint8_t corner_bits = 0;
		for (int i = 16; i < 24; i++) {
			corner_bits |= t::to_array(c)[i];
		}
		CHECK((corner_bits & 0xc8) == 0);

		// Check unused edge positions (12..15)
		int edge_padding_ok = 0;
		for (int i = 12; i < 16; i++) {
			edge_padding_ok += (t::to_array(c)[i] == i % 16);
		}
		CHECK(edge_padding_ok == 4);

		// Check unused corner positions (24..31)
		int corner_padding_ok = 0;
		for (int i = 24; i < 32; i++) {
			corner_padding_ok += (t::to_array(c)[i] == i % 16);
		}
		CHECK(corner_padding_ok == 8);

		check_corner_orient(c);
		check_corner_permutation(c);
		check_edge_orient(c);
		check_edge_permutation(c);
	}
};

TEST(Cube, DefaultConstructor) {
	LONGS_EQUAL(sizeof(cube), 32);

	cube c;
	for (int i = 0; i < 32; i++) {
		LONGS_EQUAL(i % 16, t::to_array(c)[i]);
	}

	check_invariants(c);
}

TEST(Cube, EdgeOrient) {
	cube c;

	LONGS_EQUAL(0, c.getEdgeOrient());

	for (int i = 0; i < N_EORIENT; i++) {
		c = t::random_cube();
		c.setEdgeOrient(i);
		LONGS_EQUAL(i, c.getEdgeOrient());
		check_invariants(c);
	}

	/* setEdgeOrient should only affect edge orient */
	for (int i = 0; i < 1000; i++) {
		cube old = c = t::random_cube();
		c.setEdgeOrient(c.getEdgeOrient());
		CHECK(c == old);
	}
}

TEST(Cube, Edge4Orient) {
	cube c;

	LONGS_EQUAL(0, c.getEdge4Orient());

	for (int i = 0; i < N_E4ORIENT; i++) {
		c = t::random_cube();
		c.setEdge4Orient(i);
		LONGS_EQUAL(i, c.getEdge4Orient());
		check_invariants(c);
	}

	/* Flipping the other eight edges should not affect the coordinate */
	for (int i = 0; i < 1000; i++) {
		c = t::random_cube();

		/* Flip random e8 edges */
		eorient_t e8flip = t::rand(256);
		e8flip ^= _popcnt32(e8flip) & 1;
		cube c_e8flip;
		c_e8flip.setEdgeOrient(e8flip);

		CHECK(c.getEdge4Orient() == (c_e8flip * c).getEdge4Orient());
	}

	/* setEdge4Orient should only affect edge orient */
	for (int i = 0; i < 1000; i++) {
		cube old = c = t::random_cube();
		c.setEdge4Orient(c.getEdge4Orient());
		CHECK(c.getEdge4Orient() == old.getEdge4Orient());
		CHECK(c.getEdgePerm() == old.getEdgePerm());
		CHECK(c.getCornerPerm() == old.getCornerPerm());
		CHECK(c.getCornerOrient() == old.getCornerOrient());
	}
}

TEST(Cube, Edge8Orient) {
	cube c;

	LONGS_EQUAL(0, c.getEdge8Orient());

	for (int i = 0; i < N_E8ORIENT; i++) {
		c = t::random_cube();
		c.setEdge8Orient(i);
		LONGS_EQUAL(i, c.getEdge8Orient());
		check_invariants(c);
	}

	/* Flipping the other four edges should not affect the coordinate */
	for (int i = 0; i < 1000; i++) {
		c = t::random_cube();

		/* Flip random e4 edges */
		cube c_e4flip;
		c_e4flip.setEdgeOrient(t::rand(8) << 8);

		CHECK(c.getEdge8Orient() == (c_e4flip * c).getEdge8Orient());
	}

	/* setEdge8Orient should only affect edge orient */
	for (int i = 0; i < 1000; i++) {
		cube old = c = t::random_cube();
		c.setEdge8Orient(c.getEdge8Orient());
		CHECK(c.getEdge8Orient() == old.getEdge8Orient());
		CHECK(c.getEdgePerm() == old.getEdgePerm());
		CHECK(c.getCornerPerm() == old.getCornerPerm());
		CHECK(c.getCornerOrient() == old.getCornerOrient());
	}
}

TEST(Cube, CornerOrient) {
	cube c;

	LONGS_EQUAL(0, c.getCornerOrient());

	for (int i = 0; i < N_CORIENT; i++) {
		c = t::random_cube();
		c.setCornerOrient(i);
		LONGS_EQUAL(i, c.getCornerOrient());
		check_invariants(c);
	}

	/* setCornerOrient should only affect corner orient */
	for (int i = 0; i < 1000; i++) {
		cube old = c = t::random_cube();
		c.setCornerOrient(c.getCornerOrient());
		CHECK(c == old);
	}

	/* corner orientations are lexically ordered */
	int prev = -1;
	for (int i = 0; i < N_CORIENT; i++) {
		c.setCornerOrient(i);
		uint16_t cur = c.getCornerOrientRaw();
		CHECK(prev < cur);
		prev = cur;
	}
}

TEST(Cube, EdgePerm) {
	cube c;

	LONGS_EQUAL(0, c.getEdgePerm());

	c.setEdgePerm(N_EPERM - 1);
	LONGS_EQUAL(N_EPERM - 1, c.getEdgePerm());

	for (int i = 0; i < 1000; i++) {
		eperm_t eperm = t::rand(N_EPERM);
		c.setEdgeOrient(t::rand(N_EORIENT));
		c.setEdgePerm(eperm);
		LONGS_EQUAL(eperm, c.getEdgePerm());
		LONGS_EQUAL(0, c.getEdgeOrient());
		check_invariants(c);
	}

	/* setEdgePerm should only affect edge perm+orient */
	for (int i = 0; i < 1000; i++) {
		cube old = c = t::random_cube();
		eorient_t eorient = c.getEdgeOrient();
		c.setEdgePerm(c.getEdgePerm());
		c.setEdgeOrient(eorient);
		CHECK(c == old);
	}
}

TEST(Cube, CornerPerm) {
	cube c;

	LONGS_EQUAL(0, c.getCornerPerm());

	c.setCornerPerm(N_CPERM - 1);
	LONGS_EQUAL(N_CPERM - 1, c.getCornerPerm());

	for (int i = 0; i < 1000; i++) {
		cperm_t cperm = t::rand(N_CPERM);
		c.setCornerOrient(t::rand(N_CORIENT));
		c.setCornerPerm(cperm);
		LONGS_EQUAL(cperm, c.getCornerPerm());
		LONGS_EQUAL(0, c.getCornerOrient());
		check_invariants(c);
	}

	/* setCornerPerm should only affect corner perm+orient */
	for (int i = 0; i < 1000; i++) {
		cube old = c = t::random_cube();
		corient_t corient = c.getCornerOrient();
		c.setCornerPerm(c.getCornerPerm());
		c.setCornerOrient(corient);
		CHECK(c == old);
	}
}

TEST(Cube, Parity) {
	cube c;
	CHECK_EQUAL(c.parity(), false);

	bool expected_parity = false;
	for (int i = 0; i < 1000; i++) {
		int x = t::rand(12), y = t::rand(12);
		std::swap(t::to_array(c)[x], t::to_array(c)[y]);
		expected_parity ^= (x != y);
		CHECK_EQUAL(c.parity(), expected_parity);

		x = t::rand(8), y = t::rand(8);
		std::swap(t::to_array(c)[16 + x], t::to_array(c)[16 + y]);
		expected_parity ^= (x != y);
		CHECK_EQUAL(c.parity(), expected_parity);
	}

	// Orientation should not affect parity
	for (int i = 0; i < 1000; i++) {
		c = t::random_cube();
		bool parity = c.parity();

		c.setCornerOrient(t::rand(N_CORIENT));
		CHECK_EQUAL(parity, c.parity());

		c.setEdgeOrient(t::rand(N_EORIENT));
		CHECK_EQUAL(parity, c.parity());
	}
}

TEST(Cube, ParitySwap) {
	cube c;
	for (int i = 0; i < 1000; i++) {
		// Edge parity can be fixed by "eperm ^= 1"
		eperm_t eperm = t::rand(N_EPERM);
		c.setEdgePerm(eperm);
		std::swap(t::to_array(c)[10], t::to_array(c)[11]);
		CHECK_EQUAL(eperm ^ 1, c.getEdgePerm());

		// Corner parity can be fixed by "cperm ^= 1"
		cperm_t cperm = t::rand(N_CPERM);
		c.setCornerPerm(cperm);
		std::swap(t::to_array(c)[22], t::to_array(c)[23]);
		CHECK_EQUAL(cperm ^ 1, c.getCornerPerm());
	}
}

TEST(Cube, CubeEquality) {
	cube c, d;

	CHECK(c == c); CHECK_FALSE(c != c);
	CHECK(c == d); CHECK_FALSE(c != d);

	d = cube(); d.setEdgeOrient(1);
	CHECK(c != d); CHECK_FALSE(c == d);

	d = cube(); d.setEdgePerm(1);
	CHECK(c != d); CHECK_FALSE(c == d);

	d = cube(); d.setCornerOrient(1);
	CHECK(c != d); CHECK_FALSE(c == d);

	d = cube(); d.setCornerPerm(1);
	CHECK(c != d); CHECK_FALSE(c == d);
}

TEST(Cube, CubeLessThan) {
	cube c, d = t::random_cube();

	for (int i = 0; i < 1000; i++) {
		c = d;

		CHECK_FALSE_TEXT(c < c, "Cube compared unequal to itself");
		CHECK_FALSE_TEXT(c < d, "c and d are the same cube");
		CHECK_FALSE_TEXT(d < c, "c and d are the same cube");

		d = t::random_cube();

		CHECK_TEXT(c != d, "Rolled the same cube twice in a row?");
		CHECK_TEXT((c < d) ^ (d < c), "Exactly one of (c < d) or (d < c) should be true");
	}
}

TEST(Cube, Invert) {
	CHECK(cube() == ~cube());

	for (int i = 0; i < 1000; i++) {
		cube c = t::random_cube();
		cube ci = ~c;
		CHECK(c * ci == cube());
		check_invariants(ci);
	}
}

TEST(Cube, Symmetry) {
	// 48 unique symmetries
	std::set<cube> s;
	for (int i = 0; i < 48; i++) {
		s.insert(vcube::sym[i]);
	}
	CHECK_TEXT(s.size() == 48, "Symmetries are not all unique");

	// Inverse table is correct
	for (int i = 0; i < 48; i++) {
		CHECK(vcube::sym[i].compose(vcube::sym[sym_inv[i]], i & 1) == cube());
	}

	// sym[0] is the identity
	CHECK(vcube::sym[0] == cube());

	// S_LR2 is the least significant bit
	for (int i = 0; i < 48; i++) {
		CHECK(vcube::sym[i] * vcube::S_LR2 == vcube::sym[i ^ 1]);
	}

	// S_URF3 is the most significant
	for (int i = 0; i < 48; i++) {
		CHECK(vcube::S_URF3 * vcube::sym[i] == vcube::sym[(i + 16) % 48]);
	}

	// The symmetry group is closed
	for (int i = 0; i < 48; i++) {
		for (int j = 0; j < 48; j++) {
			CHECK(s.find(vcube::sym[i].compose(vcube::sym[j], i & 1)) != s.end());
		}
	}

	// Mirroring works as expected
	CHECK(vcube::moves[ 0] == vcube::S_LR2 % vcube::moves[ 2] % vcube::S_LR2);
	CHECK(vcube::moves[ 3] == vcube::S_LR2 % vcube::moves[14] % vcube::S_LR2);
	CHECK(vcube::moves[ 6] == vcube::S_LR2 % vcube::moves[ 8] % vcube::S_LR2);
	CHECK(vcube::moves[ 9] == vcube::S_LR2 % vcube::moves[11] % vcube::S_LR2);
	CHECK(vcube::moves[12] == vcube::S_LR2 % vcube::moves[ 5] % vcube::S_LR2);
	CHECK(vcube::moves[15] == vcube::S_LR2 % vcube::moves[17] % vcube::S_LR2);
}

TEST(Cube, SymConjugate) {
	for (int s = 0; s < 48; s++) {
		cube c = t::random_cube();
		CHECK(c.symConjugate(s) == vcube::sym[sym_inv[s]] * c * vcube::sym[s]);
		s++;
		CHECK(c.symConjugate(s) == vcube::sym[sym_inv[s]] % c % vcube::sym[s]);
	}
}

TEST(Cube, Move) {
	// 18 unique moves
	std::set<cube> s;
	for (int i = 0; i < 18; i++) {
		s.insert(vcube::moves[i]);
	}
	CHECK_TEXT(s.size() == 18, "Moves are not all unique");

	for (int i = 0; i < 18; i += 3) {
		CHECK(vcube::moves[i    ] * vcube::moves[i] == vcube::moves[i + 1]);
		CHECK(vcube::moves[i + 1] * vcube::moves[i] == vcube::moves[i + 2]);
	}
}

TEST(Cube, Compose) {
	for (int i = 0; i < 1000; i++) {
		cube c = t::random_cube();
		cube d = t::random_cube();
		check_invariants(c * d);
		check_invariants(c % d);
	}
}

TEST(Cube, MoveParse) {
	CHECK(cube::from_moves("") == cube());

	cube superflip = cube().setEdgeOrient(N_EORIENT - 1);

	CHECK(cube::from_moves("U R2 F B R B2 R U2 L B2 R U' D' R2 F R' L B2 U2 F2\n") == superflip);
	CHECK(cube::from_moves("U1R2F1B1R1B2R1U2L1B2R1U3D3R2F1R3L1B2U2F2") == superflip);
	CHECK(cube::from_moves("URRFBRBBRUULBBRUUUDDDRRFRRRLBBUUFF") == superflip);
}

TEST(Cube, NumericMoves) {
	CHECK(cube::from_movev({}) == cube());
	CHECK(cube::from_movev({0,3,6,9,12,15,1,4,7,10,13,16,2,5,8,11,14,17}) == cube::from_moves("URFDLBU2R2F2D2L2B2U'R'F'D'L'B'"));
}

TEST(Cube, ReidParse) {
	CHECK(cube::from_reid("uf ur ub ul df dr db dl fr fl br bl ufr urb ubl ulf drf dfl dlb dbr") == cube());

	// Cube within a cube
	cube c = cube::from_moves("F L F U' R U F2 L2 U' L' B D' B' L2 U");

	cube d = cube::from_reid("UF UR FL FD BR BU DB DL FR RD LU BL UFR FUL FLD FDR BUR BRD DLB BLU\n");
	CHECK(c == d);
}

TEST(Cube, SpeffzParse) {
	CHECK(cube::from_speffz("") == cube());

	// Cube within a cube
	cube c = cube::from_moves("F L F U' R U F2 L2 U' L' B D' B' L2 U");
	cube d = cube::from_speffz("lopbip.loteut\n");
	CHECK(c == d);

	// Superflip
	c = cube::from_moves("U R2 F B R B2 R U2 L B2 R U' D' R2 F R' L B2 U2 F2");
	d = cube::from_speffz(".qbmcidejpntrhflkuovgxsw");
	CHECK(c == d);
	d = cube::from_speffz(".BCDKOSGJTFH");
	CHECK(c == d);

	// Supertwist
	c = cube::from_moves("B2 L R2 B2 F2 D2 U2 R' F2 D U2 B2 F2 L2 R2 U'");
	d = cube::from_speffz("bqcjdikplgtosh.");
	CHECK(c == d);
	d = cube::from_speffz("MNFPOGH.");
	CHECK(c == d);

	// Alternate edge buffer
	CHECK(cube::from_moves("F") == cube::from_speffz("pcfup.pcf", 'A', 'U'));

	// Alternate edge buffer, flipped
	CHECK(cube::from_moves("F") == cube::from_speffz("pcfup.jil", 'A', 'K'));

	// Alternate edge buffer, in-place flip
	CHECK(cube::from_speffz(".BU") == cube::from_speffz(".B", 'A', 'U'));

	// Alternate corner buffer
	CHECK(cube::from_moves("F") == cube::from_speffz("mdg.jilkj", 'V', 'A'));

	// Alternate corner buffer, twisted
	CHECK(cube::from_moves("F") == cube::from_speffz("cfu.jilkj", 'P', 'A'));

	// Alternate edge buffer, in-place twist
	CHECK(cube::from_speffz("MP.") == cube::from_speffz("M", 'V', 'A'));
}

TEST(Cube, Superflip) {
	cube c = cube::from_moves("U R2 F B R B2 R U2 L B2 R U' D' R2 F R' L B2 U2 F2");
	LONGS_EQUAL_TEXT(0,     c.getEdgePerm(), "bad edge perm");
	LONGS_EQUAL_TEXT(0,     c.getCornerPerm(), "bad corner perm");
	LONGS_EQUAL_TEXT(0x7ff, c.getEdgeOrient(), "bad edge orient");
	LONGS_EQUAL_TEXT(0,     c.getCornerOrient(), "bad corner orient");
	CHECK(c == ~c);
}

TEST(Cube, Supertwist) {
	cube c = cube::from_moves("B2 L R2 B2 F2 D2 U2 R' F2 D U2 B2 F2 L2 R2 U'");
	LONGS_EQUAL_TEXT(0,      c.getEdgePerm(), "bad edge perm");
	LONGS_EQUAL_TEXT(0,      c.getCornerPerm(), "bad corner perm");
	LONGS_EQUAL_TEXT(0,      c.getEdgeOrient(), "bad edge orient");
	LONGS_EQUAL_TEXT(0x6699, c.getCornerOrientRaw(), "bad corner orient");
	CHECK(c * c == ~c);
}

TEST(Cube, Corner4Comb) {
	cube c;

	LONGS_EQUAL(0, c.getCorner4Comb());

	for (int i = 0; i < N_C4COMB; i++) {
		c = t::random_cube();
		c.setCorner4Comb(i);
		LONGS_EQUAL(i, c.getCorner4Comb());
		LONGS_EQUAL(0, c.getCornerOrient());
		check_invariants(c);
	}

	/* Cycling the U-face or D-face corners should not affect the coordinate */
	for (int i = 0; i < 1000; i++) {
		c = t::random_cube();
		CHECK(c.getCorner4Comb() == c.premove(0).getCorner4Comb());
		CHECK(c.getCorner4Comb() == c.premove(9).getCorner4Comb());
	}

	/* setCorner4Comb should only affect corner perm+orient */
	for (int i = 0; i < 1000; i++) {
		c = t::random_cube();
		eperm_t eperm = c.getEdgePerm();
		eorient_t eorient = c.getEdgeOrient();
		c.setCorner4Comb(c.getCorner4Comb());
		LONGS_EQUAL(eperm, c.getEdgePerm());
		LONGS_EQUAL(eorient, c.getEdgeOrient());
	}
}

TEST(Cube, Edge4Comb) {
	cube c;

	LONGS_EQUAL(0, c.getEdge4Comb());

	for (int i = 0; i < N_E4COMB; i++) {
		c = t::random_cube();
		c.setEdge4Comb(i);
		LONGS_EQUAL(i, c.getEdge4Comb());
		LONGS_EQUAL(0, c.getEdgeOrient());
		/* Should produce an identity EdgeUD4Comb */
		LONGS_EQUAL(0, c.getEdgeUD4Comb());
		check_invariants(c);
	}

	/* Cycling the U-face or D-face edges should not affect the coordinate */
	for (int i = 0; i < 1000; i++) {
		c = t::random_cube();
		CHECK(c.getEdge4Comb() == c.premove(0).getEdge4Comb()); // U
		CHECK(c.getEdge4Comb() == c.premove(9).getEdge4Comb()); // D
	}

	/* Swapping the equatorial slice edges should not affect the coordinate */
	for (int i = 0; i < 1000; i++) {
		c = t::random_cube();
		CHECK(c.getEdge4Comb() == c.premove( 4).getEdge4Comb()); // R2
		CHECK(c.getEdge4Comb() == c.premove( 7).getEdge4Comb()); // F2
		CHECK(c.getEdge4Comb() == c.premove(13).getEdge4Comb()); // L2
		CHECK(c.getEdge4Comb() == c.premove(16).getEdge4Comb()); // B2
	}

	/* setEdge4Comb should only affect edge perm+orient */
	for (int i = 0; i < 1000; i++) {
		c = t::random_cube();
		cperm_t cperm = c.getCornerPerm();
		corient_t corient = c.getCornerOrient();
		c.setEdge4Comb(c.getEdge4Comb());
		LONGS_EQUAL(cperm, c.getCornerPerm());
		LONGS_EQUAL(corient, c.getCornerOrient());
	}
}

TEST(Cube, Edge4Perm) {
	cube c;

	LONGS_EQUAL(0, c.getEdge4Perm());

	for (int i = 0; i < N_E4PERM; i++) {
		c = t::random_cube();
		c.setEdge4Perm(i);
		LONGS_EQUAL(i, c.getEdge4Perm());
		LONGS_EQUAL(0, c.getEdge4Comb());
		LONGS_EQUAL(0, c.getEdgeOrient());
		check_invariants(c);
	}

	for (int i = 0; i < 1000; i++) {
		auto e4comb = t::rand(N_E4COMB);
		for (int i = 0; i < N_E4PERM; i++) {
			cube d = cube().setEdge4Perm(i) * cube().setEdge4Comb(e4comb);
			LONGS_EQUAL(i, d.getEdge4Perm());
		}
	}
}

TEST(Cube, EdgeUD4Comb) {
	cube c;

	LONGS_EQUAL(0, c.getEdgeUD4Comb());

	for (int i = 0; i < N_EUD4COMB; i++) {
		c = t::random_cube();
		c.setEdgeUD4Comb(i);
		LONGS_EQUAL(i, c.getEdgeUD4Comb());
		LONGS_EQUAL(0, c.getEdgeOrient());
		check_invariants(c);
	}

	/* Cycling the U-face or D-face edges should not affect the coordinate */
	for (int i = 0; i < 1000; i++) {
		c = t::random_cube();
		CHECK(c.getEdgeUD4Comb() == c.premove(0).getEdgeUD4Comb()); // U
		CHECK(c.getEdgeUD4Comb() == c.premove(9).getEdgeUD4Comb()); // D
	}

	/* Interleaving E-slice and U/D face edges should not affect the coordinate */
	for (int i = 0; i < N_EUD4COMB; i++) {
		cube c_e;
		c_e.setEdge4Comb(t::rand(N_E4COMB));
		c.setEdgeUD4Comb(i);
		LONGS_EQUAL(i, (c * c_e).getEdgeUD4Comb());
	}
}

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

#include "nxprune.h"

#include "test_util.h"
#include "CppUTest/TestHarness.h"

using namespace vcube;

TEST_GROUP(NxPrune) {
};

TEST(NxPrune, CCoord) {
	cube c;

	LONGS_EQUAL(0, nx::ccoord(c));

	for (c4comb_t i = 0; i < N_C4COMB; i++) {
		c = t::random_cube();
		c.setCorner4Comb(i);
		c.setCornerOrient(t::rand(N_CORIENT));
		LONGS_EQUAL(i, nx::ccoord(c) & 0xff);
	}

	for (corient_t i = 0; i < N_CORIENT; i++) {
		c = t::random_cube();
		c.setCornerOrient(i);
		LONGS_EQUAL(c.getCornerOrientRaw(), nx::ccoord(c) >> 8);
	}
}

TEST(NxPrune, CCoordRep) {
	cube c;
	uint8_t sym;

	/* Rep has the lowest coordinate */
	for (int i = 0; i < 1000; i++) {
		c = nx::ccoord::rep(t::random_cube(), sym);
		nx::ccoord best(c);
		for (int s = 0; s < 16; s++) {
			CHECK(best <= nx::ccoord(c.symConjugate(s)));
		}
	}

	/* Symmetry is set correctly */
	for (int i = 0; i < 1000; i++) {
		c = t::random_cube();
		cube rep = nx::ccoord::rep(c, sym);
		CHECK(c.symConjugate(sym) == rep);
	}
}

TEST(NxPrune, EP1) {
	cube c;

	using ecoord = nx::ecoord<nx::EP1,nx::EO4>;

	LONGS_EQUAL(2, ecoord(c));

	uint32_t prev = ecoord(c);
	for (e4comb_t i = 1; i < N_E4COMB; i++) {
		c = t::random_cube();
		c.setEdge4Comb(i);
		c.setEdgeOrient(t::rand(N_EORIENT));
		uint32_t ecomb = ecoord(c) & 0x1ff;
		CHECK(ecomb % 64 >= 2); // Gaps are in the correct places
		CHECK(prev < ecomb);    // Coordinate monotonically increases
		prev = ecomb;
	}
	LONGS_EQUAL(prev, N_E4COMB + 2 * ((N_E4COMB + 63) / 64) - 1);

	/* Fuzz test */
	for (int i = 0; i < 1000; i++) {
		cube c = t::random_cube();
		c.setEdgeOrient(0);
		LONGS_EQUAL(0, ecoord(c) >> 9);
	}
}

TEST(NxPrune, EP2) {
	cube c;

	using ecoord = nx::ecoord<nx::EP2,nx::EO4>;
	using ecoord14 = nx::ecoord<nx::EP1,nx::EO4>;

	LONGS_EQUAL(2, ecoord(c));

	/* EP1 field matches */
	for (int i = 0; i < 1000; i++) {
		cube c = t::random_cube();
		LONGS_EQUAL(ecoord14(c) & 0x1ff, ecoord(c) & 0x1ff);
	}

	/* E4Perm field matches */
	for (e4perm_t i = 0; i < N_E4PERM; i++) {
		c.setEdge4Perm(i);
		LONGS_EQUAL(i, ecoord(c) >> 13);
	}

	/* Fuzz test */
	for (int i = 0; i < 1000; i++) {
		cube c = t::random_cube();
		LONGS_EQUAL(c.getEdge4Perm(), ecoord(c) >> 13);
	}
}

TEST(NxPrune, EP3) {
	cube c;

	using ecoord = nx::ecoord<nx::EP3,nx::EO4>;
	using ecoord14 = nx::ecoord<nx::EP1,nx::EO4>;

	LONGS_EQUAL(2, ecoord(c));

	/* EP1 field matches */
	for (int i = 0; i < 1000; i++) {
		cube c = t::random_cube();
		LONGS_EQUAL(ecoord14(c) & 0x1ff, ecoord(c) & 0x1ff);
	}

	/* UD4Comb field matches */
	for (eud4comb_t i = 0; i < N_EUD4COMB; i++) {
		c.setEdgeUD4Comb(i);
		LONGS_EQUAL(i, ecoord(c) >> 13);
	}

	/* Fuzz test */
	for (int i = 0; i < 1000; i++) {
		cube c = t::random_cube();
		LONGS_EQUAL(c.getEdgeUD4Comb(), ecoord(c) >> 13);
	}
}

TEST(NxPrune, EP4) {
	cube c;

	using ecoord = nx::ecoord<nx::EP4,nx::EO4>;
	using ecoord14 = nx::ecoord<nx::EP1,nx::EO4>;

	LONGS_EQUAL(2, ecoord(c));

	/* EP1 field matches */
	for (int i = 0; i < 1000; i++) {
		cube c = t::random_cube();
		LONGS_EQUAL(ecoord14(c) & 0x1ff, ecoord(c) & 0x1ff);
	}

	/* E4Perm field matches */
	for (e4perm_t i = 0; i < N_E4PERM; i++) {
		c.setEdge4Perm(i);
		LONGS_EQUAL(i, (ecoord(c) >> 13) % N_E4PERM);
	}

	/* UD4Comb field matches */
	for (eud4comb_t i = 0; i < N_EUD4COMB; i++) {
		c.setEdgeUD4Comb(i);
		LONGS_EQUAL(i, (ecoord(c) >> 13) / N_E4PERM);
	}

	/* Fuzz test */
	for (int i = 0; i < 1000; i++) {
		cube c = t::random_cube();
		LONGS_EQUAL(c.getEdge4Perm(), (ecoord(c) >> 13) % N_E4PERM);
		LONGS_EQUAL(c.getEdgeUD4Comb(), (ecoord(c) >> 13) / N_E4PERM);
	}
}

TEST(NxPrune, EO4) {
	cube c;

	using ecoord = nx::ecoord<nx::EP3,nx::EO4>;

	LONGS_EQUAL(2, ecoord(c));

	/* Edge4Orient field matches */
	for (e4orient_t i = 0; i < N_E4ORIENT; i++) {
		c = t::random_cube();
		c.setEdge4Orient(i);
		LONGS_EQUAL(i, (ecoord(c) >> 9) & 0xf);
	}
}

TEST(NxPrune, EO8) {
	cube c;

	using ecoord = nx::ecoord<nx::EP3,nx::EO8>;

	LONGS_EQUAL(2, ecoord(c));

	/* Edge8Orient field matches */
	for (e8orient_t i = 0; i < N_E8ORIENT; i++) {
		c = t::random_cube();
		c.setEdge8Orient(i);
		LONGS_EQUAL(i, (ecoord(c) >> 9) & 0xff);
	}
}

TEST(NxPrune, EO12) {
	cube c;

	using ecoord = nx::ecoord<nx::EP3,nx::EO12>;

	LONGS_EQUAL(2, ecoord(c));

	/* EdgeOrient field matches */
	for (eorient_t i = 0; i < N_EORIENT; i++) {
		c = t::random_cube();
		c.setEdgeOrient(i);
		LONGS_EQUAL(i, (ecoord(c) >> 9) & 0x7ff);
	}
}

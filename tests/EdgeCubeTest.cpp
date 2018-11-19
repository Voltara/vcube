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

#include <cstring>
#include "test_util.h"
#include "CppUTest/TestHarness.h"

using namespace vcube;

TEST_GROUP(EdgeCube) {
	bool equivalent(const edgecube &ec, const cube &c) {
		return !memcmp(&ec, &c, sizeof(ec));
	}
};

TEST(EdgeCube, Constructor) {
	LONGS_EQUAL(sizeof(edgecube), 16);

	for (int i = 0; i < 1000; i++) {
		cube c = t::random_cube();
		CHECK(equivalent(edgecube(c), c));
	}
}

TEST(EdgeCube, Compose) {
	for (int i = 0; i < 1000; i++) {
		cube a = t::random_cube();
		cube b = t::random_cube();
		CHECK(equivalent(edgecube(a).compose(edgecube(b)), a.compose(b)));
	}
}

TEST(EdgeCube, Move) {
	for (int m = 0; m < 18; m++) {
		cube c = t::random_cube();
		CHECK(equivalent(edgecube(c).move(m), c.move(m)));
		CHECK(equivalent(edgecube(c).premove(m), c.premove(m)));
	}
}

TEST(EdgeCube, SymConjugate) {
	for (int s = 0; s < 48; s++) {
		cube c = t::random_cube();
		CHECK(equivalent(edgecube(c).symConjugate(s), c.symConjugate(s)));
	}
}

TEST(EdgeCube, GetEdgeOrient) {
	LONGS_EQUAL(0, edgecube().getEdgeOrientRaw());

	for (int i = 0; i < 1000; i++) {
		cube c = t::random_cube();
		LONGS_EQUAL(edgecube(c).getEdgeOrientRaw(), c.getEdgeOrientRaw());
	}
}

TEST(EdgeCube, SetEdgeOrient) {
	edgecube ec;
	for (int i = 0; i < 1000; i++) {
		cube c = t::random_cube();

		auto eo  = c.getEdgeOrient();
		auto e8o = c.getEdge8Orient();
		auto e4o = c.getEdge4Orient();

		c.setEdgeOrient(0);
		edgecube ec(c);

		CHECK(equivalent(ec.setEdgeOrient(eo),   c.setEdgeOrient(eo)));
		CHECK(equivalent(ec.setEdge8Orient(e8o), c.setEdge8Orient(e8o)));
		CHECK(equivalent(ec.setEdge4Orient(e4o), c.setEdge4Orient(e4o)));
	}
}

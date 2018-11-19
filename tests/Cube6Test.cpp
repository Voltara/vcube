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


#include "cube6.h"

#include "test_util.h"
#include "CppUTest/TestHarness.h"

using namespace vcube;

TEST_GROUP(Cube6) {
	bool equal(const cube6 &a, const cube6 &b) {
		for (int i = 0; i < 6; i++) {
			if (a[i] != b[i]) {
				return false;
			}
		}
		return true;
	}
};

TEST(Cube6, CubeConstructor) {
	cube c = t::random_cube();
	cube6 c6(c);

	CHECK(c6[0] == c);
	CHECK(c6[1] ==  vcube::S_URF3 * c6[0] * ~vcube::S_URF3);
	CHECK(c6[2] == ~vcube::S_URF3 * c6[0] *  vcube::S_URF3);
	CHECK(c6[3] == ~c);
	CHECK(c6[4] ==  vcube::S_URF3 * c6[3] * ~vcube::S_URF3);
	CHECK(c6[5] == ~vcube::S_URF3 * c6[3] *  vcube::S_URF3);
}

TEST(Cube6, Handedness) {
	CHECK(cube6(cube::from_moves("U "))[0] == cube::from_moves("U"));
	CHECK(cube6(cube::from_moves("R "))[1] == cube::from_moves("U"));
	CHECK(cube6(cube::from_moves("F "))[2] == cube::from_moves("U"));
	CHECK(cube6(cube::from_moves("U'"))[3] == cube::from_moves("U"));
	CHECK(cube6(cube::from_moves("R'"))[4] == cube::from_moves("U"));
	CHECK(cube6(cube::from_moves("F'"))[5] == cube::from_moves("U"));
}

TEST(Cube6, Equality) {
	// Unlikely for them to be the same
	cube c = t::random_cube(), d = t::random_cube();

	cube6 c6(c);

	CHECK(c6 == c);
	CHECK_FALSE(c6 != c);

	CHECK(c6 != d);
	CHECK_FALSE(c6 == d);
}

TEST(Cube6, Move) {
	cube c = t::random_cube();
	for (int m = 0; m < N_MOVES; m++) {
		CHECK(equal(c.move(m), cube6(c).move(m)));
		CHECK(equal(c.premove(m), cube6(c).premove(m)));
	}
}

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
#include <random>

using namespace vcube;

struct t {
    public:
	static void setup() {
		rng.seed(rng.default_seed);
	}

	static int rand(int max) {
		std::uniform_int_distribution<> dis(0, max - 1);
		return dis(rng);
	}

	static uint8_t * to_array(cube &c) {
		return reinterpret_cast<uint8_t *>(&c);
	}

	static const uint8_t * to_array(const cube &c) {
		return reinterpret_cast<const uint8_t *>(&c);
	}

	static cube random_cube() {
		cube c;
		c.setEdgePerm(rand(N_EPERM));
		c.setEdgeOrient(rand(N_EORIENT));
		c.setCornerPerm(rand(N_CPERM));
		c.setCornerOrient(rand(N_CORIENT));
		if (c.parity()) {
			std::swap(to_array(c)[0], to_array(c)[1]);
		}
		return c;
	}

    private:
	static std::mt19937 rng;
};

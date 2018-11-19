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

#include <set>
#include "nxsolve.h"

using namespace vcube;
using namespace vcube::nx;

decltype(solver_base::depth4) solver_base::depth4;

void solver_base::init() {
	std::set<cube> seen = { cube() };
	std::vector<queue_t> prev = { { cube(), 0, 0xff } };

	// Find all cubes at depth 4 (there are 43,239 of them)
	for (int depth = 0; depth < 4; depth++) {
		depth4.clear();
		for (const auto &q : prev) {
			for (int m = 0; m < N_MOVES; m++) {
				cube6 c6 = q.c6.move(m);
				if (seen.insert(c6[0]).second) {
					depth4.emplace_back(c6, (q.moves << 8) | m, m / 3);
				}
			}
		}
		prev.swap(depth4);
	}

	prev.swap(depth4);
}

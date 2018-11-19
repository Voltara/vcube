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

#include "util.h"

using namespace vcube;

void moveseq::canonicalize(moveseq_t &moves) {
	// Append a dummy move different from the final axis to ensure
	// the final moves are flushed
	if (moves.empty()) {
		return;
	}
	moves.push_back(moves.back() + 3);

	uint8_t last_axis = 0, power[2] = { 0, 0 };
	auto tail = moves.begin();
	for (auto m : moves) {
		auto axis = (m / 3) % 3;
		if (axis != last_axis) {
			for (int pole = 0; pole < 2; pole++) {
				if (power[pole] %= 4) {
					*tail++ = (last_axis * 3 + pole * 9) + (power[pole] % 4) - 1;
				}
				power[pole] = 0;
			}
			last_axis = axis;
		}
		power[m >= 9] += (m % 3) + 1;
	}

	moves.resize(tail - moves.begin());
}

std::string moveseq::to_string(const moveseq_t &moves, style_t style) {
	const char *face = "URFDLB";
	const char *power = (style == FIXED) ? "123" : " 2'";
	bool delim = (style != FIXED);

	std::string s;
	for (auto m : moves) {
		s.push_back(face[m / 3]);
		s.push_back(power[m % 3]);
		if (delim) {
			s.push_back(' ');
		}
	}

	while (!s.empty() && s.back() == ' ') {
		s.pop_back();
	}

	return s;
}

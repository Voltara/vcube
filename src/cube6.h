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

#ifndef VCUBE_CUBE6_H
#define VCUBE_CUBE6_H

#include <array>
#include "cube.h"

namespace vcube {

class cube6 {
    public:
	cube6() : ca() {
	}

	cube6(const cube &c) {
		cube ci = ~c;
		ca = {
			c,
			S_URF3  * c * S_URF3i,
			S_URF3i * c * S_URF3,
			ci,
			S_URF3  * ci * S_URF3i,
			S_URF3i * ci * S_URF3
		};
	}

	const cube & operator [] (size_t idx) const { return ca[idx]; }
	bool operator == (const cube &c) const { return ca[0] == c; }
	bool operator != (const cube &c) const { return ca[0] != c; }

	cube6 operator * (const cube6 &o) const {
		return {{
			ca[0] * o.ca[0],
			ca[1] * o.ca[1],
			ca[2] * o.ca[2],
			o.ca[3] * ca[3],
			o.ca[4] * ca[4],
			o.ca[5] * ca[5]
		}};
	}

	cube6 move(uint8_t m0) const {
		auto &m = move_sym6[m0];
		return {{
			ca[0].move(   m0),
			ca[1].move(   m[1]),
			ca[2].move(   m[2]),
			ca[3].premove(m[3]),
			ca[4].premove(m[4]),
			ca[5].premove(m[5])
		}};
	}

	cube6 premove(uint8_t m0) const {
		auto &m = move_sym6[m0];
		return {{
			ca[0].premove(m0),
			ca[1].premove(m[1]),
			ca[2].premove(m[2]),
			ca[3].move(   m[3]),
			ca[4].move(   m[4]),
			ca[5].move(   m[5])
		}};
	}

    private:
	std::array<cube, 6> ca;

	cube6(const decltype(ca) &ca)
		: ca(ca)
	{
	}
};

}

#endif

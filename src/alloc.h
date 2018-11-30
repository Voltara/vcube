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

#ifndef VCUBE_ALLOC_H
#define VCUBE_ALLOC_H

#include <cstdlib>
#include <stdint.h>

namespace vcube {

class alloc {
    public:
	template<typename T>
	static T * huge(size_t n) {
		return (T *) huge_impl(n * sizeof(T));
	}

	template<typename T>
	static T * shared(size_t n, uint32_t key, bool rdwr) {
		return (T *) shared_impl(n * sizeof(T), key, rdwr);
	}

    private:
	static void * huge_impl(size_t n);
	static void * shared_impl(size_t n, uint32_t key, bool rdwr);
};

}

#endif

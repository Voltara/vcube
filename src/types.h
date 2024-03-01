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

#ifndef VCUBE_TYPES_H
#define VCUBE_TYPES_H

#include <cstdint>

namespace vcube {

using eorient_t = uint32_t;
static constexpr eorient_t N_EORIENT = 2048;

using e8orient_t = uint32_t;
static constexpr e8orient_t N_E8ORIENT = 256;

using e4orient_t = uint32_t;
static constexpr e4orient_t N_E4ORIENT = 16;

using corient_t = uint32_t;
static constexpr corient_t N_CORIENT = 2187;

using eperm_t = uint32_t;
static constexpr eperm_t N_EPERM = 479001600;

using cperm_t = uint32_t;
static constexpr cperm_t N_CPERM = 40320;

using c4comb_t = uint32_t;
static constexpr c4comb_t N_C4COMB = 70;

using e4comb_t = uint32_t;
static constexpr e4comb_t N_E4COMB = 495;

using e4perm_t = uint32_t;
static constexpr e4perm_t N_E4PERM = 24;

using eud4comb_t = uint32_t;
static constexpr eud4comb_t N_EUD4COMB = 70;

static constexpr int N_MOVES = 18;

}

#endif

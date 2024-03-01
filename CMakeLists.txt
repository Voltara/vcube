# This file is part of vcube.
#
# Copyright (C) 2018 Andrew Skalski
#
# vcube is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# vcube is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with vcube.  If not, see <https://www.gnu.org/licenses/>.

cmake_minimum_required (VERSION 3.5)

project(vcube)

# Need -std=c++17 for proper alignment of AVX variables in std::vector
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mlzcnt -mbmi -mbmi2 -mavx2 -flax-vector-conversions -std=c++17")

# The Clang compiler currently produces faster code than GCC for this project
set(CMAKE_C_COMPILER "clang" CACHE STRING "clang compiler" FORCE)
set(CMAKE_CXX_COMPILER "clang++" CACHE STRING "clang++ compiler" FORCE)

add_executable(gen-const-cubes
	src/gen-const-cubes.cpp
	)

add_library(vcube
	src/alloc.cpp
	src/cube.cpp
	src/nxprune.cpp
	src/nxsolve.cpp
	src/util.cpp
	)

add_executable(vc-optimal
	src/vc-optimal.cpp
	)
target_link_libraries(vc-optimal vcube pthread)

add_subdirectory(tests)

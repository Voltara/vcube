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

if(DEFINED ENV{CPPUTEST_HOME})
	message(STATUS "Using CppUTest found in $ENV{CPPUTEST_HOME}")
	set(CPPUTEST_INCLUDE_DIRS $ENV{CPPUTEST_HOME}/include)
	set(CPPUTEST_LIBRARIES $ENV{CPPUTEST_HOME}/lib)
	set(CPPUTEST_LDFLAGS CppUTest CppUTestExt)
else()
	find_package(PkgConfig REQUIRED)
	pkg_search_module(CPPUTEST REQUIRED cpputest>=3.8)
	message(STATUS "Found CppUTest version ${CPPUTEST_VERSION}")
endif()

include_directories(${CPPUTEST_INCLUDE_DIRS} ../src/)
link_directories(${CPPUTEST_LIBRARIES})

add_executable(check
	RunAllTests.cpp
	CubeTest.cpp
	Cube6Test.cpp
	EdgeCubeTest.cpp
	NxPruneTest.cpp
	)
target_link_libraries(check vcube ${CPPUTEST_LDFLAGS})
add_custom_command(TARGET check COMMAND ./check POST_BUILD)

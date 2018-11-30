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

#include <algorithm>
#include <cstdio>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "nxprune.h"
#include "alloc.h"

using namespace vcube::nx;

prune_base::prune_base(size_t stride) : os_unique(), index(), stride(stride) {
	os_unique_t os_tmp;
	auto os_next = os_unique.begin();
	decltype(index_t::base) next_symcoord = 0;
	uint8_t _ignore_;

	for (corient_t corient = 0; corient < N_CORIENT; corient++) {
		// Find the representative corient by symmetry
		cube c;
		c.setCornerOrient(corient);
		auto corient_s = ccoord::rep(c, _ignore_).getCornerOrient();

		// If this is a new representative, set base to a new coordinate;
		// otherwise copy base from the representative
		auto &idx = index[corient], &idx_s = index[corient_s];
		idx.base = (corient == corient_s) ? next_symcoord : idx_s.base;

		// Fill in the offsets and symmetries
		decltype(offset_sym_t::offset) offset = 0;
		for (c4comb_t c4comb = 0; c4comb < N_C4COMB; c4comb++) {
			c.setCorner4Comb(c4comb);
			c.setCornerOrient(corient);

			auto &os = os_tmp[c4comb];
			auto c4comb_s = ccoord::rep(c, os.sym).getCorner4Comb();
			auto &os_s = idx_s.os[c4comb_s];

			if (corient != corient_s) {
				os.offset = os_s.offset;
			} else if (c4comb != c4comb_s) {
				os.offset = os_tmp[c4comb_s].offset;
			} else {
				os.offset = offset++;
			}
		}

		next_symcoord += offset;

		// Deduplicate
		auto os_found = std::find(os_unique.begin(), os_next, os_tmp);
		if (os_found == os_next) {
			*os_next++ = os_tmp;
		}

		idx.os = &(*os_found)[0];
	}
}

void prune_base::setPrune(decltype(index_t::prune) p) {
	for (auto &idx : index) {
		idx.prune = p + idx.base * stride;
	}
}

bool prune_base::save(const std::string &filename) const {
	auto dir = filename;
	(void) mkdir(dirname(dir.data()), 0777);

	auto tmpname = filename + ".tmp";
	FILE *fp = fopen(tmpname.c_str(), "w");
	if (!fp) {
		return false;
	}
	size_t n = fwrite(index[0].prune, stride, N_CORNER_SYM, fp);
	if (fclose(fp)) {
		return false;
	}
	if (n != N_CORNER_SYM) {
		return false;
	}
	return rename(tmpname.c_str(), filename.c_str()) == 0;
}

bool prune_base::load(const std::string &filename) {
	size_t sz = stride * N_CORNER_SYM;

	FILE *fp = fopen(filename.c_str(), "r");
	if (!fp) {
		return false;
	}

	auto mem = alloc::huge<uint8_t>(sz);
	if (!mem) {
		fclose(fp);
		return false;
	}

	size_t n = fread(mem, stride, N_CORNER_SYM, fp);
	if (fclose(fp)) {
		return false;
	}

	if (n != N_CORNER_SYM) {
		return false;
	}

	setPrune(mem);

	return true;
}

bool prune_base::loadShared(uint32_t key, const std::string &filename) {
	size_t sz = stride * N_CORNER_SYM;

	auto mem = alloc::shared<uint8_t>(sz, key, false);
	if (!mem) {
		if (filename.empty()) {
			return false;
		}

		mem = alloc::shared<uint8_t>(sz, key, true);
		if (!mem) {
			return false;
		}

		FILE *fp = fopen(filename.c_str(), "r");
		if (!fp) {
			return false;
		}

		size_t n = fread(mem, stride, N_CORNER_SYM, fp);
		if (fclose(fp)) {
			return false;
		}

		if (n != N_CORNER_SYM) {
			return false;
		}
	}

	setPrune(mem);

	return true;
}

std::vector<vcube::cube> prune_base::getCornerRepresentatives() const {
	std::vector<cube> cv;
	for (corient_t corient = 0; corient < N_CORIENT; corient++) {
		auto &idx = index[corient];
		for (c4comb_t c4comb = 0; c4comb < N_C4COMB; c4comb++) {
			auto &os = idx.os[c4comb];
			if (os.sym == 0) {
				cube c;
				c.setCorner4Comb(c4comb);
				c.setCornerOrient(corient);
				cv.push_back(c);
			}
		}
	}
	return cv;
}

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
#include <vector>
#include <utility>

/* The nxprune table is an inconsistent heuristic, so Bidirectional PathMax
 * can offer additional pruning opportunities.  The reduction in node
 * expansions is very small, however.
 */
#define VCUBE_NX_USE_BPMX 1

namespace vcube::nx {

class solver_base {
    protected:
	struct queue_t {
		cube6 c6;
		uint32_t moves;
		uint8_t last_face;

		queue_t() : c6(), moves(), last_face() {
		}

		queue_t(const cube6 &c6, uint32_t moves, uint8_t last_face) :
			c6(c6), moves(moves), last_face(last_face)
		{
		}
	};

	// List of all cubes at depth=4
	static std::vector<queue_t> depth4;

    public:
	static void init();
};

template<typename prune_t>
class solver : public solver_base {
	uint64_t n_expands;
	uint8_t moves[20], *movep;
	prune_t &P;

	// Lookup table for expanding a 3-bit axis mask into a move list
	static constexpr uint32_t axis_mask_expand[] = {
		0777777, 0770770, 0707707, 0700700,
		0077077, 0070070, 0007007, 0000000 };

	// Lookup table for the list of canonical next moves following a
	// move of a given face.  The value 6 denotes no previous move
	static constexpr uint32_t last_face_mask[] = {
		0777770, 0777707, 0777077, 0770770, 0707707, 0077077, 0777777 };
	static constexpr uint8_t NO_FACE = 6;

    public:
	solver(prune_t &P) : P(P), n_expands(), moves(), movep(moves) {
	}

	auto solve(const cube6 &c6, int limit = 20) {
		movep = moves;
		n_expands = 0;

		uint8_t len = 0xff;
		auto limit1 = std::min(limit, prune_t::BASE + 4);
		for (int d = P.initial_depth(c6); d <= limit1; d++) {
			if (!search(c6, d, NO_FACE, NO_FACE, 0xff, 0)) {
				len = d;
				break;
			}
		}

		if (len == 0xff) {
			len = queue_search(c6, prune_t::BASE + 5, limit);
			if (len == 0xff) {
				len = 0;
			}
		}

		return get_moves(len);
	}

	/* Returns the cost of the previous solve */
	uint64_t cost() const {
		return n_expands;
	}

    private:
	uint8_t search(const cube6 &c6, uint8_t max_depth, uint8_t last_face, uint8_t last_face_r, int skip, int val) {
		if (max_depth == 0) {
			return c6 != cube();
		}

		uint32_t prune_vals;
		uint8_t axis_mask;
		uint8_t prune = P.lookup(c6, max_depth, prune_vals, skip, val, axis_mask);
		if (prune > max_depth) {
			return prune;
		}
		max_depth--;

		n_expands++;

		auto mask_f = axis_mask_expand[axis_mask >> 3] & last_face_mask[last_face];
		auto mask_r = axis_mask_expand[axis_mask  & 7] & last_face_mask[last_face_r];

		// Choose direction with the smaller branching factor
		int dir = _popcnt32(mask_r) - _popcnt32(mask_f);
		if (dir == 0) {
			// Tiebreaker, direction with the larger sum of pruning values
			int32_t sum =
				((prune_vals >> 8) & 0xf00f) +
				((prune_vals >> 4) & 0xf00f) +
				((prune_vals >> 0) & 0xf00f);
			dir = (sum & 0xfff) - (sum >> 12);
		}

		if (dir > 0) {
			// Forward
			while (mask_f) {
				uint8_t m = _tzcnt_u32(mask_f);
				mask_f = _blsr_u32(mask_f);

				uint8_t face = _popcnt32(011111 << m >> 15);
				uint8_t axis = (face + (face > 2)) & 3;

				// preserve one of the inverse cube pruning values
				skip = axis + 3;
				val = (prune_vals >> (4 * skip)) & 0xf;
				auto sol = search(c6.move(m), max_depth, face, last_face_r, skip, val);
#if VCUBE_NX_USE_BPMX
				if (sol > max_depth + 2) {
					return sol - 1;
				} else if (sol == max_depth + 2) {
#else
				if (sol > max_depth + 1) {
#endif
					mask_f &= ~7L << (3 * face);
				} else if (!sol) {
					*movep++ = m;
					return 0;
				}
			}
		} else {
			// Reverse (pre-move)
			while (mask_r) {
				uint8_t m = _tzcnt_u32(mask_r);
				mask_r = _blsr_u32(mask_r);

				uint8_t face = _popcnt32(011111 << m >> 15);
				uint8_t axis = (face + (face > 2)) & 3;

				// preserve one of the forward cube pruning values
				skip = axis;
				val = (prune_vals >> (4 * skip)) & 0xf;
				auto sol = search(c6.premove(m), max_depth, last_face, face, skip, val);
#if VCUBE_NX_USE_BPMX
				if (sol > max_depth + 2) {
					return sol - 1;
				} else if (sol == max_depth + 2) {
#else
				if (sol > max_depth + 1) {
#endif
					mask_r &= ~7L << (3 * face);
				} else if (!sol) {
					*movep++ = 0x80 | m;
					return 0;
				}
			}
		}

		return prune + !prune;
	}

	uint8_t queue_search(const cube6 &c6, uint8_t depth, int limit) {
		std::vector<queue_t> queue;

		for (const auto &q : depth4) {
			queue.emplace_back(c6 * q.c6, q.moves, q.last_face);
		}

		struct order_t {
			uint16_t idx;
			uint16_t density;
			order_t() : idx(), density() {
			}
			order_t(uint16_t idx, uint16_t density) : idx(idx), density(density) {
			}
		};
		std::vector<order_t> order, order_new;
		for (int i = 0; i < queue.size(); i++) {
			order.emplace_back(i, 0);
		}
		order_new.resize(order.size());

		// Histograms for 2-pass radix sort
		std::array<uint16_t, 256> hist0, hist1;

		for (auto d = depth; d <= limit; d++) {
			hist0.fill(0);
			hist1.fill(0);
			for (auto &o : order) {
				auto &q = queue[o.idx];
				auto old_cost = cost();
				auto prune = search(q.c6, d - 4, q.last_face, NO_FACE, -1, 0);
				if (!prune) {
					auto moves = q.moves;
					for (int i = 0; i < 4; i++) {
						*movep++ = q.moves;
						q.moves >>= 8;
					}
					return d;
				}

				// Search next level in order of decreasing density
				//   58206:47525 approximates sqrt(3):sqrt(2) which is the
				//   ratio of canonical sequences starting with URF vs DLB
				constexpr uint64_t ratio[] = { 58206, 47525 };
				float density = (cost() - old_cost) * ratio[q.last_face < 3];
				uint32_t u_density = *(uint32_t *) &density;
				o.density = ~(u_density >> 15);
				hist0[o.density & 0xff]++;
				hist1[o.density >> 8]++;
			}

			uint16_t sum0 = 0, sum1 = 0;
			for (int i = 0; i < 256; i++) {
				std::swap(sum0, hist0[i]);
				sum0 += hist0[i];
				std::swap(sum1, hist1[i]);
				sum1 += hist1[i];
			}
			for (auto &o : order) {
				order_new[hist0[o.density & 0xff]++] = o;
			}
			for (auto &o : order_new) {
				order[hist1[o.density >> 8]++] = o;
			}
		}

		return 0xff;
	}

	auto get_moves(uint8_t len) const {
		std::vector<uint8_t> m(len);
		auto mi_f = m.begin();
		auto mi_r = m.rbegin();
		for (int i = len - 1; i >= 0; i--) {
			if (moves[i] & 0x80) {
				*mi_r++ = moves[i] ^ 0x80;
			} else {
				*mi_f++ = moves[i];
			}
		}
		return m;
	}
};

}

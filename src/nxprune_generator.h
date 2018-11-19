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
#include <cstdio>
#include <cstring>
#include <chrono>
#include <thread>
#include <mutex>
#include "nxprune.h"
#include "alloc.h"

namespace vcube::nx {

template<typename Prune>
class prune_generator {
	struct neighbor_t {
		uint16_t first, second;
		mutable uint32_t moves, moves_inv;

		neighbor_t(uint16_t first = 0, uint16_t second = 0)
			: first(first), second(second), moves(), moves_inv()
		{
		}

		bool operator < (const neighbor_t &o) const {
			return (first == o.first) ? second < o.second : first < o.first;
		}
	};

    public:
	prune_generator(Prune &P, int n_threads) :
		P(P), edge_rep(), corner_prune(), n_threads(), mod3_next_xor(), mod3_mask(), depth_xor()
	{
		this->n_threads = std::max(1, n_threads);
		edge_rep.init<typename Prune::ecoord>();
	}

	void generate() {
		// Allocate the pruning table and initialize all to unvisited
		auto mem = alloc::huge<uint8_t>(16 * Prune::N_EDGE_STRIPE * N_CORNER_SYM);
		memset(mem, 0xff, 16 * Prune::N_EDGE_STRIPE * N_CORNER_SYM);
		P.init(mem);

		// Neighbor tables
		auto corner_rep = P.getCornerRepresentatives();
		auto neighbors = getNeighbors(corner_rep);

		std::vector<__m128i *> prune_row;
		for (uint32_t i = 0; i < N_CORNER_SYM; i++) {
			prune_row.push_back(reinterpret_cast<__m128i*>(P.getPruneRow(i)));
		}

		// Set the identity cube depth to zero
		mem[0] = 0xc0;
		uint64_t found = 1;

		for (int depth = 0; depth <= Prune::BASE + 1; depth++) {
			// Upon reaching the pruning table base value, zero
			// all visited positions.  This will leave two distinct
			// values in the table, 0 (visited), and 3 (unvisited).
			// The final two passes will fill in the 1 and 2 values.
			if (depth == Prune::BASE) {
				zero_visited(mem);
			}

			auto prev_found = found;

			// The initial passes set the values to (depth % 3); the
			// final two passes set them to (depth - Prune::BASE)
			uint8_t mod3 = (depth < Prune::BASE) ? (depth % 3) : (depth - Prune::BASE);

			// Values in the table are set by xoring with the existing
			// unvisited value, which is known to be 3
			mod3_next_xor = ((mod3 + 1) % 3) ^ 3;

			// Similarly, the stripe-min values are set by xoring
			// with the known unvisited value of 0xf
			depth_xor = (depth + 1) ^ 0xf;

			// This mask is used to quickly find values matching
			// the current frontier
			mod3 |= mod3 << 2;
			mod3 |= mod3 << 4;
			mod3_mask = _mm_set1_epi8(~mod3);

			auto t0 = std::chrono::steady_clock::now();

			std::mutex mtx;
			std::vector<std::thread> workers;
			std::vector<bool> busy(N_CORNER_SYM), done(neighbors.size());
			for (int i = 0; i < n_threads; i++) {
				workers.push_back(std::thread([&]() {
					mtx.lock();
					bool ok;
					do {
						ok = false;
						size_t idx = 0;

						// Scan for and process unfinished neighbor pairs that are not
						// currently being processed by another thread
						for (auto &n : neighbors) {
							if (done[idx] || busy[n.first] || busy[n.second]) {
								idx++;
								continue;
							}
							ok = done[idx++] = busy[n.first] = busy[n.second] = true;
							mtx.unlock();

							uint64_t this_found = 0;
							auto c = corner_rep[n.first];
							auto v_src = prune_row[n.first], v_dst = prune_row[n.second];

							// Try all moves that transition from one neighbor to the other
							for (auto moves = n.moves; ; moves = n.moves_inv) {
								while (moves) {
									int m = _tzcnt_u32(moves);
									moves = _blsr_u32(moves);

									// Try all self-symmetries of the goal coordinate
									auto c_m = c.move(m);
									auto goalc = ccoord(c_m.symConjugate(P.get_sym(c_m)));
									for (uint8_t sym = 0; sym < 16; sym++) {
										if (ccoord(c_m.symConjugate(sym)) == goalc) {
											this_found += generateCornerPair(v_src, v_dst, m, sym);
										}
									}
								}

								if (v_src < v_dst) {
									c = corner_rep[n.second];
									std::swap(v_src, v_dst);
								} else {
									break;
								}
							}

							mtx.lock();
							found += this_found;
							busy[n.first] = busy[n.second] = false;
						}
					} while (ok);
					mtx.unlock();
				}));
			}

			for (auto &t : workers) {
				t.join();
			}

			std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - t0;

			fprintf(stderr, "depth=%u found=%lu (%.06f)\n", depth + 1, found - prev_found, elapsed.count());
		}
	}

    private:
	Prune &P;
	ecoord_rep edge_rep;
	std::vector<uint8_t *> corner_prune;
	int n_threads;
	uint8_t mod3_next_xor;
	__m128i mod3_mask;
	uint8_t depth_xor;

	auto getNeighbors(const std::vector<cube> &corner_rep) {
		std::set<neighbor_t> nset;
		for (size_t idx0 = 0; idx0 < corner_rep.size(); idx0++) {
			for (int m = 0; m < N_MOVES; m++) {
				cube c = corner_rep[idx0].move(m);
				auto idx1 = P.sym_coord(c);
				if (idx0 <= idx1) {
					nset.emplace(idx0, idx1).first->moves |= 1 << m;
				} else {
					nset.emplace(idx1, idx0).first->moves_inv |= 1 << m;
				}
			}
		}
		return std::vector<neighbor_t>(nset.begin(), nset.end());
	}

	void zero_visited(uint8_t *mem) {
		/* Zero all visited nodes in the table, but preserve stripe-min field
		 * This will leave two distinct values in the table:
		 *     0: Visited; depth <= base
		 *     3: Unvisited; depth > base
		 * This prepares for the final two passes which fill in the "1" and "2" values
		 */
		auto end = (__m128i *) mem + Prune::N_EDGE_STRIPE * N_CORNER_SYM;
		for (auto v = (__m128i *) mem; v != end; v++) {
			auto &s = *v;
			// save the stripe-min field
			auto min = _mm_and_si128(s, _mm_set_epi64x(0, 0xf));
			// zero out all non 3 values
			s = _mm_and_si128(s, _mm_srli_epi64(s, 1));
			s = _mm_and_si128(s, _mm_set1_epi8(0x55));
			s = _mm_or_si128(s, _mm_slli_epi64(s, 1));
			// restore the stripe-min field
			s = _mm_or_si128(s, min);
		}
	}

	uint64_t generateCornerPair(__m128i *src, __m128i *dst, uint8_t m, uint8_t sym) {
		uint64_t found = 0;
		uint32_t stripe_idx = 0;
		for (auto src_end = src + Prune::N_EDGE_STRIPE; src != src_end; src++, stripe_idx++) {
			// Skip untouched stripe (stripe-min is 0xf)
			if ((_mm_extract_epi8(*src, 0) & 0xf) == 0xf) {
				continue;
			}

			auto [ high, low, eo ] = Prune::ecoord::decode(stripe_idx << 6);

			auto cmp = _mm_xor_si128(*src, mod3_mask);
			cmp = _mm_and_si128(cmp, _mm_srli_epi64(cmp, 1));
			uint64_t bits =
				_pext_u64(_mm_extract_epi64(cmp, 1), 0x5555555555555555) << 32 |
				_pext_u64(_mm_extract_epi64(cmp, 0), 0x5555555555555555);
			bits &= (low == 448) ? 0x7ffffffffffffffc : 0xfffffffffffffffc;
			while (bits) {
				int b = _tzcnt_u64(bits);
				bits = _blsr_u64(bits);

				edgecube rep = edge_rep.get<typename Prune::ecoord>(high, low + b, eo);

				typename Prune::ecoord coord(rep.move(m), sym);

				auto s_d = (uint8_t *) &dst[coord / 64];
				auto &s_octet = s_d[(coord / 4) % 16];
				auto s_shift = (coord % 4) * 2;
				if (((s_octet >> s_shift) & 3) == 3) {
					s_octet ^= mod3_next_xor << s_shift;
					// set stripe-min if not already set
					if ((s_d[0] & 0xf) == 0xf) {
						s_d[0] ^= depth_xor;
					}
					found++;
				}
			}
		}

		return found;
	}
};

}

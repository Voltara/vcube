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

#include <cstring>
#include <cctype>
#include "cube.h"

using namespace vcube;

// Parse a move sequence; supports multiple formats such as "U R2 F'", "U1R2F3", "URRFFF"
cube cube::from_moves(const std::string &s) {
	cube c;

	int face = -1;
	for (auto ch : s) {
		int f = -1, power = -1;
		switch (ch) {
		    case 'u': case 'U': f =  0; break;
		    case 'r': case 'R': f =  3; break;
		    case 'f': case 'F': f =  6; break;
		    case 'd': case 'D': f =  9; break;
		    case 'l': case 'L': f = 12; break;
		    case 'b': case 'B': f = 15; break;
		    case '3': case '\'': power = 2; break;
		    case '2': power = 1; break;
		    case '1': power = 0; break;
		    default: 
			      // End move on whitespace or unknown character
			      power = 0;
		}

		if (f != -1) {
			// Flush previous move if two faces in a row "UR"
			if (face != -1) {
				c = c.move(face);
			}
			face = f;
		}

		if (power != -1) {
			if (face != -1) {
				c = c.move(face + power);
				face = -1;
			}
			power = -1;
		}
	}

	// Handle face at end of string "....U"
	if (face != -1) {
		c = c.move(face);
	}

	return c;
}

// Vector of numeric moves
cube cube::from_movev(const std::vector<uint8_t> &v) {
	cube c;
	for (auto m : v) {
		c = c.move(m);
	}
	return c;
}

// Parse a cube position in the notation used by Michael Reid's solver,
// which has the identity:
//   UF UR UB UL DF DR DB DL FR FL BR BL UFR URB UBL ULF DRF DFL DLB DBR
cube cube::from_reid(std::string s) {
	uint64_t e_map = 0xfab9867452301, c_map = 0xf76541230;

	constexpr char c_lookup[] = "UFRUF   ULFUL   UBLUB   URBUR   DRFDR   DFLDF   DLBDL   DBRDB";
	constexpr char e_lookup[] = "URU UFU ULU UBU DRD DFD DLD DBD FRF FLF BLB BRB";

	cube c;
	uint8_t *edge   = reinterpret_cast<uint8_t*>(&c.ev());
	uint8_t *corner = reinterpret_cast<uint8_t*>(&c.cv());

	s.c_str();
	for (auto &ch : s) {
		ch = toupper(ch);
	}

	uint32_t edges_todo = 0xfff, corners_todo = 0xff;
	uint8_t eorient_sum = 0, corient_sum = 0;

	const char *ws = " \t\r\n";
	for (char *saveptr, *tok = strtok_r(&s[0], ws, &saveptr); tok; tok = strtok_r(NULL, ws, &saveptr)) {
		int len = strlen(tok);
		char *p;
		switch (strlen(tok)) {
		    case 2:
			if ((p = strstr(e_lookup, tok))) {
				int offset = p - e_lookup;
				edge[e_map & 0xf] = (offset >> 2) | ((offset << 4) & 0x10);
				e_map >>= 4;
				edges_todo ^= 1 << (offset >> 2);
				eorient_sum += offset & 1;
			}
			break;
		    case 3:
			if ((p = strstr(c_lookup, tok))) {
				int offset = p - c_lookup;
				corner[c_map & 0xf] = (offset >> 3) | ((offset << 4) & 0x30);
				c_map >>= 4;
				corners_todo ^= 1 << (offset >> 3);
				corient_sum += offset & 3;
			}
			break;
		}
	}

	// There must be exactly one of each edge and corner cubie
	if (edges_todo || corners_todo || e_map != 0xf || c_map != 0xf) {
		return cube();
	}

	// Check for valid orientation
	if ((eorient_sum & 1) || (corient_sum % 3)) {
		return cube();
	}

	// Check for legal parity
	if (c.parity()) {
		return cube();
	}

	return c;
}

// Parse a cube position in Speffz lettering, corners first
cube cube::from_speffz(const std::string &s, int corner_buffer, int edge_buffer) {
	constexpr uint8_t c_map[] = {
		2, 3, 0, 1, 2, 1, 5, 6, 1, 0, 4, 5, 0, 3, 7, 4, 3, 2, 6, 7, 5, 4, 7, 6 };
	constexpr uint8_t e_map[] = {
		3, 0, 1, 2, 2, 9, 6, 10, 1, 8, 5, 9, 0, 11, 4, 8, 3, 10, 7, 11, 5, 4, 7, 6 };
	constexpr uint8_t c_ori[] = {
		0, 0, 0, 0, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 0, 0, 0, 0 };
	constexpr uint8_t e_ori[] = {
		0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0 };

	corner_buffer = toupper(corner_buffer) - 'A';
	if (corner_buffer < 0 || corner_buffer >= 24) {
		corner_buffer = 0;
	}
	uint8_t corner_buffer_ori = c_ori[corner_buffer];
	corner_buffer = c_map[corner_buffer];

	edge_buffer = toupper(edge_buffer) - 'A';
	if (edge_buffer < 0 || edge_buffer >= 24) {
		edge_buffer = 0;
	}
	uint8_t edge_buffer_ori = e_ori[edge_buffer];
	edge_buffer = e_map[edge_buffer];

	cube c;
	uint8_t *edge   = reinterpret_cast<uint8_t*>(&c.ev());
	uint8_t *corner = reinterpret_cast<uint8_t*>(&c.cv());

	bool parity_ok = true;

	bool parse_edges = false;
	for (auto ch : s) {
		if (ch == '.') {
			parse_edges = true;
		} else if (ch >= 'A' && ch <= 'X') {
			// Flip/twist in place
			if (parse_edges) {
				uint8_t idx = e_map[ch - 'A'];
				edge[edge_buffer] ^= 0x10;
				edge[idx] ^= 0x10;
			} else {
				uint8_t idx = c_map[ch - 'A'];
				uint8_t ori = c_ori[ch - 'A'] << 4;
				corner[corner_buffer] = (corner[corner_buffer] + ori) % 0x30;
				corner[idx] = (0x30 + corner[idx] - ori) % 0x30;
			}
		} else if (ch >= 'a' && ch <= 'x') {
			// Cycle with buffer
			if (parse_edges) {
				uint8_t idx = e_map[ch - 'a'];
				uint8_t ori = (e_ori[ch - 'a'] ^ edge_buffer_ori) << 4;
				std::swap(edge[edge_buffer], edge[idx]);
				edge[edge_buffer] ^= ori;
				edge[idx] ^= ori;
				parity_ok ^= (idx != edge_buffer);
			} else {
				uint8_t idx = c_map[ch - 'a'];
				int8_t ori = (c_ori[ch - 'a'] - corner_buffer_ori) << 4;
				std::swap(corner[corner_buffer], corner[idx]);
				corner[idx] = (0x30 + corner[idx] + ori) % 0x30;
				corner[corner_buffer] = (0x30 + corner[corner_buffer] - ori) % 0x30;
				parity_ok ^= (idx != corner_buffer);
			}
		}
	}

	return parity_ok ? ~c : cube();
}

cube & cube::setCornerOrient(corient_t corient) {
	/* The scalar and vector versions are close in performance,
	 * but the vector version is currently faster (tested on i7-7700K)
	 */
#if 0
	uint64_t tmp, co;

	tmp = (corient / 729);     co = tmp;
	tmp = (corient / 243) % 3; co = (co << 8) | tmp;
	tmp = (corient /  81) % 3; co = (co << 8) | tmp;
	tmp = (corient /  27) % 3; co = (co << 8) | tmp;
	tmp = (corient /   9) % 3; co = (co << 8) | tmp;
	tmp = (corient /   3) % 3; co = (co << 8) | tmp;
	tmp = (corient      ) % 3; co = (co << 8) | tmp;

	uint64_t sum = co;
	sum += sum >> 32;
	sum += sum >> 16;
	sum += sum >> 8;

	static const uint64_t mod3 = 0x9249249249249244;
	uint64_t last_corner = ((mod3 >> sum) | (mod3 << (64 - sum))) & (3LL << 4);
	co = (co << 12) | last_corner;

	// Replace old corner orientation
	u64()[2] = (u64()[2] & 0x0f0f0f0f0f0f0f0f) | co;
#else
	// Replace old corner orientation
	u64()[2] = (u64()[2] & 0x0f0f0f0f0f0f0f0f) | avx2::unrank_corner_orient(corient);
#endif

	return *this;
}

cube & cube::setCorner4Comb(c4comb_t c4comb) {
	auto mask = unrank_8C4(c4comb);

	// Set all D-face corners to 4, and U-face corners to 0
	uint64_t corners = _pdep_u64(mask, 0x0404040404040404);

	// Create a mask to fill in the low bits (+0 +1 +2 +3)
	uint64_t fill_mask = (corners >> 1) | (corners >> 2);

	// Fill in the D-face and U-face low bits
	uint64_t fill =
		_pdep_u64(0xe4, fill_mask) |
		_pdep_u64(0xe4, fill_mask ^ 0x0303030303030303);

	u64()[2] = corners | fill;

	return *this;
}

cube & cube::setEdgeUD4Comb(eud4comb_t eud4comb) {
	auto mask = unrank_8C4(eud4comb);

	// Set all D-face edges to 4, and U-face edges to 0
	uint64_t edges = _pdep_u64(mask, 0x0404040404040404);

	// Create a mask to fill in the low bits (+0 +1 +2 +3)
	uint64_t fill_mask = (edges >> 1) | (edges >> 2);

	// Fill in the D-face and U-face low bits
	uint64_t fill =
		_pdep_u64(0xe4, fill_mask) |
		_pdep_u64(0xe4, fill_mask ^ 0x0303030303030303);

	u64()[0] = edges | fill;
	u64()[1] = 0x0f0e0d0c0b0a0908;

	return *this;
}

cube & cube::setEdge4Comb(e4comb_t e4comb) {
	auto mask = unrank_12C4(e4comb);

	uint64_t edges = _pdep_u64(mask, 0x888888888888);

	uint64_t fill_mask = (edges >> 1) | (edges >> 2) | (edges >> 3);

	uint64_t fill =
		_pdep_u64(076543210, fill_mask) |
		_pdep_u64(076543210, fill_mask ^ 0x777777777777);

	edges |= fill;

	u64()[0] = _pdep_u64(edges, 0x0f0f0f0f0f0f0f0f);
	u64()[1] = _pdep_u64(edges >> 32, 0x0f0f0f0f0f0f0f0f) | 0x0f0e0d0c00000000;

	return *this;
}

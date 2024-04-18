#include "util.h"

#include "CppUTest/TestHarness.h"

using namespace vcube;

TEST_GROUP(MoveSeq) {
	void check_parse(const std::string &s, const moveseq_t &expected) {
		CHECK(moveseq_t::parse(s) == expected);
	}
};

TEST(MoveSeq, Parse) {
	check_parse("", {});
	check_parse("URFDLB", { 0, 3, 6, 9, 12, 15 });
	check_parse("U1R1F1D1L1B1", { 0, 3, 6, 9, 12, 15 });
	check_parse("U2R2F2D2L2B2", { 1, 4, 7, 10, 13, 16 });
	check_parse("U'R'F'D'L'B'", { 2, 5, 8, 11, 14, 17 });
	check_parse("UUURRRFFF", { 0, 0, 0, 3, 3, 3, 6, 6, 6 });
}

TEST(MoveSeq, ParseLowercase) {
	check_parse("urfdlb", { 0, 3, 6, 9, 12, 15 });
	check_parse("u1r1f1d1l1b1", { 0, 3, 6, 9, 12, 15 });
	check_parse("u2r2f2d2l2b2", { 1, 4, 7, 10, 13, 16 });
	check_parse("u'r'f'd'l'b'", { 2, 5, 8, 11, 14, 17 });
}

TEST(MoveSeq, ParseDelimited) {
	check_parse(" U2?R1,XF2\tD' L   B ", { 1, 3, 7, 11, 12, 15 });
}

TEST(MoveSeq, ParseMalformedInput) {
	check_parse("U2 U 2", { 1, 0 });
	check_parse("U321", { 2 });
	check_parse("1", {});
	check_parse("2", {});
	check_parse("3", {});
	check_parse("'", {});
}

// g++ -std=c++17 -Wall -Wextra -fsanitize=address,undefined lz_tests.cpp -o lz_tests && ./lz_tests
#include <iostream>
#include "lz_encoder.hpp"

static int checks = 0, failures = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++failures; std::cerr << "FAIL line " << __LINE__ << ": " #c "\n"; } } while (0)

int main() {
    // Empty input: no tokens, just the 32-bit length header.
    CHECK(lzTokens("").empty());
    CHECK(lzEncodeBits("").size() == 32);

    // ABABABA -> A | B | AB | ABA
    auto t = lzTokens("ABABABA");
    CHECK(t.size() == 4);
    CHECK((t[0] == LZToken{0, 'A'}));
    CHECK((t[1] == LZToken{0, 'B'}));
    CHECK((t[2] == LZToken{1, 'B'}));
    CHECK((t[3] == LZToken{3, 'A'}));
    // 32 header + (0+8) + (1+8) + (2+8) + (2+8) bits
    CHECK(lzEncodeBits("ABABABA").size() == 69);

    // Input ending mid-phrase: A | (A,pad)
    auto e = lzTokens("AA");
    CHECK(e.size() == 2);
    CHECK((e[1] == LZToken{1, 0}));

    // Exact bit contents for "A": header=1, token (0 bits prefix, 0x41)
    CHECK(lzEncodeBits("A") == std::string(31, '0') + "1" + "01000001");

    // Packing
    CHECK(packBits("1") == std::string(1, static_cast<char>(0x80)));
    CHECK(packBits("101010101").size() == 2);
    CHECK(lzCompress("A").size() == 5);

    // Repetitive data compresses; every phrase is distinct for random-ish data.
    std::string rep(10000, 'a');
    CHECK(lzCompress(rep).size() < 1000);
    CHECK(lzTokens(rep).size() < 200);

    // Binary-safe (NUL bytes and high bytes).
    std::string bin = std::string("\0\xff\0\xff", 4);
    CHECK(lzTokens(bin).size() == 3);

    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
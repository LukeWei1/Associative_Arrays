#ifndef LZ_ENCODER_HPP
#define LZ_ENCODER_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "hash_table.hpp"

/**
 * Lempel-Ziv (LZ78) encoder built on our HashTable.
 *
 * The input is parsed into phrases. Each phrase is the longest phrase already
 * seen plus one new byte, so it is described by a token
 * (index of the earlier phrase, new byte). Index 0 means "empty phrase".
 *
 * The dictionary maps (prefix phrase index, next byte) -> phrase index. That
 * pair is packed into one 64-bit key: (index << 8) | byte.
 */
struct LZToken {
    std::size_t prefix;  ///< 1-based index of the earlier phrase (0 = empty)
    unsigned char ch;    ///< the byte appended to that phrase
    bool operator==(const LZToken& o) const { return prefix == o.prefix && ch == o.ch; }
};

/**
 * Parses @p input into LZ78 tokens.
 * If the input ends in the middle of a phrase that is already in the
 * dictionary, a final token (index, 0) is emitted; the original length stored
 * in the bit-string header tells a decoder to drop that padding byte.
 */
inline std::vector<LZToken> lzTokens(const std::string& input) {
    HashTable<std::uint64_t, std::size_t> dict;
    std::vector<LZToken> tokens;
    std::size_t current = 0;  // phrase matched so far (0 = empty)
    std::size_t nextIndex = 1;
    for (unsigned char c : input) {
        std::uint64_t key = (static_cast<std::uint64_t>(current) << 8) | c;
        auto found = dict.get(key);
        if (found) {
            current = *found;  // keep extending the current phrase
        } else {
            tokens.push_back({current, c});
            dict.set(key, nextIndex++);  // new phrase = current phrase + c
            current = 0;
        }
    }
    if (current != 0) tokens.push_back({current, 0});
    return tokens;
}

/** Number of bits needed to write a number in [0, n-1]: ceil(log2(n)). */
inline unsigned bitsForChoices(std::size_t n) {
    unsigned bits = 0;
    while ((std::size_t{1} << bits) < n) ++bits;
    return bits;
}

/** Appends the low @p nbits of @p value to @p out as '0'/'1' chars, MSB first. */
inline void appendBits(std::string& out, std::uint64_t value, unsigned nbits) {
    for (unsigned i = nbits; i-- > 0;) out.push_back(((value >> i) & 1) ? '1' : '0');
}

/**
 * Encodes @p input into a string of '0'/'1' characters:
 *   [32 bits: original length in bytes]
 *   for the i-th token (i starting at 1):
 *     [ceil(log2(i)) bits: prefix index (it can only be 0..i-1)]
 *     [8 bits: new byte]
 */
inline std::string lzEncodeBits(const std::string& input) {
    std::string bits;
    appendBits(bits, input.size(), 32);
    std::size_t i = 1;
    for (const LZToken& t : lzTokens(input)) {
        appendBits(bits, t.prefix, bitsForChoices(i));
        appendBits(bits, t.ch, 8);
        ++i;
    }
    return bits;
}

/** Packs a '0'/'1' string into bytes (MSB first, zero-padded at the end). */
inline std::string packBits(const std::string& bits) {
    std::string bytes((bits.size() + 7) / 8, '\0');
    for (std::size_t i = 0; i < bits.size(); ++i)
        if (bits[i] == '1') bytes[i / 8] |= static_cast<char>(0x80 >> (i % 8));
    return bytes;
}

/** Convenience: LZ78-compress @p input into a packed byte string. */
inline std::string lzCompress(const std::string& input) {
    return packBits(lzEncodeBits(input));
}

#endif  // LZ_ENCODER_HPP
// Usage: ./lz <input-file> [packed-output-file] [bit-string-output-file]
// Compresses a file with LZ78 and reports the compression ratio.
#include <fstream>
#include <iostream>
#include <iterator>

#include "lz_encoder.hpp"

int main(int argc, char** argv) {
    if (argc < 2) { std::cerr << "usage: " << argv[0] << " <input> [output] [bits-output]\n"; return 1; }
    std::ifstream in(argv[1], std::ios::binary);
    if (!in) { std::cerr << "cannot open " << argv[1] << "\n"; return 1; }
    std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    std::string bits = lzEncodeBits(data);
    std::string packed = packBits(bits);
    std::cout << "original bytes:   " << data.size() << "\n"
              << "phrases:          " << lzTokens(data).size() << "\n"
              << "compressed bytes: " << packed.size() << "\n"
              << "ratio:            "
              << (data.empty() ? 0.0 : static_cast<double>(packed.size()) / data.size()) << "\n";
    if (argc > 2) std::ofstream(argv[2], std::ios::binary) << packed;
    if (argc > 3) std::ofstream(argv[3]) << bits;
    return 0;
}
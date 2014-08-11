#pragma once

// Note: We could make use of #include <immintrin.h> if we ever needed SHA

namespace rx {
namespace hash {

struct B16 { unsigned char bytes[16]; };

void murmur3_128(const std::string& s, B16& result, uint32_t seed=0);
void encode_128(const B16&, char buf[22]);
std::string encode_128(const B16&);

}
} // namespace

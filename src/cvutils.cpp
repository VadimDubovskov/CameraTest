#include "cvutils.h"
#include <random>

std::string gen_uuid() {
    static std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<int> hex(0, 15);
    const char* d = "0123456789abcdef";
    std::string s;
    for (int i = 0; i < 36; ++i)
        s += (i == 8 || i == 13 || i == 18 || i == 23) ? '-' : d[hex(rng)];
    return s;
}

std::vector<uint8_t> cvMatToVector(const cv::Mat& mat) {
    std::vector<uint8_t> result;

    if (mat.isContinuous()) {
        // Direct assignment if memory is contiguous
        result.assign(mat.data, mat.data + mat.total() * mat.elemSize());
    } else {
        // Row-by-row fallback for submatrices or padded structures
        for (int i = 0; i < mat.rows; ++i) {
            const uint8_t* rowPtr = mat.ptr<uint8_t>(i);
            result.insert(result.end(), rowPtr, rowPtr + mat.cols * mat.elemSize());
        }
    }
    return result;
}

std::string int32ToIPString(uint32_t ip)
{
    uint8_t byte0 = (uint8_t) ip & 0xFF;
    uint8_t byte1 = (uint8_t) (ip >> 8) & 0xFF;
    uint8_t byte2 = (uint8_t) (ip >> 16) & 0xFF;
    uint8_t byte3 = (uint8_t) (ip >> 24) & 0xFF;

    std::string ret{std::to_string(byte3) + "." + std::to_string(byte2) + "." + std::to_string(byte1) + "." + std::to_string(byte0)};
    return ret;
}


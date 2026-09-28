#ifndef CVUTILS_H
#define CVUTILS_H

#include <vector>
// OpenCV
#include <opencv2/opencv.hpp>

std::string gen_uuid();
std::vector<uint8_t> cvMatToVector(const cv::Mat& mat);
std::string int32ToIPString(uint32_t ip);

#endif // CVUTILS_H

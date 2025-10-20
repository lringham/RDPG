#pragma once
#include <vector>
#include <string>

struct Image
{
    std::string filename_;
    int width_, height_, numComponents_;
    std::vector<unsigned char> data_;
};

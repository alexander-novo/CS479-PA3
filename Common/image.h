// Common/image.h
#pragma once

#include <Eigen/Core>

#define IMG_WIDTH 48
#define IMG_HEIGHT 60
#define IMG_PIXELS (IMG_WIDTH * IMG_HEIGHT)

using Image = Eigen::Matrix<double, IMG_PIXELS, 1>;

void read(std::istream& in, Image& im);
void write(std::ostream& out, const Image& im);
void normalize(Image& im, double newMax);
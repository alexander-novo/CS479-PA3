// Common/image.h
#pragma once

#include <Eigen/Core>

using Eigen::VectorXd;
using Image = VectorXd;

struct ImageHeader {
	char magicNumber[3];
	unsigned M, N, Q;
};

ImageHeader readHeader(std::istream& in);
void read(std::istream& in, Image& im, ImageHeader match);
void write(std::ostream& out, const Image& im, ImageHeader header);
void normalize(Image& im, ImageHeader header);
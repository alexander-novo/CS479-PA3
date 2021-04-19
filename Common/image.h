// Common/image.h
#pragma once

#include <Eigen/Core>

using MatrixXd = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
using Eigen::VectorXd;

struct ImageHeader {
	char magicNumber[3];
	unsigned M, N, Q;
};

ImageHeader readHeader(std::istream& in);
void read(std::istream& in, Eigen::Block<MatrixXd, -1, 1> im, ImageHeader match);
void write(std::ostream& out, const VectorXd& im, ImageHeader header);
void normalize(VectorXd& im, ImageHeader header);
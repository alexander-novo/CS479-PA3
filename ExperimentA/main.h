#include <omp.h>

#include <Eigen/Core>
#include <Eigen/Eigen>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <queue>
#include <regex>
#include <string>

#include "../Common/image.h"

#define EIGEN_PRESERVE .8

#pragma omp declare reduction(+ : Image : omp_out += omp_in) initializer(omp_priv = omp_orig)

struct Arguments {
	std::ofstream meanFile, cmcPlotFile;
	std::string trainingImageDir, testingImageDir, outDir;
};

double mahalanobisDistance(const Eigen::VectorXd& im1, const Eigen::VectorXd& im2, const Eigen::VectorXd& eigenValues);
bool verifyArguments(int argc, char** argv, Arguments& arg, int& err);
void printHelp();
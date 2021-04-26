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

#define PRINT_CORRECT_IMAGES_NUM 3

#pragma omp declare reduction(+ : VectorXd : omp_out += omp_in) initializer(omp_priv = omp_orig)

enum OperatingMode { Training, Testing, Information };

struct Arguments {
	OperatingMode mode;
	std::ofstream meanFile, cmcPlotFile;
	std::fstream trainingFile;
	std::string imageDir, outDir;
	double infoPercent      = .8;
	bool printCorrectImages = false, printIncorrectImages = false;
};

void train(Arguments& arg);
void info(Arguments& arg);
void test(Arguments& arg);
void getFilesPathsInDir(std::vector<std::filesystem::path>& paths, const std::string& dir);
void readInImages(const std::vector<std::filesystem::path>& imagePaths, MatrixXd& images,
                  std::vector<std::string>& labels, ImageHeader& header);
void writeTrainingData(std::ostream& out, const ImageHeader& header, const VectorXd& mean, const MatrixXd& U,
                       const VectorXd& eigenValues, const MatrixXd& projectedImages,
                       const std::vector<std::string>& labels);
void readTrainingData(std::istream& in, ImageHeader& header, VectorXd& mean, MatrixXd& U, VectorXd& eigenValues,
                      MatrixXd& projectedImages, std::vector<std::string>& labels);
template <typename T, typename U>
double mahalanobisDistance(Eigen::Block<T, -1, 1> im1, Eigen::Block<U, -1, 1> im2,
                           Eigen::VectorBlock<VectorXd> eigenValues);
bool verifyArguments(int argc, char** argv, Arguments& arg, int& err);
void printHelp();
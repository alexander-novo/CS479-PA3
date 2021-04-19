#include "main.h"

int main(int argc, char** argv) {
	int err;
	Arguments arg;

	if (!verifyArguments(argc, argv, arg, err)) { return err; }

	switch (arg.mode) {
		case OperatingMode::Training:
			train(arg);
			break;
		case OperatingMode::Information:
			info(arg);
			break;
		case OperatingMode::Testing:
			test(arg);
			break;
	}
}

void train(Arguments& arg) {
	std::vector<std::filesystem::path> imagePaths;

	getFilesPathsInDir(imagePaths, arg.imageDir);

	std::ifstream init(imagePaths.front());
	ImageHeader header = readHeader(init);
	init.close();

	unsigned imagePixels = header.M * header.N;

	// Read in all the training images and their labels
	MatrixXd images(imagePixels, imagePaths.size());
	std::vector<std::string> labels(images.cols());

	readInImages(imagePaths, images, labels, header);

	VectorXd mean = VectorXd::Zero(imagePixels);

#pragma omp parallel for reduction(+ : mean)
	for (unsigned i = 0; i < images.cols(); i++) { mean += images.col(i) / images.cols(); }

	MatrixXd A = images.colwise() - mean;

	MatrixXd covariance = A.transpose() * A / images.size();
	Eigen::SelfAdjointEigenSolver<MatrixXd> solver(covariance);
	// Multiply by A to get eigenvectors of A*A^T instead of A^T*A
	MatrixXd U = A * solver.eigenvectors();

	MatrixXd projectedImages = U.transpose() * (images.colwise() - mean);

	writeTrainingData(arg.trainingFile, header, mean, U, solver.eigenvalues(), projectedImages, labels);
}

void info(Arguments& arg) {
	ImageHeader header;
	VectorXd mean;
	MatrixXd U;
	VectorXd eigenValues;
	MatrixXd projectedTrainingImages;
	std::vector<std::string> trainingLabels;
	readTrainingData(arg.trainingFile, header, mean, U, eigenValues, projectedTrainingImages, trainingLabels);

	if (arg.meanFile.is_open() && arg.meanFile) { write(arg.meanFile, mean, header); }

	if (!arg.outDir.empty()) {
#pragma omp parallel
		{
#pragma omp for nowait
			for (unsigned i = 0; i < 10; i++) {
				std::ofstream out(arg.outDir + "smallest-" + std::to_string(i + 1) + ".pgm");

				VectorXd img = U.col(i);
				normalize(img, header);
				write(out, img, header);
			}
#pragma omp for
			for (unsigned i = 0; i < 10; i++) {
				std::ofstream out(arg.outDir + "largest-" + std::to_string(i + 1) + ".pgm");

				VectorXd img = U.col(U.cols() - i - 1);
				normalize(img, header);
				write(out, img, header);
			}
		}
	}
}

void test(Arguments& arg) {
	ImageHeader header;
	VectorXd mean;
	MatrixXd U;
	VectorXd eigenValues;
	MatrixXd projectedTrainingImages;
	std::vector<std::string> trainingLabels;
	readTrainingData(arg.trainingFile, header, mean, U, eigenValues, projectedTrainingImages, trainingLabels);

	unsigned imagePixels = header.M * header.N;
	std::vector<std::filesystem::path> testingImagePaths;

	getFilesPathsInDir(testingImagePaths, arg.imageDir);

	// Read in all the training images and their labels
	MatrixXd testingImages(imagePixels, testingImagePaths.size());
	std::vector<std::string> testingLabels(testingImagePaths.size());

	readInImages(testingImagePaths, testingImages, testingLabels, header);

	double totalEigenSum     = eigenValues.sum();
	double targetEigenSum    = totalEigenSum * arg.infoPercent;
	unsigned lowerDimensions = 0;

	while (targetEigenSum > 0) {
		targetEigenSum -= eigenValues[eigenValues.rows() - lowerDimensions - 1];
		++lowerDimensions;
	}

	std::cout << "Reduced to " << lowerDimensions << " dimensions for a total of "
	          << (totalEigenSum * arg.infoPercent - targetEigenSum) / totalEigenSum * 100 << "% information\n";

	MatrixXd projectedTestingImages = U.rightCols(lowerDimensions).transpose() * (testingImages.colwise() - mean);
	std::vector<std::pair<unsigned, unsigned>> correct_matches, incorrect_matches;

	auto lowerDimTrainingImages = projectedTrainingImages.bottomRows(lowerDimensions);
	auto lowerDimEigenValues    = eigenValues.tail(lowerDimensions);

#define MAX_N 50
	using DistanceLabel = std::pair<double, unsigned>;

	// The N value required to correctly classify each image
	std::vector<unsigned> NRequired(projectedTestingImages.cols());
	std::vector<std::string> classifiedLabels(testingImages.cols());
	std::vector<unsigned> bestTrainingImages(testingImages.cols());

#pragma omp parallel
	{
		std::vector<DistanceLabel> heap(MAX_N);
#pragma omp for
		for (unsigned i = 0; i < projectedTestingImages.cols(); i++) {
			double distance;

			// For our first 50 distances, just insert into the heap
			for (unsigned j = 0; j < MAX_N; j++) {
				distance = mahalanobisDistance(projectedTestingImages.col(i), lowerDimTrainingImages.col(j),
				                               lowerDimEigenValues);

				heap[j] = {distance, j};
			}

			std::make_heap(heap.begin(), heap.end());

			// Afterwards, keep the size constant and only insert if needed
			for (unsigned j = MAX_N; j < lowerDimTrainingImages.cols(); j++) {
				distance = mahalanobisDistance(projectedTestingImages.col(i), lowerDimTrainingImages.col(j),
				                               lowerDimEigenValues);

				if (distance < heap.front().first) {
					std::pop_heap(heap.begin(), heap.end());
					heap.back() = {distance, j};
					std::push_heap(heap.begin(), heap.end());
				}
			}

			std::sort_heap(heap.begin(), heap.end());

			classifiedLabels[i]   = trainingLabels[heap[0].second];
			bestTrainingImages[i] = heap[0].second;

			unsigned j;
			for (j = 0; j < heap.size(); j++) {
				if (trainingLabels[heap[j].second] == testingLabels[i]) {
					NRequired[i] = j + 1;
					break;
				}
			}

			if (j == heap.size()) { NRequired[i] = MAX_N + 1; }
		}
	}

	// Store correctly and incorrectly matched query images for which N = 1
	for (unsigned i = 0; i < testingImages.cols(); ++i) {
		if (classifiedLabels[i] == testingLabels[i]) {
			correct_matches.push_back({i, bestTrainingImages[i]});
		} else {
			incorrect_matches.push_back({i, bestTrainingImages[i]});
		}
	}
	std::cout << "Assuming N=1, Correct matches: " << correct_matches.size() << std::endl;
	std::cout << "Assuming N=1, Incorrect matches: " << incorrect_matches.size() << std::endl;
	std::cout << "Total Size: " << testingImages.size() << std::endl;

	if (arg.cmcPlotFile.is_open() && arg.cmcPlotFile) {
		arg.cmcPlotFile << "# N   Performance\n";
		unsigned correctMatches = 0;
		for (unsigned i = 0; i < MAX_N; i++) {
			correctMatches += std::count_if(NRequired.begin(), NRequired.end(), [i](unsigned n) { return n == i + 1; });
			arg.cmcPlotFile << std::setw(3) << i + 1 << "   " << ((double) correctMatches) / testingImages.cols()
			                << '\n';
		}
		arg.cmcPlotFile.close();
	}

#pragma omp parallel
	{
#pragma omp for nowait
		for (unsigned i = 0; i < 3; i++) {
			std::ofstream out(arg.outDir + "correct-" + std::to_string(i + 1) + "-train.pgm");

			VectorXd img = U * projectedTrainingImages.col(correct_matches[i].second);
			normalize(img, header);
			write(out, img, header);
		}
#pragma omp for nowait
		for (unsigned i = 0; i < 3; i++) {
			std::ofstream out(arg.outDir + "correct-" + std::to_string(i + 1) + "-test.pgm");

			VectorXd img = testingImages.col(correct_matches[i].first);
			write(out, img, header);
		}
#pragma omp for nowait
		for (unsigned i = 0; i < 3; i++) {
			std::ofstream out(arg.outDir + "incorrect-" + std::to_string(i + 1) + "-train.pgm");

			VectorXd img = U * projectedTrainingImages.col(incorrect_matches[i].second);
			normalize(img, header);
			write(out, img, header);
		}
#pragma omp for
		for (unsigned i = 0; i < 3; i++) {
			std::ofstream out(arg.outDir + "incorrect-" + std::to_string(i + 1) + "-test.pgm");

			VectorXd img = testingImages.col(incorrect_matches[i].first);
			write(out, img, header);
		}
	}
}

void getFilesPathsInDir(std::vector<std::filesystem::path>& paths, const std::string& dir) {
	for (auto& p : std::filesystem::directory_iterator(dir)) {
		if (!p.is_regular_file()) continue;

		paths.push_back(p.path());
	}
}

void readInImages(const std::vector<std::filesystem::path>& imagePaths, MatrixXd& images,
                  std::vector<std::string>& labels, ImageHeader& header) {
	// https://regex101.com/r/YmCEZ3/1
	const std::regex labelRegex("([[:digit:]]{5})_[[:digit:]]{2}[[:digit:]]{2}[[:digit:]]{2}_f[ab](_[[:alpha:]])?.pgm");

	std::ifstream inFile;
	std::smatch match;

#pragma omp parallel for private(inFile) private(match)
	for (unsigned i = 0; i < images.cols(); i++) {
		inFile.open(imagePaths[i]);

		try {
			read(inFile, images.col(i), header);
		} catch (std::runtime_error& e) {
#pragma omp critical
			std::cout << "Could not read file " << imagePaths[i] << ": " << e.what() << '\n';
			inFile.close();
			continue;
		}

		inFile.close();

		std::string filename = imagePaths[i].filename();
		if (!std::regex_match(filename, match, labelRegex)) {
#pragma omp critical
			std::cout << "Failed to match \"" << filename << "\" from " << imagePaths[i] << "\n";
		}
		labels[i] = match[1].str();
	}
}

// TODO: rewrite this using binary mode
void writeTrainingData(std::ostream& out, const ImageHeader& header, const VectorXd& mean, const MatrixXd& U,
                       const VectorXd& eigenValues, const MatrixXd& projectedImages,
                       const std::vector<std::string>& labels) {
	out << header.M << " " << header.N << " " << header.Q << " " << projectedImages.cols();
	for (unsigned i = 0; i < mean.rows(); i++) { out << '\n' << mean(i); }
	for (unsigned i = 0; i < U.rows(); i++) {
		for (unsigned j = 0; j < U.cols(); j++) { out << '\n' << U(i, j); }
	}
	for (unsigned i = 0; i < eigenValues.rows(); i++) { out << eigenValues(i) << '\n'; }
	for (unsigned i = 0; i < projectedImages.rows(); i++) {
		for (unsigned j = 0; j < projectedImages.cols(); j++) { out << '\n' << projectedImages(i, j); }
	}
	for (const std::string& label : labels) { out << '\n' << label; }
}

void readTrainingData(std::istream& in, ImageHeader& header, VectorXd& mean, MatrixXd& U, VectorXd& eigenValues,
                      MatrixXd& projectedImages, std::vector<std::string>& labels) {
	size_t numImages;
	in >> header.M >> header.N >> header.Q >> numImages;
	header.magicNumber[0] = 'P';
	header.magicNumber[1] = '5';
	header.magicNumber[2] = '\0';

	unsigned numPixels = header.M * header.N;
	mean.resize(numPixels);
	U.resize(numPixels, numImages);
	eigenValues.resize(numImages);
	// Projected images are projected onto a reduced number of eigenvectors (only numImages many)
	// so they are reduced in dimension
	projectedImages.resize(numImages, numImages);
	labels.resize(numImages);

	for (unsigned i = 0; i < mean.rows(); i++) { in >> mean(i); }
	for (unsigned i = 0; i < U.rows(); i++) {
		for (unsigned j = 0; j < U.cols(); j++) { in >> U(i, j); }
	}
	for (unsigned i = 0; i < eigenValues.rows(); i++) { in >> eigenValues(i); }
	for (unsigned i = 0; i < projectedImages.rows(); i++) {
		for (unsigned j = 0; j < projectedImages.cols(); j++) { in >> projectedImages(i, j); }
	}
	// We're at the end of a line at this point, so consume the end of line token
	std::getline(in, labels.front());
	for (std::string& label : labels) { std::getline(in, label); }
}

template <typename T, typename U>
double mahalanobisDistance(Eigen::Block<T, -1, 1> im1, Eigen::Block<U, -1, 1> im2,
                           Eigen::VectorBlock<VectorXd> eigenValues) {
	double distance = 0;
	unsigned dim    = im1.rows();
	for (unsigned i = 0; i < dim; i++) {
		unsigned idx = dim - i - 1;
		double temp  = im1(idx) - im2(idx);
		distance += temp * temp / eigenValues(idx);
	}

	return distance;
}

bool verifyArguments(int argc, char** argv, Arguments& arg, int& err) {
	if (argc < 2 || (argc < 3 && strcmp(argv[1], "-h") && strcmp(argv[1], "--help"))) {
		std::cout << "Missing operand.\n\n";
		err = 1;
		printHelp();
		return false;
	}

	if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
		printHelp();
		return false;
	}

	// Required arguments
	if (strcmp(argv[1], "train") == 0) {
		arg.mode = OperatingMode::Training;
	} else if (strcmp(argv[1], "info") == 0) {
		arg.mode = OperatingMode::Information;
	} else if (strcmp(argv[1], "test") == 0) {
		arg.mode = OperatingMode::Testing;
	} else {
		std::cout << "Operating mode \"" << argv[1] << "\" not recognised.\n";
		err = 1;
		printHelp();
		return false;
	}

	switch (arg.mode) {
		case OperatingMode::Training: {
			if (argc < 4) {
				std::cout << "Missing operand.\n\n";
				err = 1;
				printHelp();
				return false;
			}

			arg.imageDir = argv[2];
			if (!std::filesystem::exists(arg.imageDir) ||
			    !std::filesystem::directory_entry(arg.imageDir).is_directory()) {
				std::cout << "Directory \"" << arg.imageDir << "\" could not be found or is not a directory.\n";
				err = 1;
				printHelp();
				return false;
			}
			auto iter = std::filesystem::directory_iterator(arg.imageDir);
			if (iter == std::filesystem::end(iter)) {
				std::cout << "Directory \"" << arg.imageDir << "\" is empty.\n";
				err = 1;
				printHelp();
				return false;
			}

			arg.trainingFile.open(argv[3], std::ios_base::out);
			if (!arg.trainingFile.is_open() || !arg.trainingFile) {
				std::cout << "Could not open file \"" << argv[3] << "\".\n";
				err = 1;
				return false;
			}
			break;
		}
		case OperatingMode::Information: {
			arg.trainingFile.open(argv[2], std::ios_base::in);
			if (!arg.trainingFile.is_open() || !arg.trainingFile) {
				std::cout << "Could not open file \"" << argv[3] << "\".\n";
				err = 1;
				return false;
			}
			break;
		}
		case OperatingMode::Testing: {
			if (argc < 4) {
				std::cout << "Missing operand.\n\n";
				err = 1;
				printHelp();
				return false;
			}

			arg.imageDir = argv[2];
			if (!std::filesystem::exists(arg.imageDir) ||
			    !std::filesystem::directory_entry(arg.imageDir).is_directory()) {
				std::cout << "Directory \"" << arg.imageDir << "\" could not be found or is not a directory.\n";
				err = 1;
				printHelp();
				return false;
			}
			auto iter = std::filesystem::directory_iterator(arg.imageDir);
			if (iter == std::filesystem::end(iter)) {
				std::cout << "Directory \"" << arg.imageDir << "\" is empty.\n";
				err = 1;
				printHelp();
				return false;
			}

			arg.trainingFile.open(argv[3], std::ios_base::in);
			if (!arg.trainingFile.is_open() || !arg.trainingFile) {
				std::cout << "Could not open file \"" << argv[3] << "\".\n";
				err = 1;
				return false;
			}
			break;
		}
	}

	// Optional Arguments
	switch (arg.mode) {
		case OperatingMode::Training:
			break;
		case OperatingMode::Information:
			for (unsigned i = 3; i < argc; i++) {
				if (!strcmp(argv[i], "-m")) {
					if (i + 1 >= argc) {
						std::cout << "Missing mean image file.\n\n";
						err = 1;
						printHelp();
						return false;
					}

					arg.meanFile.open(argv[i + 1]);
					if (!arg.meanFile.is_open() || !arg.meanFile) {
						std::cout << "Could not open file \"" << argv[i + 1] << "\".\n";
						err = 2;
						return false;
					}

					i++;
				} else if (!strcmp(argv[i], "-eig")) {
					if (i + 1 >= argc) {
						std::cout << "Missing eigenface output prefix.\n\n";
						err = 1;
						printHelp();
						return false;
					}

					arg.outDir = argv[i + 1];

					i++;
				} else {
					std::cout << "Unrecognised argument \"" << argv[i] << "\".\n";
					printHelp();
					err = 1;
					return false;
				}
			}
			break;
		case OperatingMode::Testing:
			for (unsigned i = 4; i < argc; i++) {
				if (!strcmp(argv[i], "-cmc")) {
					if (i + 1 >= argc) {
						std::cout << "Missing CMC plot file.\n\n";
						err = 1;
						printHelp();
						return false;
					}

					arg.cmcPlotFile.open(argv[i + 1]);
					if (!arg.cmcPlotFile.is_open() || !arg.cmcPlotFile) {
						std::cout << "Could not open file \"" << argv[i + 1] << "\".\n";
						err = 2;
						return false;
					}

					i++;
				} else if (!strcmp(argv[i], "-i")) {
					if (i + 1 >= argc) {
						std::cout << "Missing information percentage.\n\n";
						err = 1;
						printHelp();
						return false;
					}

					char* end;
					arg.infoPercent = strtod(argv[i + 1], &end) / 100;
					if (end == argv[i + 1]) {
						std::cout << "\"" << argv[i + 1] << "\" could not be interpreted as a floating-point number.\n";
						err = 2;
						return false;
					} else if (arg.infoPercent <= 0 || arg.infoPercent > 1) {
						std::cout << "Information percentage must in (0,100].\n";
						err = 2;
						return false;
					}

					i++;
				} else if (!strcmp(argv[i], "-img")) {
					if (i + 1 >= argc) {
						std::cout << "Missing output image prefix.\n\n";
						err = 1;
						printHelp();
						return false;
					}

					arg.outDir = argv[i + 1];

					i++;
				} else {
					std::cout << "Unrecognised argument \"" << argv[i] << "\".\n";
					printHelp();
					err = 1;
					return false;
				}
			}
			break;
	}
	return true;
}

void printHelp() {
	Arguments arg;
	std::cout << "Usage: experiment train <train-dir> <output-file>                              (1)\n"
	          << "   or: experiment info  <training-file> [options]                              (2)\n"
	          << "   or: experiment test  <test-dir>  <training-file> [options]                  (3)\n"
	          << "   or: experiment -h                                                           (4)\n\n"
	          << "(1) Train the experiment using the images from the given directory.\n"
	          << "    Then output training data to a file to be used later for testing.\n"
	          << "(2) Query information from the training data.\n"
	          << "(3) Perform the experiment using the given training data (generated using (1))\n"
	          << "    and test against images from the given directory.\n"
	          << "(4) Print this help menu.\n\n"
	          << "INFO OPTIONS\n"
	          << "  -m   <file>  Print the mean face to a file.\n"
	          << "  -eig <pre>   Output representative eigenfaces with a filename prefix of <pre>.\n"
	          << "               For instance '-eig out/pre-' will output files like\n"
	          << "               'out/pre-largest-1.pgm' and 'out/pre-smallest-1.pgm.\n\n"
	          << "TESTING OPTIONS\n"
	          << "  -cmc <file>  Print CMC plot data to a file.\n"
	          << "  -i   <num>   The percentage of information to be preserved when picking\n"
	          << "               eigenfaces. Defaults to " << arg.infoPercent * 100 << "%.\n"
	          << "  -img <pre>   Output images which were classified correctly and incorrectly\n"
	          << "               with the given prefix.\n";
}
#include "main.h"

int main(int argc, char** argv) {
	int err;
	Arguments arg;

	if (!verifyArguments(argc, argv, arg, err)) { return err; }

	std::vector<std::filesystem::path> trainingImagePaths, testingImagePaths;
	Image mean = Image::Zero();
	std::ifstream inFile;

	// Read in all of the image names in the training and testing directories
	// This can be done in parallel - one thread for each directory
#pragma omp parallel num_threads(2)
	{
		if (omp_get_thread_num() == 0) {
			for (auto& p : std::filesystem::directory_iterator(arg.trainingImageDir)) {
				if (!p.is_regular_file()) continue;

				trainingImagePaths.push_back(p.path());
			}
		} else {
			for (auto& p : std::filesystem::directory_iterator(arg.testingImageDir)) {
				if (!p.is_regular_file()) continue;

				testingImagePaths.push_back(p.path());
			}
		}
	}

	// Read in all the images - training and testing - and their labels
	std::vector<Image, Eigen::aligned_allocator<Image>> trainingImages(trainingImagePaths.size()),
	    testingImages(testingImagePaths.size());
	std::vector<std::string> trainingLabels(trainingImages.size()), testingLabels(testingImages.size());
	// https://regex101.com/r/YmCEZ3/1
	std::regex labelRegex("([[:digit:]]{5})_[[:digit:]]{2}[[:digit:]]{2}[[:digit:]]{2}_f[ab](_a)?.pgm");
#pragma omp parallel
	{
		std::ifstream inFile;
		std::smatch match;
#pragma omp for nowait reduction(+ : mean)
		for (unsigned i = 0; i < trainingImages.size(); i++) {
			inFile.open(trainingImagePaths[i]);

			Image& im = trainingImages[i];
			read(inFile, im);

			inFile.close();

			mean += im / trainingImages.size();

			std::string filename = trainingImagePaths[i].filename();
			std::regex_match(filename, match, labelRegex);
			trainingLabels[i] = match[1].str();
		}
#pragma omp for
		for (unsigned i = 0; i < testingImages.size(); i++) {
			inFile.open(testingImagePaths[i]);

			Image& im = testingImages[i];
			read(inFile, im);
			inFile.close();

			std::string filename = testingImagePaths[i].filename();
			std::regex_match(filename, match, labelRegex);
			testingLabels[i] = match[1].str();
		}
	}

	Eigen::MatrixXd A(IMG_PIXELS, trainingImages.size());

#pragma omp parallel for
	for (unsigned i = 0; i < trainingImages.size(); i++) { A.col(i) = trainingImages[i] - mean; }

	Eigen::MatrixXd covariance = A.transpose() * A / trainingImages.size();
	Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> solver(covariance);
	// Multiply by A to get eigenvectors of A*A^T instead of A^T*A
	Eigen::MatrixXd eigenVectors = A * solver.eigenvectors();

	Eigen::VectorXd eigenValues = solver.eigenvalues();
	double totalEigenSum        = eigenValues.sum();
	double targetEigenSum       = totalEigenSum * EIGEN_PRESERVE;
	unsigned lowerDimensions    = 0;

	while (targetEigenSum > 0) {
		targetEigenSum -= eigenValues[eigenValues.rows() - lowerDimensions - 1];
		++lowerDimensions;
	}

	std::cout << "Reduced to " << lowerDimensions << " dimensions for a total of "
	          << (totalEigenSum * EIGEN_PRESERVE - targetEigenSum) / totalEigenSum * 100 << "% information\n";

	auto U = eigenVectors.rightCols(lowerDimensions);
	std::vector<Eigen::VectorXd> projectedTrainingImages(testingImages.size(), Eigen::VectorXd(lowerDimensions, 1)),
	    projectedTestingImages(testingImages.size(), Eigen::VectorXd(lowerDimensions, 1));

#pragma omp parallel
	{
#pragma omp for nowait
		for (unsigned i = 0; i < projectedTrainingImages.size(); i++) {
			projectedTrainingImages[i] = U.transpose() * (trainingImages[i] - mean);
		}
#pragma omp for
		for (unsigned i = 0; i < projectedTestingImages.size(); i++) {
			projectedTestingImages[i] = U.transpose() * (testingImages[i] - mean);
		}
	}

	if (arg.cmcPlotFile) {
#define MAX_N 50
		using DistanceLabel = std::pair<double, unsigned>;
		using MaxHeap       = std::priority_queue<DistanceLabel>;

		// The N value required to correctly classify each image
		std::vector<unsigned> NRequired(testingImages.size());

// Can't collapse this loop since we need a fresh queue for each i iteration.
#pragma omp parallel for
		for (unsigned i = 0; i < projectedTestingImages.size(); i++) {
			std::vector<DistanceLabel> heap;
			double distance;

			// For our first 50 distances, just insert into the heap
			for (unsigned j = 0; j < MAX_N; j++) {
				distance = mahalanobisDistance(projectedTestingImages[i], projectedTrainingImages[j], eigenValues);
				heap.push_back({distance, j});
			}

			std::make_heap(heap.begin(), heap.end());

			// Afterwards, keep the size constant and only insert if needed
			for (unsigned j = MAX_N; j < projectedTrainingImages.size(); j++) {
				distance = mahalanobisDistance(projectedTestingImages[i], projectedTrainingImages[j], eigenValues);
				if (distance < heap.front().first) {
					std::pop_heap(heap.begin(), heap.end());
					heap.back() = {distance, j};
					std::push_heap(heap.begin(), heap.end());
				}
			}

			std::sort_heap(heap.begin(), heap.end());

			unsigned j;
			for (j = 0; j < heap.size(); j++) {
				if (trainingLabels[heap[j].second] == testingLabels[i]) {
					NRequired[i] = j + 1;
					break;
				}
			}

			if (j == heap.size()) { NRequired[i] = MAX_N + 1; }
		}

		arg.cmcPlotFile << "# N   Performance\n";
		unsigned correctMatches = 0;
		for (unsigned i = 0; i < MAX_N; i++) {
			correctMatches += std::count_if(NRequired.begin(), NRequired.end(), [i](unsigned n) { return n == i + 1; });
			arg.cmcPlotFile << std::setw(3) << i + 1 << "   " << ((double) correctMatches) / testingImages.size()
			                << '\n';
		}
		arg.cmcPlotFile.close();
	}

	if (!arg.outDir.empty()) {
#pragma omp parallel
		{
#pragma omp for nowait
			for (unsigned i = 0; i < 10; i++) {
				std::ofstream out(arg.outDir + "/smallest_eigenface_" + std::to_string(i + 1) + ".pgm");

				Image img = eigenVectors.col(i);
				normalize(img, 255);
				write(out, img);
			}
#pragma omp for
			for (unsigned i = 0; i < 10; i++) {
				std::ofstream out(arg.outDir + "/largest_eigenface_" + std::to_string(i + 1) + ".pgm");

				Image img = eigenVectors.col(trainingImages.size() - i - 1);
				normalize(img, 255);
				write(out, img);
			}
		}
	}

	if (arg.meanFile) { write(arg.meanFile, mean); }
}

double mahalanobisDistance(const Eigen::VectorXd& im1, const Eigen::VectorXd& im2, const Eigen::VectorXd& eigenValues) {
	double distance = 0;
	unsigned dim    = im1.rows();
	for (unsigned i = 0; i < dim; i++) {
		unsigned idx = dim - i - 1;
		double temp  = im1(idx, 0) - im2(idx, 0);
		distance += temp * temp / eigenValues(eigenValues.rows() - i - 1, 0);
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
	arg.trainingImageDir = argv[1];
	if (!std::filesystem::exists(arg.trainingImageDir) ||
	    !std::filesystem::directory_entry(arg.trainingImageDir).is_directory()) {
		std::cout << "Directory \"" << arg.trainingImageDir << "\" could not be found or is not a directory.\n";
		err = 1;
		printHelp();
		return false;
	}
	auto iter = std::filesystem::directory_iterator(arg.trainingImageDir);
	if (iter == std::filesystem::end(iter)) {
		std::cout << "Directory \"" << arg.trainingImageDir << "\" is empty.\n";
		err = 1;
		printHelp();
		return false;
	}

	arg.testingImageDir = argv[2];
	if (!std::filesystem::exists(arg.testingImageDir) ||
	    !std::filesystem::directory_entry(arg.testingImageDir).is_directory()) {
		std::cout << "Directory \"" << arg.testingImageDir << "\" could not be found or is not a directory.\n";
		err = 1;
		printHelp();
		return false;
	}
	iter = std::filesystem::directory_iterator(arg.testingImageDir);
	if (iter == std::filesystem::end(iter)) {
		std::cout << "Directory \"" << arg.testingImageDir << "\" is empty.\n";
		err = 1;
		printHelp();
		return false;
	}

	// Optional Arguments
	for (unsigned i = 3; i < argc; i++) {
		if (!strcmp(argv[i], "-m")) {
			if (i + 1 >= argc) {
				std::cout << "Missing mean image file.\n\n";
				err = 1;
				printHelp();
				return false;
			}

			arg.meanFile.open(argv[i + 1]);
			if (!arg.meanFile) {
				std::cout << "Could not open file \"" << argv[i + 1] << "\".\n";
				err = 2;
				return false;
			}

			i++;
		} else if (!strcmp(argv[i], "-o")) {
			if (i + 1 >= argc) {
				std::cout << "Missing output directory.\n\n";
				err = 1;
				printHelp();
				return false;
			}

			arg.outDir = argv[i + 1];
			if (!std::filesystem::is_directory(arg.outDir)) {
				std::cout << "Directory \"" << arg.outDir << "\" does not exist.";
				err = 2;
				return false;
			}

			i++;
		} else if (!strcmp(argv[i], "-cmc")) {
			if (i + 1 >= argc) {
				std::cout << "Missing CMC plot file.\n\n";
				err = 1;
				printHelp();
				return false;
			}

			arg.cmcPlotFile.open(argv[i + 1]);
			if (!arg.cmcPlotFile) {
				std::cout << "Could not open file \"" << argv[i + 1] << "\".\n";
				err = 2;
				return false;
			}

			i++;
		} else {
			std::cout << "Unrecognised argument \"" << argv[i] << "\".\n";
			printHelp();
			err = 1;
			return false;
		}
	}
	return true;
}

void printHelp() {
	std::cout << "Usage: experiment <train-dir> <test-dir> [options]                             (1)\n"
	          << "   or: experiment -h                                                           (2)\n\n"
	          << "(1) Run the experiment on the given directory of trainingImages.\n"
	          << "(2) Print this help menu.\n\n"
	          << "OPTIONS\n"
	          << "  -m   <file>  Print the mean face to a file.\n"
	          << "  -o    <dir>  Output eigenfaces to this directory.\n"
	          << "  -cmc <file>  Print CMC plot data to a file.\n";
}
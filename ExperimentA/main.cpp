#include "main.h"

int main(int argc, char** argv) {
	int err;
	Arguments arg;

	if (!verifyArguments(argc, argv, arg, err)) { return err; }

	std::vector<Image> images;
	Image mean = Image::Zero();
	std::ifstream inFile;

	// TODO - this could be rewritten to be parallelized.
	// Do something like loop through and store all the paths.
	// Then come back and read. That way we also wouldn't have to unmultiply the mean each time.
	for (auto& p : std::filesystem::directory_iterator(arg.imageDir)) {
		if (!p.is_regular_file()) continue;

		inFile.open(p.path());
		if (!inFile) {
			std::cout << "File \"" << p.path() << "\" could not be read.\n";
			return 2;
		}

		images.emplace_back();
		try {
			read(inFile, images.back());
		} catch (std::runtime_error& e) {
			std::cout << "Error reading " << p.path() << ":\n" << e.what() << std::endl;
			return 2;
		}
		inFile.close();

		unsigned numImages = images.size();
		mean               = mean * (((double) numImages - 1) / numImages) + images.back() / numImages;
	}

	Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> A(IMG_PIXELS, images.size());

#pragma omp parallel for
	for (unsigned i = 0; i < images.size(); i++) { A.col(i) = images[i] - mean; }

	Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> covariance = A * A.transpose() / images.size();
	Eigen::SelfAdjointEigenSolver<Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>> solver(covariance);
	Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> eigenVectors = solver.eigenvectors();

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

			Image img = eigenVectors.col(IMG_PIXELS - i - 1);
			normalize(img, 255);
			write(out, img);
		}
	}

	if (arg.meanFile) { write(arg.meanFile, mean); }
}

bool verifyArguments(int argc, char** argv, Arguments& arg, int& err) {
	if (argc < 2 || (argc < 2 && strcmp(argv[1], "-h") && strcmp(argv[1], "--help"))) {
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
	arg.imageDir = argv[1];
	if (!std::filesystem::exists(arg.imageDir) || !std::filesystem::directory_entry(arg.imageDir).is_directory()) {
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

	// Optional Arguments
	for (unsigned i = 2; i < argc; i++) {
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
	std::cout << "Usage: experiment <image-dir> [options]                                        (1)\n"
	          << "   or: experiment -h                                                           (2)\n\n"
	          << "(1) Run the experiment on the given directory of images.\n"
	          << "(2) Print this help menu.\n\n"
	          << "OPTIONS\n"
	          << "  -m   <file>  Print the mean face to a file.\n"
	          << "  -o    <dir>  Output eigenfaces to this directory.\n";
}
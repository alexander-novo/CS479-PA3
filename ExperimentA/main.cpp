#include "main.h"

int main(int argc, char** argv) {
	int err;
	Arguments arg;

	if (!verifyArguments(argc, argv, arg, err)) { return err; }

	Image im;
	Image mean = Image::Zero();
	std::ifstream inFile;

	unsigned numImages = 0;

	for (auto& p : std::filesystem::directory_iterator(arg.imageDir)) {
		if (!p.is_regular_file()) continue;

		inFile.open(p.path());
		if (!inFile) {
			std::cout << "File \"" << p.path() << "\" could not be read.\n";
			return 2;
		}

		try {
			read(inFile, im);
		} catch (std::runtime_error& e) {
			std::cout << "Error reading " << p.path() << ":\n" << e.what() << std::endl;
			return 2;
		}
		inFile.close();

		mean = mean * (numImages / ((double) numImages + 1)) + im / (numImages + 1);
		++numImages;
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
	          << "  -m   <file>  Print the mean face to a file.\n";
}
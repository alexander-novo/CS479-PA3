#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "../Common/image.h"

struct Arguments {
	std::ofstream meanFile;
	std::string imageDir;
};

bool verifyArguments(int argc, char** argv, Arguments& arg, int& err);
void printHelp();
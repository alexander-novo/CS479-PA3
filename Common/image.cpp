// Common/image.cpp
#include "image.h"

// SPECIFICALLY OPTIMIZED FOR KNOWN IMAGE SIZE
void read(std::istream& in, Image& im) {
	int N, M, Q;
	unsigned char charImage[IMG_PIXELS];
	char header[100], *ptr;

	using namespace std::string_literals;

	// Read magic number in header
	in.get(header, 3);
	if ((header[0] != 'P') || (header[1] != '5')) {
		throw std::runtime_error("Image is not PGM! Detected header: "s + header[0] + header[1]);
	}

	// Skip any whitespace and then any comments
	if (!isspace(in.peek())) { throw std::runtime_error("Image does not have whitespace after header!"); }
	while (isspace(in.peek())) { in.get(); }
	while (in.peek() == '#') in.getline(header, 100, '\n');

	in >> N;
	if (!isspace(in.peek())) { throw std::runtime_error("Image does not have whitespace after header!"); }
	while (isspace(in.peek())) { in.get(); }
	while (in.peek() == '#') in.getline(header, 100, '\n');
	in >> M;
	if (!isspace(in.peek())) { throw std::runtime_error("Image does not have whitespace after header!"); }
	while (isspace(in.peek())) { in.get(); }
	while (in.peek() == '#') in.getline(header, 100, '\n');
	in >> Q;
	in.getline(header, 100, '\n');

	if (N != IMG_WIDTH) {
		throw std::runtime_error("Image width does not match ("s + std::to_string(N) + " v.s. "s +
		                         std::to_string(IMG_WIDTH) + ")"s);
	} else if (M != IMG_HEIGHT) {
		throw std::runtime_error("Image height does not match ("s + std::to_string(M) + " v.s. "s +
		                         std::to_string(IMG_HEIGHT) + ")"s);
	}

	if (Q > 255) throw std::runtime_error("Image cannot be read correctly (Q > 255)!");

	in.read(reinterpret_cast<char*>(charImage), IMG_PIXELS * sizeof(unsigned char));

#pragma omp parallel for
	for (unsigned i = 0; i < IMG_PIXELS; i++) { im(i, 0) = charImage[i]; }
}

// Slightly modified version of writeImage() function provided by Dr. Bebis
void write(std::ostream& out, const Image& im) {
	out << "P5" << std::endl;
	out << IMG_WIDTH << " " << IMG_HEIGHT << std::endl;
	out << 255 << std::endl;

	unsigned char charImage[IMG_PIXELS];

#pragma omp parallel for
	for (unsigned i = 0; i < IMG_PIXELS; i++) { charImage[i] = im(i, 0); }

	out.write(reinterpret_cast<char*>(charImage), IMG_PIXELS * sizeof(unsigned char));

	if (out.fail()) throw std::runtime_error("Something failed with writing image.");
}

void normalize(Image& im, double newMax) {
	double max = im(0, 0);
	double min = max;

#pragma omp parallel for reduction(max : max)
	for (unsigned i = 1; i < IMG_PIXELS; i++) {
		max = std::max(max, im(i, 0));
		min = std::min(min, im(i, 0));
	}

	im = (im.array() - min) * newMax / (max - min);
}
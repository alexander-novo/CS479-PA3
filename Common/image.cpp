// Common/image.cpp
#include "image.h"

ImageHeader readHeader(std::istream& in) {
	ImageHeader re;
	char buf[200];

	using namespace std::string_literals;

	// Read magic number in header
	in.get(re.magicNumber, 3);
	if ((re.magicNumber[0] != 'P') || (re.magicNumber[1] != '5')) {
		throw std::runtime_error("Image is not PGM! Detected header: "s + re.magicNumber[0] + re.magicNumber[1]);
	}

	// Skip any whitespace and then any comments
	if (!isspace(in.peek())) { throw std::runtime_error("Image does not have whitespace after header!"); }
	while (isspace(in.peek())) { in.get(); }
	while (in.peek() == '#') in.getline(buf, 200, '\n');

	in >> re.N;
	if (!isspace(in.peek())) { throw std::runtime_error("Image does not have whitespace after header!"); }
	while (isspace(in.peek())) { in.get(); }
	while (in.peek() == '#') in.getline(buf, 200, '\n');
	in >> re.M;
	if (!isspace(in.peek())) { throw std::runtime_error("Image does not have whitespace after header!"); }
	while (isspace(in.peek())) { in.get(); }
	while (in.peek() == '#') in.getline(buf, 200, '\n');
	in >> re.Q;

	return re;
}

void read(std::istream& in, Eigen::Block<MatrixXd, -1, 1> im, ImageHeader match) {
	unsigned M, N, Q;
	unsigned char* charImage;
	char header[100], *ptr;

	using namespace std::string_literals;

	// Read magic number in header
	in.get(header, 3);
	if ((header[0] != 'P') || (header[1] != '5')) {
		throw std::runtime_error("Image is not PGM! Detected header: '"s + header[0] + header[1] + "'"s);
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

	if (N != match.N || M != match.M || Q != match.Q)
		throw std::runtime_error("Read Image does not match given header.");
	if (Q > 255) throw std::runtime_error("Image cannot be read correctly (Q > 255)!");

	charImage = new unsigned char[M * N];
	in.read(reinterpret_cast<char*>(charImage), M * N * sizeof(unsigned char));

#pragma omp parallel for
	for (unsigned i = 0; i < M * N; i++) { im(i) = charImage[i]; }
}

// Slightly modified version of writeImage() function provided by Dr. Bebis
void write(std::ostream& out, const VectorXd& im, ImageHeader header) {
	out << header.magicNumber << std::endl;
	out << header.N << " " << header.M << std::endl;
	out << header.Q << std::endl;

	unsigned char* charImage = new unsigned char[header.M * header.N];

#pragma omp parallel for
	for (unsigned i = 0; i < header.M * header.N; i++) { charImage[i] = im(i); }

	out.write(reinterpret_cast<char*>(charImage), header.M * header.N * sizeof(unsigned char));

	if (out.fail()) throw std::runtime_error("Something failed with writing image.");
}

void normalize(VectorXd& im, ImageHeader header) {
	double max = im(0);
	double min = max;

#pragma omp parallel for reduction(max : max) reduction(min : min)
	for (unsigned i = 1; i < im.rows(); i++) {
		max = std::max(max, im(i));
		min = std::min(min, im(i));
	}

	im = (im.array() - min) * header.Q / (max - min);
}
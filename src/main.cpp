#include "hmap.h"
#include <iostream>

int main(const int argc, const char *argv[]) {
	try {
	} catch(const std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
	} catch(...) {
		std::cerr << "Unknown exception" << std::endl;
	}
}


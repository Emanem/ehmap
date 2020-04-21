/*
    This file is part of ehmap.

    ehmap is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    ehmap is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ehmap.  If not, see <https://www.gnu.org/licenses/>.
 * */

#include "hmap.h"
#include <iostream>

int main(const int argc, const char *argv[]) {
	try {
		struct my_hasher {
			uint32_t operator()(const int& i) {
				return static_cast<uint32_t>(i);
			}
		};

		ema::hmap<int, double, my_hasher>	m;
		std::cout << "m.mem_size()\t" << m.mem_size() << std::endl;
		std::cout << "m.insert(1, 1.23)\t" << m.insert(1, 1.23) << std::endl;
		std::cout << "m.insert(1, 1.4)\t" << m.insert(1, 1.4) << std::endl;
		std::cout << "m.insert(2, 5.0123)\t" << m.insert(2, 5.0123) << std::endl;
		std::cout << "m.find(1)\t" << m.find(1) << std::endl;
		std::cout << "m.find(2)\t" << m.find(2) << std::endl;
	} catch(const std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
	} catch(...) {
		std::cerr << "Unknown exception" << std::endl;
	}
}


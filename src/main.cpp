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
#include <thread>

int main(const int argc, const char *argv[]) {
	try {
		struct my_hasher {
			uint32_t operator()(const int& i) {
				return static_cast<uint32_t>(i);
			}
		};

		ema::hmap<int, double, my_hasher, 256*1024>	m;
		std::cout << "m.mem_size()\t" << m.mem_size() << std::endl;
		std::cout << "m.insert(1, 1.23)\t" << m.insert(1, 1.23) << std::endl;
		std::cout << "m.insert(1, 1.4)\t" << m.insert(1, 1.4) << std::endl;
		std::cout << "m.insert(2, 5.0123)\t" << m.insert(2, 5.0123) << std::endl;
		std::cout << "m.find(1)\t" << m.find(1) << std::endl;
		std::cout << "m.find(2)\t" << m.find(2) << std::endl;

		auto fn_thread_test = [&m](const uint32_t b, const uint32_t e) -> void {
			for(uint32_t s = b; s < e; ++s) {
				m.insert(s, s + 1.23456);
			}
		};

		const int	N_THREAD = 4;
		std::thread	*th[N_THREAD];
		for(int i = 0; i < N_THREAD; ++i) {
			th[i] = new std::thread(fn_thread_test, i*2048, (i*2048)+(256*1024));
		}
		for(int i = 0; i < N_THREAD; ++i) {
			th[i]->join();
			delete th[i];
		}
		// print out stats for used buckets
		size_t	buckets[8];
		m.used_buckets(buckets);
		for(size_t i = 0; i < sizeof(buckets)/sizeof(size_t); ++i)
			std::cout << i << "\t" << buckets[i] << std::endl;
	} catch(const std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
	} catch(...) {
		std::cerr << "Unknown exception" << std::endl;
	}
}


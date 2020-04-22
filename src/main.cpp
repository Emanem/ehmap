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

		typedef ema::hmap<int, double, my_hasher, 128*1024>	MY_MAP;

		MY_MAP	m;
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

		// this test is particularly good to stress the
		// wrong usage of this type of map
		const int	N_THREAD = std::thread::hardware_concurrency();
		std::thread	*th[N_THREAD];
		for(int i = 0; i < N_THREAD; ++i) {
			th[i] = new std::thread(fn_thread_test, i*2048, (i*2048)+(256*1024));
		}
		for(int i = 0; i < N_THREAD; ++i) {
			th[i]->join();
			delete th[i];
		}
		// print out stats for used buckets
		MY_MAP::stats	s;
		m.get_stats(s);
		size_t		total = 0;
		std::cout << "Elems/Bucket\tTotal\n";
		for(size_t i = 0; i < sizeof(s.els_per_bucket)/sizeof(size_t); ++i) {
			std::cout << i << "\t\t" << s.els_per_bucket[i] << std::endl;
			total += i * s.els_per_bucket[i];
		}
		std::cout << "s.unused_pairs\t" << s.unused_pairs << std::endl;
		std::cout << "s.all_pairs\t" << s.all_pairs << std::endl;
		std::cout << "total      \t" << total << std::endl;
	} catch(const std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
	} catch(...) {
		std::cerr << "Unknown exception" << std::endl;
	}
}


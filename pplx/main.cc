#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include <iostream>
#include <memory>
#include <cstdlib>
#include <ctime>
#include <mutex>
#include <set>
#include <vector>
#include <unordered_map>

#include "context_thread_pool.h"

namespace asio = boost::asio;
using error_code = boost::system::error_code;

using results_t = std::unordered_map<uint32_t, std::vector<int> >;

size_t thread_id()
{
	return static_cast<size_t>(std::hash<std::thread::id>()(std::this_thread::get_id()));
}

auto now()
{
	return std::chrono::high_resolution_clock::now();
}

#define PRINT_ARGS(msg) do { \
   std::lock_guard<std::mutex> lg(mtx); \
   std::cout << '[' << std::this_thread::get_id() \
             << "] " << msg << std::endl; \
 } while (0)


int main()
{
	std::srand(std::time(nullptr));

	std::mutex mtx;

	ContextThreadPool::initialize_with_threads(4);

	results_t results;

	for (int n = 0; n < 10000; ++n)
	{
		ContextThreadPool::instance().schedule([&mtx, n, &results]()
		{
			PRINT_ARGS(n);
//			std::lock_guard<std::mutex> lg(mtx);
//			results[thread_id()].push_back(n);
		});
	}

	ContextThreadPool::instance().join();

	for (auto& kv : results)
	{
		std::cout << "thread " << kv.first << ": ";
		for (auto& n : kv.second)
		{
			std::cout << n << ' ';
		}
		std::cout << std::endl;
	}
}

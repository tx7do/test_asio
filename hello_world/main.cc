#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/date_time.hpp>

#include <iostream>
#include <memory>
#include <cstdlib>
#include <ctime>

#include "context_pool.h"

namespace asio = boost::asio;
using error_code = boost::system::error_code;

typedef std::unique_ptr<asio::io_context::work> work_ptr;

auto now()
{
	return std::chrono::high_resolution_clock::now();
}

#define PRINT_ARGS(msg) do { \
   boost::lock_guard<boost::mutex> lg(mtx); \
   std::cout << '[' << boost::this_thread::get_id() \
             << "] " << msg << std::endl; \
 } while (0)

// 测试：1上下文，1线程
void test_1c_1t()
{
	asio::io_context ctx;

	ctx.post(
		[]
		{
			std::cout << std::this_thread::get_id() << " >Hello, world!" << "<<<<-" << std::endl;
		});

	std::cout << std::this_thread::get_id() << " >Greetings: ->>>>" << std::endl;
	ctx.run();
}

// 测试：1上下文，1线程，dispatch
void test_1c_1t_dispatch()
{
	asio::io_context ctx;

	ctx.dispatch([]()
	{ std::cout << std::this_thread::get_id() << " dispatch 1" << std::endl; });

	ctx.post(
		[&ctx]
		{
			std::cout << std::this_thread::get_id() << " post 1" << std::endl;
			ctx.dispatch([]
			{
				std::cout << std::this_thread::get_id() << " dispatch 2" << std::endl;
			});
		});

	ctx.post([]
	{ std::cout << std::this_thread::get_id() << " post 2" << std::endl; });

	ctx.run();
}

void test_1c_1t_dispatch_priority()
{
	asio::io_context ctx;

	ctx.dispatch([]()
	{ std::cout << std::this_thread::get_id() << " dispatch 1" << std::endl; });

	ctx.post([]
	{ std::cout << std::this_thread::get_id() << " post 1" << std::endl; });

	ctx.post([]
	{ std::cout << std::this_thread::get_id() << " post 2" << std::endl; });

	ctx.post([]
	{ std::cout << std::this_thread::get_id() << " post 3" << std::endl; });

	ctx.dispatch([]()
	{ std::cout << std::this_thread::get_id() << " dispatch 2" << std::endl; });

	ctx.run();
}

void test_1c_mt_dispatch_priority()
{
	// handler 将在run()的工作线程中执行。
	// post将handler入列之后，就返回了，不会等待handler执行完成，handler将在run()的工作线程中执行。
	// 如果调用dispatch的是run()的工作线程，则handler将在run()的工作线程中立即执行；如果调用dispatch的不是run()的工作线程，则相当于是post。
	// post方法永远都不会直接调用handler。

	asio::io_context ctx;
	boost::mutex mtx;

	std::cout << '[' << boost::this_thread::get_id() << "] " << "main thread" << std::endl;

	ctx.dispatch([&mtx]()
	{
		PRINT_ARGS("dispatch[1]");
	});

	ctx.post([&mtx, &ctx]
	{
		PRINT_ARGS("post[1]");

		ctx.dispatch([&mtx]()
		{ PRINT_ARGS("post[1]dispatch"); });
	});

	ctx.post([&mtx, &ctx]
	{
		PRINT_ARGS("post[2]");

		ctx.dispatch([&mtx]()
		{ PRINT_ARGS("post[2]dispatch"); });
	});

	ctx.post([&mtx]
	{ PRINT_ARGS("post[3]"); });

	ctx.dispatch([&mtx]()
	{ PRINT_ARGS("dispatch[2]"); });

	boost::thread_group pool;
	for (int i = 0; i < 2; ++i)
	{
		pool.create_thread([&ctx]()
		{ ctx.run(); });
	}

	ctx.dispatch([&mtx]()
	{ PRINT_ARGS("main thread[1]dispatch"); });

	pool.join_all();
}

void test_1c_mt()
{
	asio::io_context ctx;

	boost::mutex mtx;

	for (int i = 0; i < 20; ++i)
	{
		ctx.post([i, &mtx]()
		{
			PRINT_ARGS("Handler[" << i << "]");
			boost::this_thread::sleep(boost::posix_time::seconds(1));
		});
	}

	boost::thread_group pool;
	for (int i = 0; i < 4; ++i)
	{
		pool.create_thread([&ctx]()
		{ ctx.run(); });
	}

	pool.join_all();
}

void test_single_context_multi_thread_with_guard()
{
	asio::io_context ctx;

	work_ptr work(new asio::io_service::work(ctx));
	boost::mutex mtx;

	boost::thread_group workers;
	for (int i = 0; i < 3; ++i)
	{
		workers.create_thread([&ctx, &mtx]()
		{
			PRINT_ARGS("Starting worker thread ");
			ctx.run();
			PRINT_ARGS("Worker thread done");
		});
	}

	// Post work
	for (int i = 0; i < 20; ++i)
	{
		ctx.post(
			[&ctx, &mtx]()
			{
				PRINT_ARGS("Hello, world!");
				ctx.post([&mtx]()
				{
					PRINT_ARGS("Hola, mundo!");
				});
			});
	}

	work.reset();
	workers.join_all();
}

void test_single_context_multi_thread_with_strand()
{
	asio::io_context ctx;
	asio::io_service::strand strand(ctx);

	work_ptr work(new asio::io_service::work(ctx));
	boost::mutex mtx;

	size_t regular = 0, on_strand = 0;

	auto workFuncStrand = [&mtx, &on_strand]
	{
		++on_strand;
		PRINT_ARGS(on_strand << ". Hello, from strand!");
		boost::this_thread::sleep(
			boost::posix_time::seconds(2));
	};

	auto workFunc = [&mtx, &regular]
	{
		PRINT_ARGS(++regular << ". Hello, world!");
		boost::this_thread::sleep(
			boost::posix_time::seconds(2));
	};

	for (int i = 0; i < 15; ++i)
	{
		if (rand() % 2 == 0)
		{
			ctx.post(strand.wrap(workFuncStrand));
		}
		else
		{
			ctx.post(workFunc);
		}
	}

	boost::thread_group workers;
	for (int i = 0; i < 3; ++i)
	{
		workers.create_thread([&ctx, &mtx]()
		{
			PRINT_ARGS("Starting worker thread ");
			ctx.run();
			PRINT_ARGS("Worker thread done");
		});
	}

	workers.join_all();
}

void async_wait(asio::high_resolution_timer& timer, std::chrono::high_resolution_clock::time_point& lastTime)
{
	timer.expires_after(std::chrono::seconds(1));

	timer.async_wait([&](error_code ec)
	{
		if (ec == asio::error::operation_aborted)
		{
			return;
		}
		auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now() - lastTime).count();
		lastTime = now();
		std::cout << elapsed << "\n";
		async_wait(timer, lastTime);
	});
}

void test_high_resolution_timer()
{
	asio::io_context ctx;

	asio::high_resolution_timer timer(ctx);
	auto lastTime = now();

	async_wait(timer, lastTime);

	ctx.run();
}

void test_context_pool()
{
	IOContextPool ctx_pool(std::thread::hardware_concurrency() * 2);
	auto ctx = ctx_pool.query();
	asio::ip::tcp::socket socket(ctx->get_executor());
	ctx_pool.run();
}

void test_simple_udp_server()
{
	std::uint16_t port = 15001;
	asio::io_context ctx;
	asio::ip::udp::socket socket(ctx);
	asio::ip::udp::endpoint endpoint(asio::ip::udp::v4(), port);

	for(;;)
	{
		char buffer[65536];
		boost::asio::ip::udp::endpoint sender;
		std::size_t bytes_transferred = socket.receive_from(boost::asio::buffer(buffer), sender);
		socket.send_to(boost::asio::buffer(buffer, bytes_transferred), sender);
	}
};

int main()
{
	std::srand(std::time(nullptr));

//	test_1c_1t();
//	test_1c_1t_dispatch();
//	test_1c_1t_dispatch_priority();
//	test_1c_mt_dispatch_priority();
//	test_single_context_multi_thread();
//	test_single_context_multi_thread_with_guard();
//	test_single_context_multi_thread_with_strand();
//	test_high_resolution_timer();
	test_context_pool();
}
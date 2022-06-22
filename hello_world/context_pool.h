#pragma once

#include <memory>
#include <thread>
#include <vector>

#include <boost/asio.hpp>
#include <boost/thread.hpp>

namespace asio = boost::asio;

template<typename T>
using vector_ptr = std::vector<std::shared_ptr<T>>;

class IOContextPool
{
public:
	using context_t = asio::io_context;
	using context_ptr = std::shared_ptr<context_t>;

	using thread_t = boost::thread;
	using thread_ptr = std::shared_ptr<thread_t>;

	using work_guard_t = asio::executor_work_guard<asio::io_context::executor_type>;
	using work_guard_ptr = std::shared_ptr<work_guard_t>;

	struct context_pair
	{
		context_ptr context = nullptr;
		thread_ptr thread = nullptr;
		work_guard_ptr work_guard = nullptr;
	};
	using context_pair_queue = vector_ptr<context_pair>;

	explicit IOContextPool(std::size_t size = std::thread::hardware_concurrency())
		: _index(0)
	{
		if (size <= 0)
		{
			size = std::thread::hardware_concurrency();
		}

		for (std::size_t i = 0; i < size; ++i)
		{
			auto ctx = std::make_shared<context_pair>();
			ctx->context = std::make_shared<context_t>();
			ctx->work_guard = std::make_shared<work_guard_t>(ctx->context->get_executor());
			_contexts.push_back(std::move(ctx));
		}
	}
	IOContextPool(const IOContextPool&) = delete;
	IOContextPool& operator=(const IOContextPool&) = delete;

	void run()
	{
		for(auto& ctx : _contexts)
		{
			ctx->thread = std::make_shared<thread_t>([&]
			{
				ctx->context->run();
			});
		}

		for(auto& ctx : _contexts)
		{
			ctx->thread->join();
		}
	}

	context_ptr query()
	{
		auto& ctx = _contexts[_index++ % _contexts.size()];
		return ctx->context;
	}
private:
	context_pair_queue _contexts;
	std::atomic<std::size_t> _index = 0;
};

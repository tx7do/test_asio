#include "context_thread_pool.h"
#include <memory>

enum
{
	default_thread_num = 40,
};

ContextThreadPool::ptr_t ContextThreadPool::_instance = nullptr;

ContextThreadPool::ContextThreadPool(size_t num_threads)
	: _context(static_cast<int>(num_threads))
	, _work(_context)
{
	for (size_t i = 0; i < num_threads; ++i)
	{
		add_thread();
	}
}

ContextThreadPool::~ContextThreadPool()
{
	_context.stop();
	join();
}

void ContextThreadPool::initialize_instance(size_t num_threads)
{
	static std::once_flag of;

	std::call_once(of, [num_threads]()
	{
		_instance = std::make_unique<ContextThreadPool>(num_threads);
	});
}

ContextThreadPool& ContextThreadPool::instance()
{
	initialize_instance(default_thread_num);
	return *(_instance.get());
}

void ContextThreadPool::initialize_with_threads(size_t num_threads)
{
	initialize_instance(num_threads);
}

void ContextThreadPool::add_thread()
{
	_threads.push_back(
		std::make_unique<thread_t>([this]
		{
			//auto work_guard = asio::make_work_guard(_context);
			_context.run();
		})
	);
}

void ContextThreadPool::join()
{
	for (auto& t : _threads)
	{
		t->join();
	}
}

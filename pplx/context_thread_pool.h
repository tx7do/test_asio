#pragma once

#include <boost/asio.hpp>
#include <boost/thread.hpp>

namespace asio = boost::asio;

class ContextThreadPool
{
	using context_t = asio::io_context;

	using thread_t = boost::asio::detail::thread;
	using thread_ptr = std::unique_ptr<thread_t>;
	using thread_vector = std::vector<thread_ptr>;

	using ptr_t = std::unique_ptr<ContextThreadPool>;

public:
	explicit ContextThreadPool(size_t num_threads);
	~ContextThreadPool();

	ContextThreadPool(const ContextThreadPool&) = delete;
	ContextThreadPool& operator=(const ContextThreadPool&) = delete;

public:
	inline context_t& context()
	{
		return _context;
	}

	void join();

	template<typename T>
	void schedule(T task)
	{
		context().post(task);
	}

public:
	/// 获取单体对象
	static ContextThreadPool& instance();

	/// 使用给定的线程数量初始化
	static void initialize_with_threads(size_t num_threads);

protected:
	/// 根据线程数量创建线程
	void add_thread();

	/// 初始化单体对象
	static void initialize_instance(size_t num_threads);

protected:
	context_t _context;
	thread_vector _threads;
	asio::io_context::work _work;

	static ptr_t _instance;
};

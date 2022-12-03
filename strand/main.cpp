#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>

namespace asio = boost::asio;
using error_code = boost::system::error_code;

class printer
{
	using timer_t = asio::high_resolution_timer;
	using timer_ptr = std::shared_ptr<timer_t>;

public:
	explicit printer(asio::io_context& ioc)
		: _strand(ioc),
		  _timer(std::make_shared<timer_t>(ioc)),
		  _count(0)
	{
		startPrint();
	}

	~printer()
	{
		std::cout << "Final count is " << _count << std::endl;
	}

	void print()
	{
		std::cout << "Timer 1: " << _count << "  thread id：" << std::this_thread::get_id() << std::endl;
		++_count;

		startPrint();
	}

private:
	void startPrint()
	{
		_timer->expires_after(std::chrono::milliseconds(1000));

		if (_withStrand)
		{
			_timer->async_wait(_strand.wrap([this](error_code ec)
			{
				print();
			}));
		}
		else
		{
			_timer->async_wait([this](error_code ec)
			{
				print();
			});
		}
	}

private:
	asio::io_service::strand _strand;
	timer_ptr _timer;
	std::atomic_int64_t _count;
	std::atomic_bool _withStrand{ false };
};

// https://www.crazygaze.com/blog/2016/03/17/how-strands-work-and-why-you-should-use-them/
// https://www.jianshu.com/p/70286c2ab544

int main()
{
	asio::io_context ioc;
	printer p(ioc);

	std::shared_ptr<std::thread> threads[2];

	for (auto& i : threads)
	{
		i = std::make_shared<std::thread>([&ioc]()
		{
			auto work_guard = asio::make_work_guard(ioc);
			ioc.run();
		});
	}

	for (auto& i : threads)
	{
		i->join();
	}

	getchar();
	return 0;
}

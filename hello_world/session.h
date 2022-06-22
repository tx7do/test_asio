#pragma once

#include <memory>
#include <thread>
#include <vector>

#include <boost/asio.hpp>
#include <boost/thread.hpp>

namespace asio = boost::asio;
using asio::ip::tcp;
using asio::io_context;
using work_guard_t = asio::executor_work_guard<asio::io_context::executor_type>;
using error_code = boost::system::error_code;

class session : public std::enable_shared_from_this<session>
{
	using context_t = asio::io_context;
	using context_ptr = std::shared_ptr<context_t>;

	session(context_t& io_context)
		: socket(io_context), read_strand(io_context), write_strand(io_context)
	{
	}

	void async_read()
	{
		asio::async_read(socket,
			read_buffer,
			boost::asio::bind_executor(read_strand, [&](error_code ec, std::size_t bytes_transferred)
			{
				if (!ec)
				{
					async_read();
				}
			}));
	}

	void async_write()
	{
		asio::async_read(socket,
			write_buffer,
			asio::bind_executor(write_strand, [&](error_code ec, std::size_t bytes_transferred)
			{
				if (!ec)
				{
					async_write();
				}
			}));
	}

private:

	tcp::socket socket;

	io_context::strand write_strand;
	io_context::strand read_strand;

	boost::asio::streambuf write_buffer;
	boost::asio::streambuf read_buffer;
};

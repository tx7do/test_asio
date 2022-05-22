// https://www.boost.org/doc/libs/1_71_0/doc/html/boost_asio/tutorial/tutdaytime1/src.html

#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

static const std::string HOST = "localhost";
static const std::string PORT = "1300";

int main() {
    try {

        boost::asio::io_context io_context;

        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(HOST, PORT);

        tcp::socket socket(io_context);
        boost::asio::connect(socket, endpoints);

        for (;;) {
            boost::array<char, 128> buf{};
            boost::system::error_code ec;

            size_t len = socket.read_some(boost::asio::buffer(buf), ec);

            if (ec == boost::asio::error::eof) {
                break; // Connection closed cleanly by peer.
            } else if (ec) {
                throw boost::system::system_error(ec); // Some other ec.
            }

            std::cout.write(buf.data(), (std::streamsize) len);
        }
    }
    catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}

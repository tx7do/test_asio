# ASIO + Protobuf

## 解决方案1

```c++
PlayerInfo info;
info.set_name(name);
// ...

boost::asio::streambuf b;
std::ostream os(&b);
info.SerializeToOstream(&os);

boost::asio::write(*sock, b);
```


## 解决方案2

```c++
#include "boost/asio.hpp"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"

using ::boost::asio::ip::udp;

int main() {
  PlayerInfo message;
  message.set_name("Player 1");
  // ...

  const boost::asio::ip::address_v4 kIpAddress = boost::asio::ip::address_v4::loopback();
  const unsigned short kPortNumber = 65535;

  try {
    boost::asio::io_service io_service;
    udp::socket socket(io_service, boost::asio::ip::udp::v4());

    udp::endpoint endpoint(kIpAddress, kPortNumber);
    boost::system::error_code error;

    boost::asio::streambuf stream_buffer;
    std::ostream output_stream(&stream_buffer);

    {
      ::google::protobuf::io::OstreamOutputStream raw_output_stream(&output_stream);
      ::google::protobuf::io::CodedOutputStream coded_output_stream(&raw_output_stream);
      coded_output_stream.WriteVarint32(message.ByteSize());

      message.SerializeToCodedStream(&coded_output_stream);
      // IMPORTANT: In order to flush a CodedOutputStream it has to be deleted,
      // otherwise a 0 bytes package is send over the wire.
    }
  }

  size_t len = socket.send_to(stream_buffer.data(), endpoint, 0, error);

  if (error && error != boost::asio::error::message_size) {
    throw boost::system::system_error(error);
  }

  std::cout << "Sent " << len << " bytes data to " << kIpAddress.to_string() << "." << std::endl;
} catch (const std::exception& ex) {
  std::cerr << ex.what() << std::endl;
}
```

## 解决方案3

```c++
boost::local_shared_ptr<asio::streambuf> wbuf;
asio::async_write(sock, *wbuf, [&, wbuf](const asio::error_code &ec, std::size_t len){});
```

## 参考资料

* [Sending Protobuf Messages with boost::asio](https://stackoverflow.com/questions/4810026/sending-protobuf-messages-with-boostasio)
* [使用protobuf设计消息协议(C++asio网络库相关）](https://blog.csdn.net/qq_39885372/article/details/104082698)

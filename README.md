# Asio测试程序

项目运行于WSL Ubuntu之下。

* Asio 的Linux Epoll实现，使用的是水平触发模式（Level Trigger）。

1. 单context，单thread；
2. 单context，多thread；
3. 多context，多thread（一对一）。

cpprest采用的是单context，多thread。  
MongoDB采用的是：acceptor一个context一个thread；每个连接也是一个context一个thread。

## Ubuntu安装依赖项

```bash
# 开发必要软件包，包含：dpkg-dev fakeroot g++ g++-4.6 libalgorithm-diff-perl libalgorithm-diff-xs-perl libalgorithm-merge-perl libdpkg-perl libstdc++6-4.6-dev libtimedate-perl
sudo apt-get install build-essential

# build tools
sudo apt-get install cmake gcc clang gdb llvm lldb llvm-dev liblldb-dev

# Boost.Asio
sudo apt-get install libboost-dev

# Asio only
sudo apt-get install -y libasio-dev

# Protocol Buffers
sudo apt-get install libprotobuf-dev

# protoc
sudo apt install protobuf-compiler
```

## post和dispatch

首先以伪代码描述其功能：

```c++
void post(Handler handler)
{
  _queue.push(handler);
}

void dispatch(Handler handler)
{
  if (can_execute())
    handler();
  else
    post(handler);
}

void run()
{
	_work_thrd_id = boost::this_thread::get_id();
	while (!_queue.empty())
    {
        Handler handler = _queue.front();
        _queue.pop();
        handler();
    }
}

bool can_execute()
{
  return _work_thrd_id == boost::this_thread::get_id();
}
```

1. post永远都不会直接调用handler，而是将handler放入队列中，等待执行。
2. dispatch则是判断是否可以直接执行handler，如果可以执行，则直接执行，否则将handler放入队列中等待执行。
3. run是一个循环，每次循环都会从队列中取出一个handler，然后执行。

## strand

类boost::asio::io_context::strand的主要作用是在asio中利用多线程进行事件处理的时候，如果涉及到多线程访问共享资源，借助于strand类，我们不需要显示的使用线程同步相关的类（比如mutex）就可以让多个事件处理函数依次执行。 简而言之，strand定义了事件处理程序的严格顺序调用。

## 参考文档

* [asio的异步与线程模型解析](https://www.cnblogs.com/ishen/p/14593598.html)
* [浅谈 Boost.Asio 的多线程模型](http://senlinzhan.github.io/2017/09/17/boost-asio/)
* [Boost.Asio基本原理](https://mmoaay.gitbooks.io/boost-asio-cpp-network-programming-chinese/content/Chapter2.html)
* [Asio实现浅析](https://zhuanlan.zhihu.com/p/55503053)
* [A Priority Queue with Boost.Asio](https://zhuanlan.zhihu.com/p/87400227)
* [Task Execution with Asio](https://www.packt.com/task-execution-asio/)
* [Event Loop](https://gist.github.com/kassane/f2330ef44b070f4a5fa9d59c770f68e9)
* [Boost asio io_service dispatch vs post](https://stackoverflow.com/questions/2326588/boost-asio-io-service-dispatch-vs-post)
* [To post or to dispatch?](http://thisthread.blogspot.com/2011/06/to-post-or-to-dispatch.html)
* [Multithreaded execution with asio, part 1](https://dens.website/tutorials/cpp-asio/multithreading)
* [Multithreaded execution with asio, part 2](https://dens.website/tutorials/cpp-asio/multithreading-2)
* [【翻译】为何我们要使用boost strands](https://www.jianshu.com/p/70286c2ab544)
* [Strands](https://zhuanlan.zhihu.com/p/87388918)
* [How strands work and why you should use them](http://www.crazygaze.com/blog/2016/03/17/how-strands-work-and-why-you-should-use-them/)
* [【翻译】为何我们要使用boost strands](https://www.crazygaze.com/blog/2016/03/17/how-strands-work-and-why-you-should-use-them/)
* [How strands work and why you should use them](https://www.jianshu.com/p/70286c2ab544)

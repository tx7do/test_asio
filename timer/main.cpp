// https://github.com/chriskohlhoff/asio/blob/master/asio/src/tests/unit/system_timer.cpp

#include <cstdio>

// Disable autolinking for unit tests.
#if !defined(BOOST_ALL_NO_LIB)
#define BOOST_ALL_NO_LIB 1
#endif // !defined(BOOST_ALL_NO_LIB)

// Prevent link dependency on the Boost.System library.
#if !defined(BOOST_SYSTEM_NO_DEPRECATED)
#define BOOST_SYSTEM_NO_DEPRECATED
#endif // !defined(BOOST_SYSTEM_NO_DEPRECATED)

// Test that header file is self-contained.
#include "asio/system_timer.hpp"

//#include "asio/bind_cancellation_slot.hpp"
//#include "asio/cancellation_signal.hpp"
#include "asio/executor_work_guard.hpp"
#include "asio/io_context.hpp"
#include "asio/thread.hpp"

#if defined(ASIO_HAS_BOOST_BIND)

# include <boost/bind/bind.hpp>

#else // defined(ASIO_HAS_BOOST_BIND)
# include <functional>
#endif // defined(ASIO_HAS_BOOST_BIND)

#if defined(ASIO_HAS_BOOST_BIND)
namespace bindns = boost;
#else // defined(ASIO_HAS_BOOST_BIND)
namespace bindns = std;
#endif // defined(ASIO_HAS_BOOST_BIND)

void increment(int *count) {
    ++(*count);
}

void increment_if_not_cancelled(int *count,
                                const asio::error_code &ec) {
    if (!ec) {
        ++(*count);
    }
}

void cancel_timer(asio::system_timer *t) {
    std::size_t num_cancelled = t->cancel();
}

void cancel_one_timer(asio::system_timer *t) {
    std::size_t num_cancelled = t->cancel_one();
}

void io_context_run(asio::io_context *ioc) {
    ioc->run();
}

asio::system_timer::time_point now() {
    return asio::system_timer::clock_type::now();
}

void system_timer_test() {
    using asio::chrono::seconds;
    using asio::chrono::microseconds;
    using bindns::placeholders::_1;
    using bindns::placeholders::_2;

    asio::io_context ioc;
    const asio::io_context::executor_type ioc_ex = ioc.get_executor();
    int count = 0;

    asio::system_timer::time_point start = now();

    asio::system_timer t1(ioc, seconds(1));
    t1.wait();

    // The timer must block until after its expiry time.
    asio::system_timer::time_point end = now();
    asio::system_timer::time_point expected_end = start + seconds(1);

    start = now();

    asio::system_timer t2(ioc_ex, seconds(1) + microseconds(500000));
    t2.wait();

    // The timer must block until after its expiry time.
    end = now();
    expected_end = start + seconds(1) + microseconds(500000);

    t2.expires_at(t2.expiry() + seconds(1));
    t2.wait();

    // The timer must block until after its expiry time.
    end = now();
    expected_end += seconds(1);

    start = now();

    t2.expires_after(seconds(1) + microseconds(200000));
    t2.wait();

    // The timer must block until after its expiry time.
    end = now();
    expected_end = start + seconds(1) + microseconds(200000);

    start = now();

    asio::system_timer t3(ioc, seconds(5));
    t3.async_wait([&](const asio::error_code &) {
        increment(&count);
    });

    ioc.run();

    // The run() call will not return until all operations have finished, and
    // this should not be until after the timer's expiry time.
    ASIO_CHECK(count == 1);
    end = now();
    expected_end = start + seconds(1);
    ASIO_CHECK(expected_end < end || expected_end == end);

    count = 3;
    start = now();

    asio::system_timer t4(ioc, seconds(1));
    t4.async_wait(bindns::bind(decrement_to_zero, &t4, &count));

    // No completions can be delivered until run() is called.
    ASIO_CHECK(count == 3);

    ioc.restart();
    ioc.run();

    // The run() call will not return until all operations have finished, and
    // this should not be until after the timer's final expiry time.
    ASIO_CHECK(count == 0);
    end = now();
    expected_end = start + seconds(3);
    ASIO_CHECK(expected_end < end || expected_end == end);

    count = 0;
    start = now();

    asio::system_timer t5(ioc, seconds(10));
    t5.async_wait(bindns::bind(increment_if_not_cancelled, &count, _1));
    asio::system_timer t6(ioc, seconds(1));
    t6.async_wait(bindns::bind(cancel_timer, &t5));

    ioc.restart();
    ioc.run();

    // The timer should have been cancelled, so count should not have changed.
    // The total run time should not have been much more than 1 second (and
    // certainly far less than 10 seconds).
    end = now();
    expected_end = start + seconds(2);

    // Wait on the timer again without cancelling it. This time the asynchronous
    // wait should run to completion and increment the counter.
    t5.async_wait(bindns::bind(increment_if_not_cancelled, &count, _1));

    ioc.restart();
    ioc.run();

    // The timer should not have been cancelled, so count should have changed.
    // The total time since the timer was created should be more than 10 seconds.
    end = now();
    expected_end = start + seconds(10);

    count = 0;
    start = now();

    // Start two waits on a timer, one of which will be cancelled. The one
    // which is not cancelled should still run to completion and increment the
    // counter.
    asio::system_timer t7(ioc, seconds(3));
    t7.async_wait([&](const asio::error_code &ec) { increment_if_not_cancelled(&count, ec); });
    t7.async_wait([&](const asio::error_code &ec) { increment_if_not_cancelled(&count, ec); });
    asio::system_timer t8(ioc, seconds(1));
    t8.async_wait(bindns::bind(cancel_one_timer, &t7));

    ioc.restart();
    ioc.run();

    // One of the waits should not have been cancelled, so count should have
    // changed. The total time since the timer was created should be more than 3
    // seconds.
    end = now();
    expected_end = start + seconds(3);
}

void system_timer_thread_test() {
    asio::io_context ioc;
    asio::executor_work_guard<asio::io_context::executor_type> work
            = asio::make_work_guard(ioc);
    asio::system_timer t1(ioc);
    asio::system_timer t2(ioc);
    int count = 0;

    asio::thread th([capture0 = &ioc] { return io_context_run(capture0); });

    t2.expires_after(asio::chrono::seconds(2));
    t2.wait();

    t1.expires_after(asio::chrono::seconds(2));
    t1.async_wait([&](const asio::error_code &) {
        increment(&count);
    });

    t2.expires_after(asio::chrono::seconds(4));
    t2.wait();

    ioc.stop();
    th.join();
}


int main() {
    system_timer_thread_test();
}

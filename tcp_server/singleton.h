#pragma once

#include <boost/noncopyable.hpp>
#include <boost/thread/once.hpp>
#include <boost/scoped_ptr.hpp>

template<typename T>
class Singleton : boost::noncopyable
{
  static void initialize()
  {
    m_pointer.reset(new T);
  }

public:
  static T* get()
  {
    boost::call_once(m_flag, initialize);
    return m_pointer.get();
  }

private:
  static boost::scoped_ptr<T> m_pointer;

  static boost::once_flag m_flag;
};

template<typename T>
boost::scoped_ptr<T> Singleton<T>::m_pointer;

template<typename T>
boost::once_flag Singleton<T>::m_flag = BOOST_ONCE_INIT;

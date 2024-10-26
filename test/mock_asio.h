// This file contains the mocks and interceptors for boost asio functions.
//
#ifndef __MOCK_ASIO_HPP__
#define __MOCK_ASIO_HPP__

#include <boost/asio.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>

// types for boost template parameters in Mocks, Fakes, and Interceptors.
using BError = boost::system::error_code;
using BSocket = boost::asio::ip::tcp::socket;
using BStreamBuf = boost::asio::basic_streambuf<std::allocator<char>>;
using BSyncRdStream =
    boost::asio::basic_stream_socket<boost::asio::ip::tcp,
                                     boost::asio::any_io_executor>;
using BCompletionCond = boost::asio::detail::transfer_exactly_t;
using BSyncWrStream =
    boost::asio::basic_stream_socket<boost::asio::ip::tcp,
                                     boost::asio::any_io_executor>;
using BConstBufSeqType = boost::asio::mutable_buffers_1;

// Mock boost::asio interfaces for testing purposes
class MockAsio {
public:
  MockAsio() {
    assert(inst_ == nullptr); // shouldn't be another instance
    inst_ = this;             // set this instance as the global one
  }
  ~MockAsio() {
    assert(inst_ == this);
    inst_ = nullptr;
  }

  MOCK_METHOD(std::size_t, read,
              (BSyncRdStream & s, BStreamBuf &b, BCompletionCond cc));
  MOCK_METHOD(std::size_t, read_until,
              (BSocket & s, BStreamBuf &b, std::string_view delim));
  MOCK_METHOD(std::size_t, read_until,
              (BSocket & s, BStreamBuf &b, std::string_view delim, BError &ec));
  MOCK_METHOD(std::size_t, write,
              (BSyncWrStream & s, const BConstBufSeqType &b));

  // return the current instance of the MockAsio class
  static MockAsio &inst() {
    assert(inst_ != nullptr); // can only call when there is valid instance
    return *inst_;
  }

private:
  static MockAsio *inst_; // pointer to current instance of MockAsio if any.
};

MockAsio *MockAsio::inst_ = nullptr;

// These are Interceptor functions for boost asio template functions.The
// interceptor functions use template specializationto overrides the general
// template function definition in the boost library.  With this override, the
// interceptors are able to forward the request to the MockAsio instance.
namespace boost {
namespace asio {

template <>
std::size_t read(BSyncRdStream &s, BStreamBuf &b,
                 BCompletionCond completion_condition) {
  return MockAsio::inst().read(s, b, completion_condition);
}

template <>
std::size_t read_until(BSocket &s, BStreamBuf &b, string_view delim) {
  return MockAsio::inst().read_until(s, b, delim);
}

template <>
std::size_t read_until(BSocket &s, BStreamBuf &b, string_view delim,
                       BError &ec) {
  return MockAsio::inst().read_until(s, b, delim, ec);
}

template <>
typename boost::asio::ip::tcp::endpoint connect(
    basic_socket<boost::asio::ip::tcp, boost::asio::any_io_executor> &s,
    const boost::asio::ip::basic_resolver_results<boost::asio::ip::tcp>
        &endpoints,
    typename constraint<is_endpoint_sequence<
        boost::asio::ip::basic_resolver_results<boost::asio::ip::tcp>>::value>::
        type) {
  boost::asio::ip::tcp::endpoint ep;
  return ep;
}

template <>
std::size_t write(
    BSyncWrStream &s, const BConstBufSeqType &b,
    typename boost::asio::constraint<
        boost::asio::is_const_buffer_sequence<BConstBufSeqType>::value>::type) {
  return MockAsio::inst().write(s, b);
}

} // namespace asio
} // namespace boost

#endif // __MOCK_ASIO_HPP__
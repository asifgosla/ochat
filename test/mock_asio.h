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
  MOCK_METHOD(std::size_t, read,
              (BSyncRdStream & s, BStreamBuf &b, BCompletionCond cc));
  MOCK_METHOD(std::size_t, read_until,
              (BSocket & s, BStreamBuf &b, std::string_view delim));
  MOCK_METHOD(std::size_t, read_until,
              (BSocket & s, BStreamBuf &b, std::string_view delim, BError &ec));
  MOCK_METHOD(std::size_t, write,
              (BSyncWrStream & s, const BConstBufSeqType &b));
};

extern MockAsio *g_mock_asio;

// Interceptors for boost asio interfaces that are templates.
// Intercept the boost::asio::read_until functions here and forward to the mock.
// This boost functions are templates and the template specialization here for
// those functions overrides the general template function definition in the
// boost library.
namespace boost {
namespace asio {

template <>
std::size_t read(BSyncRdStream &s, BStreamBuf &b,
                 BCompletionCond completion_condition) {
  return g_mock_asio->read(s, b, completion_condition);
}

template <>
std::size_t read_until(BSocket &s, BStreamBuf &b, string_view delim) {
  return g_mock_asio->read_until(s, b, delim);
}

template <>
std::size_t read_until(BSocket &s, BStreamBuf &b, string_view delim,
                       BError &ec) {
  return g_mock_asio->read_until(s, b, delim, ec);
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
  return g_mock_asio->write(s, b);
}

} // namespace asio
} // namespace boost
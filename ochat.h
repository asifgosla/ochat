#include <boost/asio.hpp>
#include <string>
#include <vector>

std::string format_post_request(std::string prompt, std::string model_name,
                                bool stream_resp,
                                std::vector<std::string> &history);

void read_until_delimeter(boost::asio::ip::tcp::socket &socket,
                          boost::asio::streambuf &resp_buff, const char *delim);

std::string get_msg_content_from_json(std::string json_str);

void SendRequestToAi(const std::string &req, std::vector<std::string> &history);

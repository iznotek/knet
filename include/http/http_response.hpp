#pragma once
#include <memory>
#include "http_message.hpp"
#include "http_decoder.hpp"

namespace knet {
namespace http {

class HttpResponse {

public:
	HttpResponse(const std::string& rsp = "", uint32_t c = 200) {

		this->status_code = c;
		http_encoder.init_http_message(this);
		http_encoder.content = rsp;
		http_encoder.status_code = c;
	}

	HttpResponse(uint32_t c, const std::string & rsp = "") {
		this->status_code = c;
		http_encoder.init_http_message(this);
		http_encoder.content = rsp;
		http_encoder.status_code = c;
	}

	inline uint32_t parse(const char* data, uint32_t len, bool inplace = false) {
		http_decoder.init_http_message(this); 
		return http_decoder.parse_response(data, len, inplace);
	}

	void add_header(const std::string& key, const std::string& value)
	{
		http_encoder.add_header(key,value); 
	}

	inline uint32_t code() const { return status_code; }

	inline std::string to_string() const { return http_encoder.encode(); }

	bool is_websocket() const {
		return http_decoder.is_websocket();
	};

	inline std::string body() const  { return http_encoder.content; }

	std::string uri;
	std::string content;
	uint32_t status_code = 200;

	private: 

	HttpDecoder<HttpResponse> http_decoder;
	HttpEncoder<HttpResponse> http_encoder;
};
		using HttpResponsePtr = std::shared_ptr<HttpResponse>;

} // namespace http
} // namespace knet

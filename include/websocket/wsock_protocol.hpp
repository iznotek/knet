
//***************************************************************
//	created:	2020/08/01
//	author:		wkui
//***************************************************************


#pragma once
#include "knet.hpp"
#include "klog.hpp"
using namespace knet::tcp; 
namespace knet {
namespace websocket {

struct MinFrameHead {

	uint32_t fin : 1;
	uint32_t rsv1 : 1;
	uint32_t rsv2 : 1;
	uint32_t rsv3 : 1;
	uint32_t opcode : 4;
	uint32_t mask : 1;
	uint32_t len : 7;
	uint64_t xlen;
};

enum  OpCode {
 	ERROR_FRAME = 0xff,
	CONTINUATION = 0x0,
	TEXT_FRAME = 0x1,
	BINARY_FRAME = 0x2,
	WSOCK_CLOSE = 8,
	PING = 9,
	PONG = 0xa,
};

struct WsFrame {
	unsigned int header_size;
	bool fin;
	bool mask;
	enum opcode_type {
		CONTINUATION = 0x0,
		TEXT_FRAME = 0x1,
		BINARY_FRAME = 0x2,
		WSOCK_CLOSE = 8,
		PING = 9,
		PONG = 0xa,
	} opcode;
	int N0;
	uint64_t N;
	uint8_t masking_key[4];
};


 template <class WsConn> 
struct WSockHandler {

	std::function<void(std::shared_ptr<WsConn>)> open = nullptr;
	std::function<void(std::shared_ptr<WsConn>, const std::string & , MessageStatus)> message = nullptr;
};
enum class WSockStatus { WSOCK_INIT, WSOCK_CONNECTING, WSOCK_OPEN, WSOCK_CLOSING, WSOCK_CLOSED };

// enum MessageStatus {
// 	MESSAGE_NONE,
// 	MESSAGE_FULL,
// 	MESSAGE_BEGIN,
// 	MESSAGE_CONT, // continue
// 	MESSAGE_END,
// };

#ifndef ntohll
#define htonll(x) \
	((1 == htonl(1)) ? (x) : ((uint64_t)htonl((x)&0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) \
	((1 == ntohl(1)) ? (x) : ((uint64_t)ntohl((x)&0xFFFFFFFF) << 32) | ntohl((x) >> 32))
#endif

using WSMessageHandler = std::function<void(fmt::string_view , MessageStatus )>;
struct WSMessageReader {

	MinFrameHead head = {0};

	WSMessageReader(uint32_t bufSize = 1024 * 8)
		: buffer_size(bufSize) {}

	uint64_t buffer_size = 2048;
	uint64_t left_length = 0;

	uint64_t payload_length = 0;
	MessageStatus status = MessageStatus::MESSAGE_NONE;

	uint32_t read( const char* pData, uint32_t dataLen, WSMessageHandler handler) {
		dlog("reading websocket message {} status {}", dataLen, status);
		if (dataLen < sizeof(uint16_t)) {
			return 0;
		}

		if (status == MessageStatus::MESSAGE_NONE) {

			uint8_t* vals = (uint8_t*)pData;
			std::cout << "fin is " << ((vals[0] & 0x80) == 0x80) << std::endl;
			std::cout << "opcode is " << (vals[0] & 0x0f) << std::endl;
			std::cout << "length is " << (vals[1] & 0x7F) << std::endl;
			std::cout << "mask is " << ((vals[1] & 0x80) == 0x80) << std::endl;

			head.fin = ((vals[0] & 0x80) == 0x80);
			head.opcode = (vals[0] & 0x0f);
			head.len = (vals[1] & 0x7F);
			head.mask = ((vals[1] & 0x80) == 0x80);

			uint32_t payloadMark = vals[1] & 0x7f;
			uint32_t headSize = sizeof(uint16_t);

			uint32_t maskSize = head.mask ? sizeof(uint32_t) : 0;

			if (payloadMark < 126) {
				this->payload_length = payloadMark;
				if (handler) {
					dlog("payload length is {} head size {} , mask size {}", payload_length, headSize , maskSize); 
					if (head.mask)
					{

						std::string payload ; 
						payload.reserve(payload_length); 

						uint8_t * maskKey = (uint8_t *)pData + headSize; 
						uint32_t payloadPos = headSize + maskSize; 
						for(uint32_t i = 0; i < payload_length; i++) {	
							payload[i] = pData[payloadPos+ i] ^ maskKey[i%4];  
						}
						handler(fmt::string_view(payload.data(), payload_length),  MessageStatus::MESSAGE_NONE);
					} else {

						handler(fmt::string_view((const char*)(pData + headSize + maskSize), payload_length),  MessageStatus::MESSAGE_NONE);
					}
			 
					
				}
				status = MessageStatus::MESSAGE_NONE;
				return payload_length + headSize + maskSize;

			} else if (payloadMark == 126) {
				if (dataLen < headSize + sizeof(uint16_t)) {
					return 0;
				}
				headSize += sizeof(uint16_t);
				payload_length = *(uint16_t*)((uint16_t*)pData + 1);
				payload_length = ntohs(payload_length);

				if (payload_length + headSize + maskSize > buffer_size) {

					left_length = payload_length + headSize + maskSize - buffer_size;
					if (handler) {

						auto ploadLen = (buffer_size - headSize) > payload_length
											? payload_length
											: (buffer_size - headSize);

						if (head.mask)
						{

							std::string payload ; 
							payload.reserve(ploadLen); 
							uint8_t * maskKey = (uint8_t *)pData + headSize; 
							uint32_t payloadPos = headSize + maskSize; 
							for(uint32_t i = 0; i < ploadLen; i++) {	
								payload[i]= pData[payloadPos+ i] ^ maskKey[i%4];  
							}

							handler(fmt::string_view(payload.data(), ploadLen),  MessageStatus::MESSAGE_NONE);
						} else {
							handler(fmt::string_view(pData + headSize+ maskSize, ploadLen),  MessageStatus::MESSAGE_NONE);
						}
						
					}
					this->status = MessageStatus::MESSAGE_CHUNK;
					return dataLen;
				} else {
					if (handler) {

						if (head.mask)
						{
							std::string payload ; 
							payload.reserve(payload_length); 
							uint8_t * maskKey = (uint8_t *)pData + headSize; 
							uint32_t payloadPos = headSize + maskSize; 
							for(uint32_t i = 0; i < payload_length; i++) {	
								payload[i] = pData[payloadPos+ i] ^ maskKey[i%4];  
							}
							handler(fmt::string_view(payload.data(), payload_length),  MessageStatus::MESSAGE_NONE);
						} else {
							handler(fmt::string_view(pData + headSize + maskSize, payload_length),  MessageStatus::MESSAGE_NONE);
						}


						
					}
					status = MessageStatus::MESSAGE_NONE;

					return payload_length + headSize + maskSize;
				}

				// std::cout << "length is " << payload_length << std::endl;
			} else if (payloadMark == 127) {
				if (dataLen < headSize + sizeof(uint64_t)) {
					return 0;
				}

				headSize += sizeof(uint64_t);
				payload_length = *(uint64_t*)((uint16_t*)pData + 1);

				payload_length = ntohll(payload_length);

				if (payload_length + headSize + maskSize > buffer_size) {
					left_length = payload_length + headSize + maskSize - buffer_size;

					if (handler) {
						handler(fmt::string_view(pData + headSize, buffer_size - headSize),  MessageStatus::MESSAGE_NONE);
					}
					this->status = MessageStatus::MESSAGE_CHUNK;
					return dataLen;
				} else {
					if (handler) {

						if (head.mask)
						{
							std::string payload ; 
							payload.reserve(payload_length); 
							uint8_t * maskKey = (uint8_t *)pData + headSize; 
							uint32_t payloadPos = headSize + maskSize; 
							for(uint32_t i = 0; i < payload_length; i++) {	
								payload[i]	=  pData[payloadPos+ i] ^ maskKey[i%4];  
							}
							handler(fmt::string_view(payload.data(), payload_length),  MessageStatus::MESSAGE_NONE);
						} 
						else {
							handler(fmt::string_view(pData + headSize, payload_length),  MessageStatus::MESSAGE_NONE);
						}

						
					}
					status = MessageStatus::MESSAGE_NONE;
					return payload_length + headSize + maskSize;
				}
				// std::cout << "length is " << payloadLen << std::endl;
			}
		} else if (status == MessageStatus::MESSAGE_CHUNK) {

			if (left_length < dataLen) {				
				handler(fmt::string_view(pData, left_length),   MessageStatus::MESSAGE_END);
				auto leftLen = left_length;
				left_length = 0;
				status = MessageStatus::MESSAGE_NONE;
				payload_length = 0;
				return leftLen;
			} else {
				handler(fmt::string_view(pData, dataLen), MessageStatus::MESSAGE_CHUNK);
				left_length -= dataLen;
				status = MessageStatus::MESSAGE_CHUNK;
				return dataLen;
			}
		}

		return 0;
	}
};
} // namespace ws
} // namespace knet

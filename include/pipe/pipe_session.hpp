#pragma once
#include <memory>
#include "pipe_proto.hpp"

namespace knet
{

	namespace pipe
	{

		enum PipeEvent
		{
			PIPE_CONNECT,
			PIPE_DISCONNECT,
			PIPE_HEARTBEAT_REQUEST,
			PIPE_HEARTBEAT_RESPONSE
		};

		class PipeSession : public std::enable_shared_from_this<PipeSession>
		{
		public:
			PipeSession(const std::string &pid = "", const std::string &h = "", uint16_t p = 0)
			{
				m.host = h;
				m.port = p;
				m.pipeid = pid;
				ready_flag = false; 
			}

			virtual ~PipeSession() {}
			virtual bool handle_event(NetEvent evt)
			{				
				return true;
			}
			virtual int32_t handle_message(const std::string_view &msg, uint64_t obdata = 0)
			{				 
				return 0;
			} 

			int32_t send(const std::string &msg, uint64_t obdata = 0 )
			{
				if (is_ready())
				{
					PipeMsgHead head(PIPE_MSG_DATA, msg.length());
					head.data = obdata;
					return connection->msend(head, msg);
				}
				return -1;
			}

			bool is_ready()
			{
				if (ready_flag  && connection)
				{
					return connection->is_connected();
				}else {
					wlog("pipe is not ready, {}",m.pipeid); 
				}
				return false;
			}

			int32_t send(const std::string_view & msg )
			{
				if (is_ready())
				{
					PipeMsgHead head(PIPE_MSG_DATA, msg.length());
					return connection->msend(head, msg);
				}
				return -1;
			}
 

			template <class P, class... Args>
			int32_t msend_with_obdata(uint64_t obdata, const P &first, const Args &...rest)
			{
				if (is_ready())
				{
					uint32_t bodyLen = pipe_data_length(first, rest...);					
					PipeMsgHead head(PIPE_MSG_DATA, bodyLen);
					head.data = obdata;
					return connection->msend(head, first, rest...);
				}
				return -1;
			}

			template <class P, class... Args>
			inline int32_t msend(const P &first, const Args &...rest)
			{
				return msend_with_obdata(0,first ,  rest ...); 			
			}

			void bind(const PipeConnectionPtr &  conn)
			{
				this->connection = conn;
				conn->pipeid = m.pipeid;
				conn->session = this->shared_from_this();
				ready_flag  = true;				 
			}

			void unbind()
			{ 
				ready_flag = false; 
			}

			inline void set_host(const std::string &h)
			{
				m.host = h;
			}

			inline void set_port(uint16_t p)
			{
				m.port = p;
			}

			inline void set_pipeid(const std::string &pid)
			{
				m.pipeid = pid;
			}

			inline void update_pipeid(const std::string &pid)
			{
				if (m.pipeid.empty())
				{
					m.pipeid = pid;
				}
			}

			inline const std::string &get_pipeid() const
			{
				return m.pipeid;
			}
			inline uint16_t get_port() const
			{
				return m.port;
			}

			inline const std::string &get_host() const
			{
				return m.host;
			}

			void on_ready(){
				if (connection && connection->is_connected() ){
					if (connection->is_passive())
					{
						m.hb_timerid = connection->start_timer([this](){ 
							if (connection->is_connected()){
								connection->send_heartbeat(m.pipeid); 
								return true; 
							} 
							return false; 
						}, 3000000); 	
					}
				}				
			}
		private:
			struct
			{
				std::string pipeid;
				std::string host;
				uint16_t port;
				uint64_t hb_timerid; 
			} m;

		 	std::atomic<bool> ready_flag; 
			PipeConnectionPtr connection;
		};

		using PipeSessionPtr = std::shared_ptr<PipeSession>;
	}

}
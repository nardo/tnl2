// Copyright Mark Frohnmayer and GarageGames.  See /license/info.txt in this distribution for licensing terms.

class udp_socket
{
public:
	enum
	{
		default_send_buffer_size = 32768,
		default_recv_buffer_size = 32768,
		max_datagram_size = 1536, ///< some routers have issues with packets larger than this
		recommended_datagram_size = 512,
	};
	
	udp_socket()
	{
		_socket = INVALID_SOCKET;
	}

	~udp_socket()
	{
		unbind();
	}

	enum bind_result
	{
		bind_success,
		address_in_use, ///< A process on this computer is already bound to this address.
		address_invalid, ///< The specified address is not valid for this computer
		initialization_failure,
		socket_allocation_failure,
		generic_failure,
	};
	bind_result bind(const address bind_address, bool accepts_broadcast_packets = true, bool non_blocking_io = true, uint32 send_buffer_size = default_send_buffer_size, uint32 recv_buffer_size = default_recv_buffer_size)
	{
		if(!sockets_init())
			return initialization_failure;
		_socket = socket(AF_INET, SOCK_DGRAM, 0);
		if(_socket == INVALID_SOCKET)
			return socket_allocation_failure;

		SOCKADDR sockaddr;
		bind_address.to_sockaddr(&sockaddr);

		if(::bind(_socket, &sockaddr, sizeof(sockaddr)) == SOCKET_ERROR)
		{
			bind_result return_value;

			#if defined(PLATFORM_WIN32)
			switch(WSAGetLastError())
			{
			case WSAEADDRINUSE:
				return_value = address_in_use;
				break;
			case WSAEADDRNOTAVAIL:
				return_value = address_invalid;
				break;
			case WSAENOBUFS:
				return_value = socket_allocation_failure;
				break;
			default:
				return_value = generic_failure;
				break;
			}
			#else
			#endif
			unbind();
			return return_value;
		}
		if(setsockopt(_socket, SOL_SOCKET, SO_RCVBUF, (char *) &recv_buffer_size, sizeof(recv_buffer_size)) == SOCKET_ERROR || setsockopt(_socket, SOL_SOCKET, SO_SNDBUF, (char *) &send_buffer_size, sizeof(send_buffer_size)) == SOCKET_ERROR)
		{
			unbind();
			return socket_allocation_failure;
		}
		int32 accepts_broadcast = accepts_broadcast_packets;
		uint32 not_block = non_blocking_io;

		#if defined(PLATFORM_WIN32)
		DWORD nb = not_block;
		if(ioctlsocket(_socket, FIONBIO, &nb) ||
		#else
		if(ioctl(_socket, FIONBIO, &not_block) ||
		#endif
				setsockopt(_socket, SOL_SOCKET, SO_BROADCAST, (char *) &accepts_broadcast, sizeof(accepts_broadcast)) == SOCKET_ERROR)
		{
			unbind();
			return generic_failure;
		}
		return bind_success;
	}

	void unbind()
	{
		if(_socket != INVALID_SOCKET)
		{
			closesocket(_socket);
			_socket = INVALID_SOCKET;
		}
	}

	address get_bound_address()
	{
		SOCKADDR sockaddr;
		socklen_t address_size = sizeof(sockaddr);
		getsockname(_socket, &sockaddr, &address_size);
		address ret;
		ret.from_sockaddr(&sockaddr);
		return ret;
	}

	bool is_bound()
	{
		return _socket != INVALID_SOCKET;
	}

	enum send_to_result
	{
		send_to_success,
		send_to_failure,
	};
	send_to_result send_to(const address &the_address, const byte *buffer, uint32 buffer_size)
	{
		SOCKADDR dest_address;
		the_address.to_sockaddr(&dest_address);
		if(sendto(_socket, (const char *) buffer, int(buffer_size), 0, &dest_address, sizeof(dest_address)) == SOCKET_ERROR)
			return send_to_failure;
		return send_to_success;
	}

	enum recv_from_result
	{
		packet_received,
		no_incoming_packets_available,
	};

	recv_from_result recv_from(address *sender_address, byte *buffer, uint32 buffer_size, uint32 *incoming_packet_size)
	{
		SOCKADDR sender_sockaddr;
		socklen_t addr_len = sizeof(sender_sockaddr);
		int32 bytes_read = recvfrom(_socket, (char *) buffer, buffer_size, 0, &sender_sockaddr, &addr_len);
		
		if(bytes_read == SOCKET_ERROR)
			return no_incoming_packets_available;
		*incoming_packet_size = uint32(bytes_read);
		return packet_received;
	}
private:
	SOCKET _socket;
};

static void udp_socket_unit_test()
{
	udp_socket s;
	address bind_address;

	s.bind(bind_address);
	bind_address = s.get_bound_address();

	string addr_string = bind_address.to_string();

	printf("Socket created, bound to address: %s\n", addr_string.c_str());
}
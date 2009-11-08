// Copyright Mark Frohnmayer and GarageGames.  See /license/info.txt in this distribution for licensing terms.

struct address
{
	uint32 _host; ///< ip address in host format
	uint16 _port;

	enum named_address
	{
		null,
		localhost,
		broadcast,
		any
	};
	address(named_address name = any, uint16 port = 0)
	{
		_port = port;
		switch(name)
		{
		case null:
			_host = 0;
			break;
		case localhost:
			_host = 0x7f000001;
			break;
		case broadcast:
			_host = INADDR_BROADCAST;
			break;
		case any:
			_host = INADDR_ANY;
			break;
		}
	}
	/// constructs an address from a string, of the form [transport:]<address>[:port] where transport must be "ip" - leaving future room for ipv6 or ipx.  The constructor may block on a DNS resolution if the name is not a dotted number or one of "broadcast", "localhost" or "any"
	address(const char *address_string, bool dns_lookup = true, uint16 port = 0)
	{
		set(address_string, dns_lookup, port);
	}

	/*address(const address &copy)
	{
		_host = copy._host;
		_port = copy._port;
	}*/

	enum
	{
		max_address_string_len = 255,
	};

	/// sets the address to the specified string, returning true if the string was a valid address.  Note that this call may block if the address portion of the string requires a DNS lookup.
	bool set(const char *string, bool dns_lookup = true, uint16 port = 0)
	{
		_host = INADDR_NONE;
		_port = port;

		if(!sockets_init())
			return false;

		// if "ip:" is in the front of the string, get rid of it.
		if(string[0] == 'i' && string[1] == 'p' && string[2] == ':')
			string += 3;

		if(strlen(string) > max_address_string_len)
			return false;

		char remote_addr[max_address_string_len + 1];
		strcpy(remote_addr, string);

		char *port_string = strchr(remote_addr, ':');
		if(port_string)
		{
			*port_string++ = 0;
			if(!_port)
				_port = atoi(port_string);
		}

		if(!strcmp(remote_addr, "broadcast"))
			_host = INADDR_BROADCAST;
		else if(!strcmp(remote_addr, "localhost"))
			_host = 0x7F000001;
		else if(!strcmp(remote_addr, "any"))
			_host = INADDR_ANY;
		else
		{
			_host = htonl(inet_addr(remote_addr));
			if(_host == INADDR_NONE && dns_lookup)
			{
				array<address> resolved_addresses;
				if(!dns_resolve(remote_addr, resolved_addresses))
					return false;
				_host = resolved_addresses[0].get_host();
			}
		}
		return true;
	}


	void set_host(uint32 host)
	{
		_host = host;
	}
	void set_port(uint16 port)
	{
		_port = port;
	}

	uint16 get_port()
	{
		return _port;
	}

	uint32 get_host()
	{
		return _host;
	}

	string to_string() const
	{
		if(_host == INADDR_BROADCAST)
			return format_string("broadcast:%d", _port);
		else if(_host == INADDR_ANY)
			return format_string("any:%d", _port);
		else if(_host == 0x7f000001)
			return format_string("localhost:%d", _port);
		else
			return format_string("%d.%d.%d.%d:%d", uint8(_host >> 24 ), uint8 (_host >> 16), uint8(_host >> 8), uint8(_host), _port);
	}

	bool operator==(const address &the_address) const
	{
		return _host == the_address._host && _port == the_address._port;
	}

	bool operator!=(const address &the_address) const
	{
		return _host != the_address._host && _port != the_address._port;
	}

	uint32 hash() const
	{
		return _host ^ (uint32(_port) << 8);
	}

	void from_sockaddr(const SOCKADDR *socket_address)
	{
		if(socket_address->sa_family == AF_INET)
		{
			_port = htons(((SOCKADDR_IN *) socket_address)->sin_port);
			_host = htonl(((SOCKADDR_IN *) socket_address)->sin_addr.s_addr);
		}
	}

	void to_sockaddr(SOCKADDR *socket_address) const
	{
		memset(socket_address, 0, sizeof(SOCKADDR_IN));
		((SOCKADDR_IN *) socket_address)->sin_family = AF_INET;
		((SOCKADDR_IN *) socket_address)->sin_port = htons(_port);
		((SOCKADDR_IN *) socket_address)->sin_addr.s_addr = htonl(_host);
	}

	static void get_interface_addresses(array<address> &address_array)
	{
		#ifdef PLATFORM_WIN32
			struct ip_address_row
			{
				uint32 address;
				uint32 index;
				uint32 mask;
				uint32 cast_addr;
				uint32 reasm_size;
				uint16 unused1;
				uint16 unused2;
			};

			struct ip_address_table
			{
				uint32 num_entries;
				ip_address_row table[512];
			};
			typedef uint32 (WINAPI *get_ip_address_table_t)(ip_address_table *,uint32 *, bool);

			static get_ip_address_table_t get_ip_address_table = NULL;

			if(!get_ip_address_table)
			{
				// Under Win32, we use the IP helper library to query all available network interfaces.
				HMODULE module = LoadLibraryA("iphlpapi.dll");
				if(!module)
					return;
				get_ip_address_table = (get_ip_address_table_t) GetProcAddress(module, "GetIpAddrTable");
			}
			ip_address_table ip_table;
			uint32 size = sizeof(ip_table);

			if(get_ip_address_table(&ip_table, &size, 0) == NO_ERROR)
			{
				for(uint32 i = 0; i < ip_table.num_entries; i++)
				{
					uint32 host = htonl(ip_table.table[i].address);
					if(host != 0x7F000001 && host != INADDR_ANY)
					{
						address a;
						a.set_host(host);
						address_array.push_back(a);
					}
				}
			}
		#endif
	}
	static bool dns_resolve(const char *public_name, array<address> &addresses)
	{
		struct hostent *host;
		if((host = gethostbyname(public_name)) == NULL)
			return false;
		else
		{
			int32 count = 0;
			while(host->h_addr_list[count])
			{
				IN_ADDR cp_addr;
				memcpy(&cp_addr, host->h_addr_list[count], sizeof(IN_ADDR));
				address a;
				a.set_host(htonl(cp_addr.s_addr));
				addresses.push_back(a);
				count++;
			}
			return count != 0;
		}
	}

};

template<class stream> bool read(stream &the_stream, address &the_address)
{
	return read(the_stream, the_address._host) && read(the_stream, the_address._port);
}

template<class stream> bool write(stream &the_stream, const address &the_address)
{
	return write(the_stream, the_address._host) && write(the_stream, the_address._port);
}

static void address_unit_test()
{
	printf("Available interface addresses are:\n");
	array<address> address_array;
	address::get_interface_addresses(address_array);
	for(uint32 i = 0; i < address_array.size(); i++)
	{
		string addr_string = address_array[i].to_string();
		printf(" %s\n", addr_string.c_str());
	}
}
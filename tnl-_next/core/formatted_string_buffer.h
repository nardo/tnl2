// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.

class formatted_string_buffer
{
public:
	formatted_string_buffer()
	{
		_dynamic_buffer = 0;
		_len = 0;
	}

	formatted_string_buffer(const char8 *fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		_dynamic_buffer = 0;
		format(fmt, args);
	}

	formatted_string_buffer(const char8 *fmt, void *args)
	{
		_dynamic_buffer = 0;
		format(fmt, args);
	}
	
	~formatted_string_buffer()
	{
		memory_deallocate(_dynamic_buffer);
	}

	int32 format( const char8 *fmt, ... )
	{
		va_list args;
		va_start(args, fmt);
		_dynamic_buffer = 0;
		
		return format(fmt, args);
	}

	int32 format(const char8 *fmt, void *args)
	{
		if(_dynamic_buffer)
		{
			memory_deallocate(_dynamic_buffer);
			_dynamic_buffer = 0;
		}
		
		_len = vsnprintf(_fixed_buffer, sizeof(_fixed_buffer), fmt, (char *) args);
		if(_len < sizeof(_fixed_buffer))
			return _len;
		
		_dynamic_size = sizeof(_fixed_buffer);
		for(;;)
		{
			_dynamic_size *= 2;
			_dynamic_buffer = (char8 *) memory_reallocate(_dynamic_buffer, _dynamic_size, false);
			_len = vsnprintf(_dynamic_buffer, _dynamic_size, fmt, (char *) args);
			if(_len < _dynamic_size)
			{
				// trim off the remainder of the allocation
				memory_reallocate(_dynamic_buffer, _len + 1, true);
				return _len;
			}
		}
	}

	bool copy(char8 *buffer, uint32 buffer_size)
	{
		assert(buffer_size > 0);
		if(buffer_size >= size())
		{
			memcpy(buffer, c_str(), size());
			return true;
		}
		else
		{
			memcpy(buffer, c_str(), buffer_size - 1);
			buffer[buffer_size - 1] = 0;
			return false;
		}
	}

	uint32 length() { return _len; };

	uint32 size() { return _len + 1; };

	const char8* c_str()
	{
		return _dynamic_buffer ? _dynamic_buffer : _fixed_buffer;
	}

	static int32 format_buffer(char8 *buffer, uint32 buffer_size, const char8 *fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		return format_buffer(buffer, buffer_size, fmt, args);
	}
		
	static int32 format_buffer(char8 *buffer, uint32 buffer_size, const char8 *fmt, void *args)
	{
		assert(buffer_size > 0);
		int32 len = vsnprintf(buffer, buffer_size, fmt, (char *) args);
		buffer[buffer_size - 1] = 0;
		return len;
	}
private:
	char8  _fixed_buffer[2048]; // Fixed size buffer
	char8* _dynamic_buffer; // Format buffer for strings longer than _fixed_buffer
	uint32 _dynamic_size; // Size of dynamic buffer
	uint32 _len; // Length of the formatted string
};

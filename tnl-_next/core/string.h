// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.

class string
{
	struct string_record
	{
		uint32 ref_count;
		uint32 len;
		char data[1];
	};

	static string_record *_get_null_string_record()
	{
		static string_record null_string = { 0x7FFFFFFF, 0, 0 };
		return &null_string;
	}
	
	string_record *_string_record;
	void _dec_ref()
	{
		if(!--_string_record->ref_count)
			memory_deallocate(_string_record);
	}
public:
	string()
	{
		_string_record = _get_null_string_record();
		_string_record->ref_count++;
	}
	
	~string()
	{
		_dec_ref();
	}
	string(const char *string_data)
	{
		_string_record = _get_null_string_record();
		_string_record->ref_count++;
		set(string_data);
	}
	string(const string &the_string)
	{
		_string_record = the_string._string_record;
		_string_record->ref_count++;
	}
	
	string& operator=(const string &the_string)
	{
		_dec_ref();
		_string_record = the_string._string_record;
		_string_record->ref_count++;
		return *this;
	}
	void set(const char *string_data)
	{
		uint32 len = strlen(string_data);
		set(string_data, len);
	}
	
	void set(const char *string_data, uint32 len)
	{
		_dec_ref();
		_string_record = (string_record *) memory_allocate(sizeof(string_record) + len);
		_string_record->len = len;
		_string_record->ref_count = 1;
		memcpy(_string_record->data, string_data, len);
		_string_record->data[len] = 0;
	}
	operator const char *()
	{
		return _string_record->data;
	}
	const char *c_str()
	{
		return _string_record->data;
	}
};

string format_string(const char *fmt, ...)
{
	formatted_string_buffer buf;
	
	va_list args;
	va_start(args, fmt);
	uint32 len = buf.format(fmt, args);
	
	string return_value;
	return_value.set(buf.c_str(), len);
	
	return return_value;
}
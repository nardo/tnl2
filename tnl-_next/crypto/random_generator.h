/// The Random class encapsulates a cryptographically secure
/// pseudo random number generator (PRNG).  Internally the Random class
/// uses the Yarrow PRNG algorithm.
class random_generator
{
	prng_state _random_state;
	uint32 _entropy_added;
	
	random_generator()
	{
		yarrow_start(&_random_state);
		yarrow_ready(&_random_state);
		_entropy_added = 0;
	}
	prng_state *get_state()
	{
		return &_random_state;
	}
	
	/// Adds random "seed" data to the random number generator
	void add_entropy(const uint8 *random_data, uint32 data_len)
	{
		yarrow_add_entropy(random_data, data_len, &_random_state);
		_entropy_added += data_len;
		if(_entropy_added >= 16)
		{
			yarrow_ready(&_random_state);
			_entropy_added = 0;
		}
	}
	
	void random_buffer(uint8 *out_buffer, uint32 buffer_size)
	{
		yarrow_read(out_buffer, buffer_size, &_random_state);
	}
	
	uint32 random_integer()
	{
		uint8 buffer[4];
		random_buffer(buffer, sizeof(buffer));
		return (uint32(buffer[0]) << 24) | (uint32(buffer[1]) << 16) | (uint32(buffer[2]) << 8) | uint32(buffer[3]);
	}
	
	uint32 random_in_range(uint32 range_start, uint32 range_end)
	{
		assert(range_start <= range_end);
		return (random_integer() % (range_end - range_start + 1)) + range_start;
	}
	
	float32 random_unit_float()
	{
		return float32( float64(random_integer()) / float64(max_value_uint32) );
	}
	
	bool random_bool()
	{
		uint8 buffer;
		random_buffer(&buffer, sizeof(buffer));
		
		return buffer & 1;
	}
};


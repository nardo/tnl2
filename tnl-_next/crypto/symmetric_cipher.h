/// Class for symmetric encryption of data across a connection.  Internally it uses
/// the libtomcrypt AES algorithm to encrypt the data.

class symmetric_cipher
{
	public:
	enum {
		block_size = 16,
		key_size = 16,
	};
	private:
	struct key
	{
		uint32 e_k[64], d_k[64];
		int n_r;
	};
	uint32 _counter[block_size >> 2];
	uint32 _init_vector[block_size];
	uint8 _pad[block_size];
	key _symmetric_key;
	uint32 _pad_len;
	public:
	symmetric_cipher(const uint8 symmetric_key[key_size], const uint8 init_vector[block_size])
	{
		rijndael_setup(symmetric_key, key_size, 0, (symmetric_key *) &_symmetric_key);
		memcpy(_init_vector, init_vector, block_size);
		memcpy(_counter, init_vector, block_size);
		rijndael_ecb_encrypt((uint8 *) _counter, _pad, (symmetric_key *) &_symmetric_key);
		_pad_len = 0;
	}

	symmetric_cipher(const ByteBuffer *theByteBuffer)
	{
		if(theByteBuffer->getBufferSize() != key_size * 2)
		{
			uint8 buffer[key_size];
			memset(buffer, 0, key_size);
			rijndael_setup(buffer, key_size, 0, (symmetric_key *) &_symmetric_key);
			memcpy(_init_vector, buffer, block_size);
		}
		else
		{
			rijndael_setup(theByteBuffer->getBuffer(), key_size, 0, (symmetric_key *) &_symmetric_key);
			memcpy(_init_vector, theByteBuffer->getBuffer() + key_size, block_size);
		}
		memcpy(_counter, _init_vector, block_size);
		rijndael_ecb_encrypt((uint8 *) _counter, _pad, (symmetric_key *) &_symmetric_key);
		_pad_len = 0;
	}


	void setupCounter(uint32 counterValue1, uint32 counterValue2, uint32 counterValue3, uint32 counterValue4)
	{
		for(uint32 i = 0; i < 4; i++)
		{
			_counter[i] = _init_vector[i];
			littleEndianToHost(_counter[i]);
		}
		_counter[0] += counterValue1;
		_counter[1] += counterValue2;
		_counter[2] += counterValue3;
		_counter[3] += counterValue4;

		for(uint32 i = 0; i < 4; i++)
			hostToLittleEndian(_counter[i]);

		rijndael_ecb_encrypt((uint8 *) _counter, _pad, (symmetric_key *) &_symmetric_key);
		_pad_len = 0;
	}

	void encrypt(const uint8 *plainText, uint8 *cipherText, uint32 len)
	{
		while(len-- > 0)
		{
			if(_pad_len == block_size)
			{
				// we've reached the end of the pad, so compute a new pad
				rijndael_ecb_encrypt(_pad, _pad, (symmetric_key *) &_symmetric_key);
				_pad_len = 0;
			}
			uint8 encryptedChar = *plainText++ ^ _pad[_pad_len];
			_pad[_pad_len++] = *cipherText++ = encryptedChar;
		}
	}


	void decrypt(const uint8 *cipherText, uint8 *plainText, uint32 len)
	{
		while(len-- > 0)
		{
			if(_pad_len == block_size)
			{
				rijndael_ecb_encrypt(_pad, _pad, (symmetric_key *) &_symmetric_key);
				_pad_len = 0;
			}
			uint8 encryptedChar = *cipherText++;
			*plainText++ = encryptedChar ^ _pad[_pad_len];
			_pad[_pad_len++] = encryptedChar;
		}
	}
};

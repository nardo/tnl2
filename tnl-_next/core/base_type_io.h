// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.

template <class stream, typename type> inline bool write(stream &the_stream, const type &value)
{
	type temp = value;
	host_to_little_endian(temp);
	return the_stream.write_bytes((byte *) &temp, sizeof(temp)) == sizeof(temp);
}

template <class stream, typename type> inline bool read(stream &the_stream, type &value)
{
	if(the_stream.read_bytes((byte *) &value, sizeof(type)) != sizeof(type))
		return false;

	host_to_little_endian(value);
	return true;
}
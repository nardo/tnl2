// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.

typedef char char8; ///< 8 bit character
typedef unsigned short char16; ///< 16 bit character
typedef unsigned int char32; ///< 32 bit character
typedef signed char int8; ///< Signed 8 bit integer
typedef unsigned char uint8; ///< Unsigned 8 bit integer
typedef unsigned char byte; ///< Unsigned 8 bit integer
typedef signed short int16; ///< Signed 16 bit integer
typedef unsigned short uint16; ///< Unsigned 16 bit integer
typedef signed int int32; ///< Signed 32 bit integer
typedef unsigned int uint32; ///< Unsigned 8 bit integer
typedef float float32; ///< 32 bit floating-point value
typedef double float64; ///< 64 bit floating-point value
typedef double float128; ///< 128 bit floating point value
#if defined(COMPILER_VISUALC)
	typedef signed _int64 int64; ///< Signed 64 bit integer
	typedef unsigned _int64 uint64; ///< Unsigned 64 bit integer
	#define CONSTANT_64(a) (a##I64)
	#define CONSTANT_U64(a) (a##UI64)
#else
	typedef signed long long int64; ///< Signed 64 bit integer
	typedef unsigned long long uint64; ///< Unsigned 64 bit integer
	#define CONSTANT_64(a) (a##LL)
	#define CONSTANT_U64(a) (a##ULL) 
#endif

typedef const char *static_string; ///< Static string type is a string that refers to a static string in the executable.  The assumption that functions can make for arguments of this type are that they will not move and will survive for the duration of the program execution.

struct empty_type {}; ///< "Null" type used by templates


const int8 min_value_int8 = -128; ///< Smallest possible value for this type
const int8 max_value_int8 = +127; ///< Largest possible value for this type
const uint8 max_value_uint8 = +255U; ///< Largest possible value for this type

const int16 min_value_int16 = -32768; ///< Smallest possible value for this type
const int16 max_value_int16 = +32767; ///< Largest possible value for this type
const uint16 max_value_uint16 = +65535U; ///< Largest possible value for this type

const int32 min_value_int32 = -2147483647L; ///< Smallest possible value for this type
const int32 max_value_int32 = +2147483647L; ///< Largest possible value for this type
const uint32 max_value_uint32 = +4294967295U; ///< Largest possible value for this type

const int64 min_value_int64 = CONSTANT_64(-9223372036854775807); ///< Smallest possible value for this type
const int64 max_value_int64 = CONSTANT_64(+9223372036854775807); ///< Largest possible value for this type
const uint64 max_value_uint64 = CONSTANT_U64(+18446744073709551615); ///< Largest possible value for this type

const float32 min_value_float32 = float32(1.175494351e-38F); ///< Smallest possible positive value for this type
const float32 max_value_float32 = float32(3.402823466e+38F); ///< Largest possible value for this type

template<int32 range_min, int32 range_max> class int_ranged
{
public:
	enum {
		max_offset_value = range_max - range_min,
	};
	int_ranged()
	{
		_offset_value = 0;
	}
	int_ranged(int32 value)
	{
		_offset_value = value - range_min;
		assert(_offset_value <= max_offset_value);
	}
	operator int32()
	{
		return _offset_value + range_min;
	}
private:
	uint32 _offset_value;
};

template<uint32 enum_count> class enumeration
{
public:
	enumeration()
	{
		_enum_value = 0;
	}
	enumeration(uint32 enum_value)
	{
		assert(enum_value < enum_count);
		_enum_value = enum_value;
	}
	operator uint32()
	{
		return _enum_value;
	}
private:
	uint32 _enum_value;
};
#ifndef _CORE_HELPERS_H_
#define _CORE_HELPERS_H_

namespace TNL {

/**
 * Helper for the creation of a class which should not be 
 * copyable.  Classes should derive from NonCopyable who wish to 
 * have this behavior. 
 */
class NonCopyable
{
public:
   NonCopyable() {}
private:
   NonCopyable(NonCopyable const & other);
   NonCopyable & operator =(NonCopyable const & other);
};

/**
 * Check if the given value is a power of 2
 * 
 * @param value
 * 
 * @return 
 */
inline bool IsPow2(U32 value)
{
	return (((value & (value - 1)) == 0) && value);
}

inline F32 DegToRad(F32 d)
{
   return F32((d * FloatPi) / F32(180));
}

inline F32 RadToDeg(F32 r)
{
   return F32((r * 180.0) / FloatPi);
}

inline F64 DegToRad(F64 d)
{
   return (d * FloatPi) / F64(180);
}

inline F64 RadToDeg(F64 r)
{
   return (r * 180.0) / FloatPi;
}

inline F32 Clamp(F32 value, F32 min, F32 max)
{
   return value < min ? min : value > max ? max : value;
}

inline S32 Clamp(S32 value, S32 min, S32 max)
{
   return value < min ? min : value > max ? max : value;
}

inline F64 Clamp(F64 value, F64 min, F64 max)
{
   return value < min ? min : value > max ? max : value;
}

inline F32 Clamp(F32 value)
{
   return Clamp(value, 0.f, 1.f);
}

inline F64 Clamp(F64 value)
{
   return Clamp(value, F64(0), F64(1));
}

} // namespace TNL

#endif // _CORE_HELPERS_H_

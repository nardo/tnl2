// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.

template<typename type> inline type* construct(type* ptr)
{
	return new((void*)ptr) type;
}

template<typename type> inline type* construct(type* ptr,const type& a)
{
	return new((void*)ptr) type(a);
}

template<typename type> inline void destroy(type* ptr)
{
	ptr->~type();
}

namespace internal {
	template <class type> inline void construct_aux(type* ptr, type* end, true_type) {}
	template <class type> inline void construct_aux(type* ptr, type* end, false_type)
	{
		while (ptr != end)
			construct(ptr++);
	}
} // Private

template<typename type> inline void construct(type* ptr,type* end)
{
   typename bool_type<has_trivial_constructor<type>::is_true>::value_type is_trivial;
   internal::construct_aux(ptr,end,is_trivial);
}

template<typename type> inline void construct(type* ptr,type* end,const type& x)
{
   while (ptr != end)
      construct(ptr++,x);
}

//-----------------------------------------------------------------------------

namespace internal {
	template <class type> inline void destroy_aux(type* ptr, type* end, true_type) {}
	template <class type> inline void destroy_aux(type* ptr, type* end, false_type)
	{
		while (ptr != end)
			(ptr++)->~type();
	}
} // Private

template<typename type> inline void destroy(type* ptr,type* end)
{
   typename bool_type<has_trivial_destructor<type>::is_true>::value_type is_trivial;
   internal::destroy_aux(ptr,end,is_trivial);
}

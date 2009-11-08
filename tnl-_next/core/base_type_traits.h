// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.

struct true_type
{
	static const bool is_true = true;
};

struct false_type
{
	static const bool is_true = false;
};

template<bool> struct bool_type
{
	typedef false_type value_type;
};

template<> struct bool_type<true>
{
	typedef true_type value_type;
};

template<int v> struct int_to_type
{
	enum { value = v };
};

template<bool flag, typename flag_true_type, typename flag_false_type>
struct select_type
{
	typedef flag_true_type value_type;
};

template<typename flag_true_type, typename flag_false_type>
struct select_type<false, flag_true_type, flag_false_type>
{
	typedef flag_false_type value_type;
};

template<typename type1, typename type2> struct is_same
{
	static const bool is_true = false;
};

template<typename type> struct is_same<type, type>
{
	static const bool is_true = true;
};

#define TYPE_INFO_TEMPLATE(name, type_info_type, type_info_name, default_value) template<typename t> struct name { static const type_info_type type_info_name = default_value; }

#define INTEGER_TYPE_INFO_TEMPLATE(name, int_default_value) TYPE_INFO_TEMPLATE(name, int32, value, int_default_value)

#define TYPE_INFO_PARTIAL_SPEC(name, type_name, type_info_type, type_info_name, value) template<> struct name<type_name> { static const type_info_type type_info_name = value; }

#define INTEGER_TYPE_INFO_PARTIAL_SPEC(name, type_name, int_value) TYPE_INFO_PARTIAL_SPEC(name, type_name, int32, value, int_value)
	
#define BOOLEAN_TYPE_INFO_TEMPLATE(name, default_is_true) TYPE_INFO_TEMPLATE(name, bool, is_true, default_is_true)

#define BOOLEAN_TYPE_INFO_PARTIAL_SPEC(name, type_name, bool_is_true) TYPE_INFO_PARTIAL_SPEC(name, type_name, bool, is_true, bool_is_true)

#define TYPE_INFO_PARTIAL_SPEC1(name, type, spec, type_info_type, type_info_name, value) template<type> struct name<spec> { static const type_info_type type_info_name = value; }

#define BOOLEAN_TYPE_INFO_PARTIAL_SPEC1(name, type, spec, bool_is_true) TYPE_INFO_PARTIAL_SPEC1(name, type, spec, bool, is_true, bool_is_true)

#define TYPE_INFO_PARTIAL_SPEC2(name, type1, type2, spec, type_info_type, type_info_name, value) template<type1, type2> struct name<spec> { static const type_info_type type_info_name = value; }

#define BOOLEAN_TYPE_INFO_PARTIAL_SPEC2(name, type1, type2, spec, bool_is_true) TYPE_INFO_PARTIAL_SPEC2(name, type1, type2, spec, bool, is_true, bool_is_true)

BOOLEAN_TYPE_INFO_TEMPLATE(is_integral, false);
BOOLEAN_TYPE_INFO_PARTIAL_SPEC(is_integral, bool, true);
BOOLEAN_TYPE_INFO_PARTIAL_SPEC(is_integral, int8, true);
BOOLEAN_TYPE_INFO_PARTIAL_SPEC(is_integral, uint8, true);
BOOLEAN_TYPE_INFO_PARTIAL_SPEC(is_integral, int16, true);
BOOLEAN_TYPE_INFO_PARTIAL_SPEC(is_integral, uint16, true);
BOOLEAN_TYPE_INFO_PARTIAL_SPEC(is_integral, int32, true);
BOOLEAN_TYPE_INFO_PARTIAL_SPEC(is_integral, uint32, true);
BOOLEAN_TYPE_INFO_PARTIAL_SPEC(is_integral, int64, true);
BOOLEAN_TYPE_INFO_PARTIAL_SPEC(is_integral, uint64, true);
template<int32 range_min, int32 range_max> struct is_integral<int_ranged<range_min, range_max> > { static const bool is_true = true; };
template<uint32 enum_count> struct is_integral<enumeration<enum_count> > { static const bool is_true = true; };

BOOLEAN_TYPE_INFO_TEMPLATE(is_signed, false);
BOOLEAN_TYPE_INFO_PARTIAL_SPEC(is_signed, int8, true);
BOOLEAN_TYPE_INFO_PARTIAL_SPEC(is_signed, int16, true);
BOOLEAN_TYPE_INFO_PARTIAL_SPEC(is_signed, int32, true);
BOOLEAN_TYPE_INFO_PARTIAL_SPEC(is_signed, int64, true);
BOOLEAN_TYPE_INFO_PARTIAL_SPEC(is_signed, float32, true);
BOOLEAN_TYPE_INFO_PARTIAL_SPEC(is_signed, float64, true);
template<int32 range_min, int32 range_max> struct is_signed<int_ranged<range_min, range_max> > { static const bool is_true = range_min < 0; };

BOOLEAN_TYPE_INFO_TEMPLATE(is_float, false);
BOOLEAN_TYPE_INFO_PARTIAL_SPEC(is_float, float32, true);
BOOLEAN_TYPE_INFO_PARTIAL_SPEC(is_float, float64, true);

template<typename t> struct integer_range {
	static const int32 range_min = 0;
	static const uint32 range_size = 0;
};

#define INTEGER_RANGE(name, min, size) template<> struct integer_range<name> { static const int32 range_min = min; static const uint32 range_size = size; }

INTEGER_RANGE(bool, 0, 1);
INTEGER_RANGE(int8, min_value_int8, max_value_uint8);
INTEGER_RANGE(uint8, 0, max_value_uint8);
INTEGER_RANGE(int16, min_value_int16, max_value_uint16);
INTEGER_RANGE(uint16, 0, max_value_uint16);
INTEGER_RANGE(int32, min_value_int32, max_value_uint32);
INTEGER_RANGE(uint32, 0, max_value_uint32);
template<int32 min, int32 max> struct integer_range<int_ranged<min, max> > {
	static const int32 range_min = min;
	static const uint32 range_size = uint32(max - min);
};
template<uint32 enum_count> struct integer_range<enumeration<enum_count> > {
	static const int32 range_min = 0;
	static const uint32 range_size = enum_count - 1;
};

template<typename t> struct is_arithmetic
{
	static const bool is_true = is_integral<t>::is_true || is_float<t>::is_true;
};

BOOLEAN_TYPE_INFO_TEMPLATE(is_void, false);
BOOLEAN_TYPE_INFO_PARTIAL_SPEC(is_void, void, true);

BOOLEAN_TYPE_INFO_TEMPLATE(is_array, false);
BOOLEAN_TYPE_INFO_PARTIAL_SPEC2(is_array, typename t, size_t n, t[n], true);
BOOLEAN_TYPE_INFO_PARTIAL_SPEC2(is_array, typename t, size_t n, const t[n], true);
BOOLEAN_TYPE_INFO_PARTIAL_SPEC2(is_array, typename t, size_t n, volatile t[n], true);
BOOLEAN_TYPE_INFO_PARTIAL_SPEC2(is_array, typename t, size_t n, const volatile t[n], true);

BOOLEAN_TYPE_INFO_TEMPLATE(is_reference, false);
BOOLEAN_TYPE_INFO_PARTIAL_SPEC1(is_reference, typename t, t&, true);

BOOLEAN_TYPE_INFO_TEMPLATE(is_member_pointer, false);
BOOLEAN_TYPE_INFO_PARTIAL_SPEC2(is_member_pointer, typename t, typename u, u t::*, true);

template<typename type> struct is_function
{
	typedef uint16 yes;
	typedef uint32 no;
	
	template <typename u> static yes test(...);
	template <typename u> static no test(u (*)[1]);
	static const bool is_true = sizeof(test<type>(0)) == sizeof(yes);
	static const bool is_false = !is_true;
};

BOOLEAN_TYPE_INFO_PARTIAL_SPEC1(is_function, typename type, type&, false);
BOOLEAN_TYPE_INFO_PARTIAL_SPEC(is_function, void, false);
BOOLEAN_TYPE_INFO_PARTIAL_SPEC(is_function, void const, false);

BOOLEAN_TYPE_INFO_TEMPLATE(is_basic_pointer, false);
BOOLEAN_TYPE_INFO_PARTIAL_SPEC1(is_basic_pointer, typename t, t*, true);
BOOLEAN_TYPE_INFO_PARTIAL_SPEC1(is_basic_pointer, typename t, t* const, true);
BOOLEAN_TYPE_INFO_PARTIAL_SPEC1(is_basic_pointer, typename t, t* volatile, true);
BOOLEAN_TYPE_INFO_PARTIAL_SPEC1(is_basic_pointer, typename t, t* const volatile, true);

template<typename type> struct is_pointer
{
	static const bool is_true = is_basic_pointer<type>::is_true && !is_member_pointer<type>::is_true;
};

namespace internal
{
	template<typename from_type> struct is_convertible_aux
	{
		struct anything {
			template<typename to_type> anything(const volatile to_type &);
			template<typename to_type> anything(to_type &);
		};
		typedef uint16 yes;
		typedef uint32 no;
		static yes test(from_type, int);
		static no test(anything,...);
	};
}

template<typename from_type, typename to_type>
struct is_convertible
{
	static from_type dummy_make_type();
	static const bool is_true = sizeof(internal::is_convertible_aux<to_type>::test(dummy_make_type(),0))
			== sizeof(typename internal::is_convertible_aux<to_type>::yes);
};

namespace internal
{
	template<typename type, bool> struct is_enum_convertible
	{
		static const bool is_true = false;
	};
	template<typename type> struct is_enum_convertible<type, false>
	{
		static const bool is_true = is_convertible<type, int>::is_true;
	};
}

template<typename type> struct is_enum
{
	static const bool is_true = internal::is_enum_convertible<type,
		is_arithmetic<type>::is_true || is_pointer<type>::is_true || 
		is_function<type>::is_true || is_reference<type>::is_true>::is_true;
};

template<typename type> struct is_scalar
{
	static const bool is_true = is_arithmetic<type>::is_true ||
		is_enum<type>::is_true ||
		is_basic_pointer<type>::is_true;
};

template<typename T> struct class_traits
{
	static const bool is_pod = false;
	static const bool has_trivial_constructor = false;
	static const bool has_trivial_destructor = false;
	static const bool has_trivial_assign = false;
	static const bool has_trivial_copy = false;
};

template<typename type> struct is_pod
{
	static const bool is_true = is_scalar<type>::is_true || 
		is_void<type>::is_true || 
		class_traits<type>::is_pod;
};

template<typename type, size_t n> struct is_pod<type[n]> : is_pod<type> {};

template<typename type> struct has_trivial_constructor
{
	static const bool is_true = is_pod<type>::is_true || class_traits<type>::has_trivial_constructor;
};

template<typename type> struct has_trivial_destructor
{
	static const bool is_true = is_pod<type>::is_true || class_traits<type>::has_trivial_destructor;
};

template<typename type> struct has_trivial_assign
{
	static const bool is_true = is_pod<type>::is_true || class_traits<type>::has_trivial_assign;
};

template<typename type> struct has_trivial_copy
{
	static const bool is_true = is_pod<type>::is_true || class_traits<type>::has_trivial_copy;
};

template<typename type> struct non_qualified_type
{
	typedef type value_type;
};

template<typename type>
struct enumeration_traits
{
	enum { bit_count = 0, };
};

template<typename type>
struct type_traits {
	typedef typename type::value_type value_type;
	typedef typename type::pointer pointer;
	typedef typename type::reference reference;
};

template<typename type>
struct type_traits<type*> {
	typedef type value_type;
	typedef type* pointer;
	typedef type& reference;
};

template<typename type>
struct type_traits<const type*> {
	typedef type value_type;
	typedef type* pointer;
	typedef type& reference;
};

template<typename type> struct non_qualified_type<const type> { typedef type value_type; };
template<typename type> struct non_qualified_type<volatile type> { typedef type value_type; };
template<typename type> struct non_qualified_type<const volatile type> { typedef type value_type; };
template<typename type, size_t count> struct non_qualified_type<type const[count]> { typedef type value_type; };
template<typename type, size_t count> struct non_qualified_type<type volatile[count]> { typedef type value_type; };
template<typename type, size_t count> struct non_qualified_type<type const volatile[count]> { typedef type value_type; };



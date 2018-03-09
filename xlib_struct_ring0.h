/*!
  \file  xlib_struct_ring0.h
  \brief xlib_struct_ring0.h定义Ring3下已提供，但Ring0下没有提供的结构，供Ring0使用

  - 结构来自于Ring3的windows定义，每个结构前列举了加入时间

  \author   triones
  \date     2011-4-8
*/
#ifndef _XLIB_STRUCT_RING0_H_
#define _XLIB_STRUCT_RING0_H_

#if defined(_WIN32) && defined(FOR_RING0)

//20120906 1115
typedef struct _IMAGE_DOS_HEADER {      // DOS .EXE header
  WORD   e_magic;                     // Magic number
  WORD   e_cblp;                      // Bytes on last page of file
  WORD   e_cp;                        // Pages in file
  WORD   e_crlc;                      // Relocations
  WORD   e_cparhdr;                   // Size of header in paragraphs
  WORD   e_minalloc;                  // Minimum extra paragraphs needed
  WORD   e_maxalloc;                  // Maximum extra paragraphs needed
  WORD   e_ss;                        // Initial (relative) SS value
  WORD   e_sp;                        // Initial SP value
  WORD   e_csum;                      // Checksum
  WORD   e_ip;                        // Initial IP value
  WORD   e_cs;                        // Initial (relative) CS value
  WORD   e_lfarlc;                    // File address of relocation table
  WORD   e_ovno;                      // Overlay number
  WORD   e_res[4];                    // Reserved words
  WORD   e_oemid;                     // OEM identifier (for e_oeminfo)
  WORD   e_oeminfo;                   // OEM information; e_oemid specific
  WORD   e_res2[10];                  // Reserved words
  LONG   e_lfanew;                    // File address of new exe header
  } IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;

//20120906 1115
typedef struct _IMAGE_DATA_DIRECTORY {
  DWORD   VirtualAddress;
  DWORD   Size;
  } IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES    16

//20120906 1115。x64 20131211 1640
typedef struct _IMAGE_OPTIONAL_HEADER {
  //
  // Standard fields.
  //

  WORD    Magic;
  BYTE    MajorLinkerVersion;
  BYTE    MinorLinkerVersion;
  DWORD   SizeOfCode;
  DWORD   SizeOfInitializedData;
  DWORD   SizeOfUninitializedData;
  DWORD   AddressOfEntryPoint;
  DWORD   BaseOfCode;
  DWORD   BaseOfData;

  //
  // NT additional fields.
  //

  PVOID   ImageBase;
  DWORD   SectionAlignment;
  DWORD   FileAlignment;
  WORD    MajorOperatingSystemVersion;
  WORD    MinorOperatingSystemVersion;
  WORD    MajorImageVersion;
  WORD    MinorImageVersion;
  WORD    MajorSubsystemVersion;
  WORD    MinorSubsystemVersion;
  DWORD   Win32VersionValue;
  DWORD   SizeOfImage;
  DWORD   SizeOfHeaders;
  DWORD   CheckSum;
  WORD    Subsystem;
  WORD    DllCharacteristics;
  TUINT   SizeOfStackReserve;
  TUINT   SizeOfStackCommit;
  TUINT   SizeOfHeapReserve;
  TUINT   SizeOfHeapCommit;
  DWORD   LoaderFlags;
  DWORD   NumberOfRvaAndSizes;
  IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
  } IMAGE_OPTIONAL_HEADER, *PIMAGE_OPTIONAL_HEADER;

//20120906 1115
typedef struct _IMAGE_FILE_HEADER {
  WORD    Machine;
  WORD    NumberOfSections;
  DWORD   TimeDateStamp;
  DWORD   PointerToSymbolTable;
  DWORD   NumberOfSymbols;
  WORD    SizeOfOptionalHeader;
  WORD    Characteristics;
  } IMAGE_FILE_HEADER;

//20120906 1115。x64 20131211 1640
typedef struct _IMAGE_NT_HEADERS {
  DWORD Signature;
  IMAGE_FILE_HEADER FileHeader;
  IMAGE_OPTIONAL_HEADER OptionalHeader;
  } IMAGE_NT_HEADERS;


//20170904 1629 源于c++标准库
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
namespace std {
//////////////////////////////////////////////////////////////////////////
#define _NOEXCEPT noexcept
#define _INLINE_VAR
#define _NODISCARD
	// STRUCT TEMPLATE integral_constant
template<class _Ty,
	_Ty _Val>
	struct integral_constant
	{	// convenient template for integral constant types
	static constexpr _Ty value = _Val;

	using value_type = _Ty;
	using type = integral_constant;

	constexpr operator value_type() const _NOEXCEPT
		{	// return stored value
		return (value);
		}

	constexpr value_type operator()() const _NOEXCEPT
		{	// return stored value
		return (value);
		}
	};

	// ALIAS TEMPLATE bool_constant
template<bool _Val>
	using bool_constant = integral_constant<bool, _Val>;

using true_type = bool_constant<true>;
using false_type = bool_constant<false>;

	// STRUCT TEMPLATE enable_if
template<bool _Test,
	class _Ty = void>
	struct enable_if
	{	// type is undefined for assumed !_Test
	};

template<class _Ty>
	struct enable_if<true, _Ty>
	{	// type is _Ty for _Test
	using type = _Ty;
	};

template<bool _Test,
	class _Ty = void>
	using enable_if_t = typename enable_if<_Test, _Ty>::type;

	// STRUCT TEMPLATE conditional
template<bool _Test,
	class _Ty1,
	class _Ty2>
	struct conditional
	{	// type is _Ty2 for assumed !_Test
	using type = _Ty2;
	};

template<class _Ty1,
	class _Ty2>
	struct conditional<true, _Ty1, _Ty2>
	{	// type is _Ty1 for _Test
	using type = _Ty1;
	};

template<bool _Test,
	class _Ty1,
	class _Ty2>
	using conditional_t = typename conditional<_Test, _Ty1, _Ty2>::type;

	// STRUCT TEMPLATE is_same
template<class _Ty1,
	class _Ty2>
	struct is_same
		: false_type
	{	// determine whether _Ty1 and _Ty2 are the same type
	};

template<class _Ty1>
	struct is_same<_Ty1, _Ty1>
		: true_type
	{	// determine whether _Ty1 and _Ty2 are the same type
	};

template<class _Ty,
	class _Uty>
	_INLINE_VAR constexpr bool is_same_v = is_same<_Ty, _Uty>::value;

	// STRUCT TEMPLATE remove_cv
template<class _Ty>
	struct remove_cv
	{	// remove top level const and volatile qualifiers
	using type = _Ty;
	};

template<class _Ty>
	struct remove_cv<const _Ty>
	{	// remove top level const and volatile qualifiers
	using type = _Ty;
	};

template<class _Ty>
	struct remove_cv<volatile _Ty>
	{	// remove top level const and volatile qualifiers
	using type = _Ty;
	};

template<class _Ty>
	struct remove_cv<const volatile _Ty>
	{	// remove top level const and volatile qualifiers
	using type = _Ty;
	};

template<class _Ty>
	using remove_cv_t = typename remove_cv<_Ty>::type;
	
	// STRUCT TEMPLATE _Is_integral
template<class _Ty>
	struct _Is_integral
		: false_type
	{	// determine whether _Ty is integral
	};

template<>
	struct _Is_integral<bool>
		: true_type
	{	// determine whether _Ty is integral
	};

template<>
	struct _Is_integral<char>
		: true_type
	{	// determine whether _Ty is integral
	};

template<>
	struct _Is_integral<unsigned char>
		: true_type
	{	// determine whether _Ty is integral
	};

template<>
	struct _Is_integral<signed char>
		: true_type
	{	// determine whether _Ty is integral
	};

 #ifdef _NATIVE_WCHAR_T_DEFINED
template<>
	struct _Is_integral<wchar_t>
		: true_type
	{	// determine whether _Ty is integral
	};
 #endif /* _NATIVE_WCHAR_T_DEFINED */

template<>
	struct _Is_integral<char16_t>
		: true_type
	{	// determine whether _Ty is integral
	};

template<>
	struct _Is_integral<char32_t>
		: true_type
	{	// determine whether _Ty is integral
	};

template<>
	struct _Is_integral<unsigned short>
		: true_type
	{	// determine whether _Ty is integral
	};

template<>
	struct _Is_integral<short>
		: true_type
	{	// determine whether _Ty is integral
	};

template<>
	struct _Is_integral<unsigned int>
		: true_type
	{	// determine whether _Ty is integral
	};

template<>
	struct _Is_integral<int>
		: true_type
	{	// determine whether _Ty is integral
	};

template<>
	struct _Is_integral<unsigned long>
		: true_type
	{	// determine whether _Ty is integral
	};

template<>
	struct _Is_integral<long>
		: true_type
	{	// determine whether _Ty is integral
	};

template<>
	struct _Is_integral<unsigned long long>
		: true_type
	{	// determine whether _Ty is integral
	};

template<>
	struct _Is_integral<long long>
		: true_type
	{	// determine whether _Ty is integral
	};

	// STRUCT TEMPLATE is_integral
template<class _Ty>
	struct is_integral
		: _Is_integral<remove_cv_t<_Ty>>::type
	{	// determine whether _Ty is integral
	};

template<class _Ty>
	_INLINE_VAR constexpr bool is_integral_v = is_integral<_Ty>::value;
	
	// STRUCT TEMPLATE _Is_floating_point
template<class _Ty>
	struct _Is_floating_point
		: false_type
	{	// determine whether _Ty is floating point
	};

template<>
	struct _Is_floating_point<float>
		: true_type
	{	// determine whether _Ty is floating point
	};

template<>
	struct _Is_floating_point<double>
		: true_type
	{	// determine whether _Ty is floating point
	};

template<>
	struct _Is_floating_point<long double>
		: true_type
	{	// determine whether _Ty is floating point
	};
	// STRUCT TEMPLATE is_floating_point
template<class _Ty>
	struct is_floating_point
		: _Is_floating_point<remove_cv_t<_Ty>>::type
	{	// determine whether _Ty is floating point
	};

template<class _Ty>
	_INLINE_VAR constexpr bool is_floating_point_v = is_floating_point<_Ty>::value;

	// STRUCT TEMPLATE is_signed
#pragma warning(push)
#pragma warning(disable: 4296)	// expression is always false
template<class _Ty,
	bool = is_integral_v<_Ty>>
	struct _Sign_base
	{	// determine whether integral _Ty is a signed or unsigned type
	using _Uty = remove_cv_t<_Ty>;
	using _Signed = bool_constant<_Uty(-1) < _Uty(0)>;
	using _Unsigned = bool_constant<_Uty(0) < _Uty(-1)>;
	};
#pragma warning(pop)

template<class _Ty>
	struct _Sign_base<_Ty, false>
	{	// floating-point _Ty is signed
		// non-arithmetic _Ty is neither signed nor unsigned
	using _Signed = typename is_floating_point<_Ty>::type;
	using _Unsigned = false_type;
	};

template<class _Ty>
	struct is_signed
		: _Sign_base<_Ty>::_Signed
	{	// determine whether _Ty is a signed type
	};

template<class _Ty>
	_INLINE_VAR constexpr bool is_signed_v = is_signed<_Ty>::value;

	// STRUCT TEMPLATE is_unsigned
template<class _Ty>
	struct is_unsigned
		: _Sign_base<_Ty>::_Unsigned
	{	// determine whether _Ty is an unsigned type
	};

template<class _Ty>
	_INLINE_VAR constexpr bool is_unsigned_v = is_unsigned<_Ty>::value;

	// ALIAS TEMPLATE _Is_nonbool_integral
template<class _Ty>
	using _Is_nonbool_integral =
		bool_constant<is_integral_v<_Ty>
			&& !is_same_v<remove_cv_t<_Ty>, bool>>;
	
	// STRUCT TEMPLATE integer_sequence
template<class _Ty,
	_Ty... _Vals>
	struct integer_sequence
	{	// sequence of integer parameters
	static_assert(is_integral_v<_Ty>,
		"integer_sequence<T, I...> requires T to be an integral type.");

	using value_type = _Ty;

	_NODISCARD static constexpr size_t size() _NOEXCEPT
		{	// get length of parameter list
		return (sizeof...(_Vals));
		}
	};
	// STRUCT TEMPLATE is_enum
template<class _Ty>
	struct is_enum
		: bool_constant<__is_enum(_Ty)>
	{	// determine whether _Ty is an enumerated type
	};

template<class _Ty>
	_INLINE_VAR constexpr bool is_enum_v = is_enum<_Ty>::value;
	
	// VARIABLE TEMPLATES _None_of_v AND _Is_any_of_v
template<bool... _Bools>
	_INLINE_VAR constexpr bool _None_of_v = is_same_v<
		integer_sequence<bool, false, _Bools...>,
		integer_sequence<bool, _Bools..., false>>;	// TRANSITION, fold expressions

template<class _Ty,
	class... _Types>
	_INLINE_VAR constexpr bool _Is_any_of_v = !_None_of_v<is_same<_Ty, _Types>::value...>;	// TRANSITION, VSO#444168

	// STRUCT TEMPLATE _Change_sign
template<class _Ty>
	struct _Change_sign
	{	// signed/unsigned partners to _Ty
	static_assert(_Is_nonbool_integral<_Ty>::value || is_enum_v<_Ty>,
		"make_signed<T>/make_unsigned<T> require that T shall be a (possibly "
		"cv-qualified) integral type or enumeration but not a bool type.");

	using _Signed =
		conditional_t<_Is_any_of_v<_Ty, long, unsigned long>, long,
		conditional_t<sizeof(_Ty) == 1, signed char,
		conditional_t<sizeof(_Ty) == 2, short,
		conditional_t<sizeof(_Ty) == 4, int,
			long long
		>>>>;

	using _Unsigned =
		conditional_t<_Is_any_of_v<_Ty, long, unsigned long>, unsigned long,
		conditional_t<sizeof(_Ty) == 1, unsigned char,
		conditional_t<sizeof(_Ty) == 2, unsigned short,
		conditional_t<sizeof(_Ty) == 4, unsigned int,
			unsigned long long
		>>>>;
	};

template<class _Ty>
	struct _Change_sign<const _Ty>
	{	// signed/unsigned partners to _Ty
	using _Signed = const typename _Change_sign<_Ty>::_Signed;
	using _Unsigned = const typename _Change_sign<_Ty>::_Unsigned;
	};

template<class _Ty>
	struct _Change_sign<volatile _Ty>
	{	// signed/unsigned partners to _Ty
	using _Signed = volatile typename _Change_sign<_Ty>::_Signed;
	using _Unsigned = volatile typename _Change_sign<_Ty>::_Unsigned;
	};

template<class _Ty>
	struct _Change_sign<const volatile _Ty>
	{	// signed/unsigned partners to _Ty
	using _Signed = const volatile typename _Change_sign<_Ty>::_Signed;
	using _Unsigned = const volatile typename _Change_sign<_Ty>::_Unsigned;
	};

	// STRUCT TEMPLATE make_signed
template<class _Ty>
	struct make_signed
	{	// signed partner to _Ty
	using type = typename _Change_sign<_Ty>::_Signed;
	};

template<class _Ty>
	using make_signed_t = typename make_signed<_Ty>::type;

	// STRUCT TEMPLATE make_unsigned
template<class _Ty>
	struct make_unsigned
	{	// unsigned partner to _Ty
	using type = typename _Change_sign<_Ty>::_Unsigned;
	};

template<class _Ty>
	using make_unsigned_t = typename make_unsigned<_Ty>::type;

	// FUNCTION TEMPLATE _Unsigned_value
template<class _Rep>
	constexpr make_unsigned_t<_Rep> _Unsigned_value(_Rep _Val)
	{	// makes _Val unsigned
	return (static_cast<make_unsigned_t<_Rep>>(_Val));
	}
	
	// STRUCT TEMPLATE is_class
template<class _Ty>
	struct is_class
		: bool_constant<__is_class(_Ty)>
	{	// determine whether _Ty is a class
	};

template<class _Ty>
	_INLINE_VAR constexpr bool is_class_v = is_class<_Ty>::value;
//////////////////////////////////////////////////////////////////////////
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#endif  // _WIN32 FOR_RING0

#endif  // _XLIB_STRUCT_RING0_H_
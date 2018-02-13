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
#ifndef _LONGLONG     // Ring0下此定义不存在，故添加之
#define _LONGLONG	int64
#define _ULONGLONG uint64
#endif

#include <xtr1common>
namespace std
  {
  //////////////////////////////////////////////////////////////////////////
  // TEMPLATE CLASS is_signed
  template<class _Ty>
  struct _Has_signed_vals
    : _Cat_base < (typename remove_cv<_Ty>::type)(-1)
    < (typename remove_cv<_Ty>::type)(0) >
    {	// integral type can represent negative values
    };

  // TYPE DEFINITIONS

  template<bool,
  class _Ty1,
  class _Ty2>
  struct _If
    {	// type is _Ty2 for assumed false
    typedef _Ty2 type;
    };

  template<class _Ty1,
  class _Ty2>
  struct _If<true, _Ty1, _Ty2>
    {	// type is _Ty1 for assumed true
    typedef _Ty1 type;
    };

  template<class _Ty>
  struct is_signed
    : _Cat_base<is_floating_point<_Ty>::value || (is_integral<_Ty>::value
    && _Has_signed_vals<
    typename _If<is_integral<_Ty>::value, _Ty, int>::type>::value)>
    {	// determine whether _Ty is a signed type
    };

  // TEMPLATE CLASS is_unsigned
  template<class _Ty>
  struct is_unsigned
    : _Cat_base<is_integral<_Ty>::value
    && !_Has_signed_vals<
    typename _If<is_integral<_Ty>::value, _Ty, int>::type>::value>
    {	// determine whether _Ty is an unsigned type
    };

#define _IS_ENUM(_Ty)	\
	: _Cat_base<__is_enum(_Ty)>

  // TEMPLATE CLASS is_enum
  template<class _Ty>
  struct is_enum
    _IS_ENUM(_Ty)
    {	// determine whether _Ty is an enumerated type
    };

  // TEMPLATE CLASS _Change_sign
  template<class _Ty>
  struct _Change_sign
    {	// signed/unsigned partners to _Ty
    static_assert(
      ((is_integral<_Ty>::value || is_enum<_Ty>::value)
      && !is_same<_Ty, bool>::value),
      "make_signed<T>/make_unsigned<T> require that T shall be a (possibly "
      "cv-qualified) integral type or enumeration but not a bool type.");

    typedef
      typename _If<is_same<_Ty, signed char>::value
      || is_same<_Ty, unsigned char     >::value, signed char,
      typename _If<is_same<_Ty, short       >::value
      || is_same<_Ty, unsigned short    >::value, short,
      typename _If<is_same<_Ty, int         >::value
      || is_same<_Ty, unsigned int      >::value, int,
      typename _If<is_same<_Ty, long        >::value
      || is_same<_Ty, unsigned long     >::value, long,
      typename _If<is_same<_Ty, long long   >::value
      || is_same<_Ty, unsigned long long>::value, long long,
      typename _If<sizeof(_Ty) == sizeof(signed char), signed char,
      typename _If<sizeof(_Ty) == sizeof(short), short,
      typename _If<sizeof(_Ty) == sizeof(int), int,
      typename _If<sizeof(_Ty) == sizeof(long), long,
      long long
      >::type>::type>::type>::type>::type>::type>::type>::type>::type
      _Signed;

    typedef
      typename _If<is_same<_Signed, signed char>::value, unsigned char,
      typename _If<is_same<_Signed, short      >::value, unsigned short,
      typename _If<is_same<_Signed, int        >::value, unsigned int,
      typename _If<is_same<_Signed, long       >::value, unsigned long,
      unsigned long long
      >::type>::type>::type>::type
      _Unsigned;
    };

  template<class _Ty>
  struct _Change_sign<const _Ty>
    {	// signed/unsigned partners to _Ty
    typedef const typename _Change_sign<_Ty>::_Signed _Signed;
    typedef const typename _Change_sign<_Ty>::_Unsigned _Unsigned;
    };

  template<class _Ty>
  struct _Change_sign<volatile _Ty>
    {	// signed/unsigned partners to _Ty
    typedef volatile typename _Change_sign<_Ty>::_Signed _Signed;
    typedef volatile typename _Change_sign<_Ty>::_Unsigned _Unsigned;
    };

  template<class _Ty>
  struct _Change_sign<const volatile _Ty>
    {	// signed/unsigned partners to _Ty
    typedef const volatile typename _Change_sign<_Ty>::_Signed _Signed;
    typedef const volatile typename _Change_sign<_Ty>::_Unsigned _Unsigned;
    };

  // TEMPLATE CLASS make_signed
  template<class _Ty>
  struct make_signed
    {	// signed partner to _Ty
    typedef typename _Change_sign<_Ty>::_Signed type;
    };

  // TEMPLATE CLASS make_unsigned
  template<class _Ty>
  struct make_unsigned
    {	// unsigned partner to _Ty
    typedef typename _Change_sign<_Ty>::_Unsigned type;
    };

#define _IS_CLASS(_Ty)	\
	: _Cat_base<__is_class(_Ty)>

  // TEMPLATE CLASS is_class
  template<class _Ty>
  struct is_class _IS_CLASS(_Ty)
    {	// determine whether _Ty is a class
    };
  //////////////////////////////////////////////////////////////////////////
  }
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#endif  // _WIN32 FOR_RING0

#endif  // _XLIB_STRUCT_RING0_H_
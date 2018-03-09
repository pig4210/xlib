/*!
  \file  xlib_def.h
  \brief xlib_def.h定义了一些库常用宏。

  \version    2.0.1611.1411
  \note       For All

  \author     triones
  \date       2014-01-07
*/
#ifndef _XLIB_DEF_H_
#define _XLIB_DEF_H_

// 考虑到可能使用其它编译器（如intel）的情况，不采用_MSC_VER宏来区别windows与linux。为记

#ifdef _WIN32                               // 适配windows
typedef unsigned __int8     uint8;
typedef unsigned __int16    uint16;
typedef unsigned __int32    uint32;
typedef unsigned __int64    uint64;

typedef __int8              int8;
typedef __int16             int16;
typedef __int32             int32;
typedef __int64             int64;
#else                                       // 适配linux
typedef unsigned char       uint8;
typedef unsigned short      uint16;
typedef unsigned int        uint32;
typedef unsigned long long  uint64;

typedef char                int8;
typedef short               int16;
typedef int                 int32;
typedef long long           int64;
#endif

#if defined(_WIN64) || defined(__amd64)     // 适配x64
typedef uint64              TUINT;
typedef int64               TINT;
typedef unsigned long long  TULONG;
typedef long long           TLONG;
#else                                       // 适配x86
typedef uint32              TUINT;
typedef int32               TINT;
typedef unsigned long       TULONG;
typedef long                TLONG;
#endif

/*!
  \brief            从指定地址开始读取一个指定类型的值。
  \param in   mem   该名字指定了一个内存地址。
  \return           从mem指定的内存地址处取出的一个指定类型的值。

  \code
    T tmpL = mkT(0x400000);   //取值（右值）
    mkT(0x10000) = 1;         //赋值（左值）
  \endcode
*/
#define mkQ(mem)    (*(uint64*)(mem))
#define mkD(mem)    (*(uint32*)(mem))
#define mkW(mem)    (*(uint16*)(mem))
#define mkB(mem)    (*(uint8*)(mem))
#define mkP(mem)    (*(void**)(mem))
#define mkTL(mem)   (*(TULONG*)(mem))

//! 计算指定类型的数据长度
#define sizeQ       (sizeof(uint64))
#define sizeD       (sizeof(uint32))
#define sizeW       (sizeof(uint16))
#define sizeB       (sizeof(uint8))
#define sizeP       (sizeof(void*))
#define sizeTL      (sizeof(TULONG))

//! 注意外层的windows判定，不建议去除以使用更优的windows定义
#ifndef _WIN32
#   ifndef _countof
#       define _countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#   endif
#endif

#endif  // _XLIB_DEF_H_
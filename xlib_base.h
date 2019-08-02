/**
  \file   xlib_base.h
  \brief  用于基础包含。

  \version    1.1.0.161114
  \note       For All

  \author     triones
  \date       2010-07-01

  \section more 额外说明
  
  - 异常处理宏定义，及 Ring0 下基础包含， Ring3 下基础包含。
  - Ring0 下全局的 new & delete 操作符重载。
  - 由于 operator new 与 operator delete 的特殊性，必须把这组操作独立出来，并置放于多数头文件声明之前，以确保内核内存分配的正确性。
  - 有了这个头文件，内核编程中也可以使用类等动态对象（默认使用非分页内存）。
  
  \section history 版本记录

  - 2010-07-05 虽然好像没有太大必要，还是新加 operator new[] 。 0.1 。
  - 2010-11-23 转移入 LIB ，强制默认使用非分页内存。取消 ExAllocatePool 的宏定义。 0.2 。
  - 2010-11-26 发现遗漏取消 ExFreePool 的宏定义。 0.2.1 。
  - 2011-07-26 发现在 Ring0 下释放空内存导致 C2 蓝屏，故在 delete 内部做处理，以保持外部代码一致性。 0.3 。
  - 2011-11-11 转移入 COMMON ，加上 ring0 头文件包含、 try 宏。 0.4 。
  - 2012-05-18 加入 common_link 包含， windows 包含， struct 包含。 0.4.1 。
  - 2014-01-06 移植入 xlib 。 1.0 。
  - 2016-11-14 适配 Linux g++。 1.1 。
*/
#ifndef _XLIB_BASE_H_
#define _XLIB_BASE_H_

#include <stdlib.h>
#include "xlib_def.h"
#include "xlib_test.h"

#ifdef _WIN32

  #ifndef FOR_RING0                     // 适配 Windows Ring3 。

    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include "xlib_struct_ring3.h"
    #include "xlib_struct.h"

  #else                                 // 适配 Windows Ring0 。

    #ifdef __cplusplus
      extern "C"
        {
    #endif

    #pragma warning(push)
    #pragma warning(disable:4201) // 使用了非标准扩展 : 无名称的结构/联合。
    #include <ntifs.h>
    #include <windef.h>
    #pragma warning(pop)

    #ifdef __cplusplus
        } 
    #endif

    /// 以下是 new & delete 的 Ring0 级重载。
    void* __cdecl operator new(size_t size);
    void* __cdecl operator new(size_t size, POOL_TYPE pooltype);
    void* __cdecl operator new[](size_t size);
    void* __cdecl operator new[](size_t size, POOL_TYPE pooltype);
    void  __cdecl operator delete(void* mem);
    void  __cdecl operator delete[](void* mem);

    #include "xlib_struct_ring0.h"
    #include "xlib_struct.h"

    /// Ring0 下不能使用 try ，虽能编译，但无法加载。只能使用 Windows 提供的异常机制。
    #define XLIB_TRY     __try
    #define XLIB_CATCH   __except(EXCEPTION_EXECUTE_HANDLER)

    // warning 在 xlib 编译时并未显现，却在被链接时触发，故此处放置。
    #pragma warning(disable:4127) // 条件表达式是常量。
    #pragma warning(disable:4100) // 未引用的形参。

  #endif  // FOR_RING0

#endif  // _WIN32

#ifndef XLIB_TRY                        // Linux 与 Windows Ring3 统一使用 try 。
  #define XLIB_TRY     try
  #define XLIB_CATCH   catch(...)
#endif

#endif  // _XLIB_BASE_H_
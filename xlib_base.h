/*!
  \file   xlib_base.h
  \brief  xlib_base.h用于基础包含

  - 异常处理宏定义，及Ring0下基础包含，Ring3下基础包含
  - Ring0下全局的new delete操作符重载。
  - 由于operator new与operator delete的特殊性，必须把这组操作独立出来，
    并置放于多数头文件声明之前，以确保内核内存分配的正确性。
  - 有了这个头文件，内核编程中也可以使用类等动态对象(默认使用非分页内存)

  \version    3.0.1611.1415
  \note       For All

  \author     triones
  \date       2010-07-01
*/
#ifndef _XLIB_BASE_H_
#define _XLIB_BASE_H_

#include <stdlib.h>

#include "xlib_link.h"
#include "xlib_def.h"

#include "xlib_test.h"

#ifdef _WIN32

#ifdef FOR_RING0                        // 适配windows ring0

// RING0下<包含目录> : $(VC_IncludePath);$(WindowsSDK_IncludePath)

#ifdef __cplusplus
extern "C"
  {
#endif

#pragma warning(push)
#pragma warning(disable:4201) //使用了非标准扩展 : 无名称的结构/联合
#include <ntifs.h>
#include <windef.h>
#pragma warning(pop)

#ifdef __cplusplus
  } 
#endif

//! 以下是new与delete的Ring0级重载
void* __cdecl operator new(size_t size);
void* __cdecl operator new(size_t size, POOL_TYPE pooltype);
void* __cdecl operator new[](size_t size);
void* __cdecl operator new[](size_t size, POOL_TYPE pooltype);
void  __cdecl operator delete(void* mem);
void  __cdecl operator delete[](void* mem);

#include "xlib_struct_ring0.h"
#include "xlib_struct.h"

//! Ring0下不能使用try，虽能编译，但无法加载。只能使用windows提供的异常机制
#define XLIB_TRY     __try
#define XLIB_CATCH   __except(EXCEPTION_EXECUTE_HANDLER)

// warning在xlib编译时并未显现，却在被链接时触发，故此处放置
#pragma warning(disable:4127) // 条件表达式是常量
#pragma warning(disable:4100) // 未引用的形参

#else                                   // 适配windows ring3

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "xlib_struct_ring3.h"
#include "xlib_struct.h"

#endif  // FOR_RING0

#endif  // _WIN32

#ifndef XLIB_TRY                        // 除了windows ring0，linux与windows ring3统一使用try
#   define XLIB_TRY     try
#   define XLIB_CATCH   catch(...)
#endif

#endif  // _XLIB_BASE_H_
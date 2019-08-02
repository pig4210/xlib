/**
  \file   xlib_nt.h
  \brief  提供未文档化的内核导出函数。 Ring0 下为 Import ， Ring3 下为 Extern 。

  \version    1.0.0.120518
  \note       For All

  \author     triones
  \date       2012-05-18
  
  \section history 版本记录

  - 2012-05-18 建立 common_nt 。 0.1 。
  - 2012-05-23 函数定义由宏实现，使用了宏不定参，没有完美解决法，但还好了已经。
  - 2014-01-07 移植入 xlib ，改名 xlib_nt 。 1.0 。
*/
#ifndef _XLIB_NT_H_
#define _XLIB_NT_H_

#ifdef _WIN32

#include "xlib_base.h"

#ifdef FOR_RING0
  #define XLIB_NTAPI    NTSYSAPI
  #define XLIB_EXTERNC  extern "C"
#else
  #define XLIB_NTAPI    WINAPI
  #define XLIB_EXTERNC
#endif

/// 以下是共通未导出函数。

// 20120518 1158 。
// http://msdn.microsoft.com/zh-cn/ms725506
XLIB_EXTERNC NTSTATUS XLIB_NTAPI ZwQuerySystemInformation(
  __in       SYSTEM_INFORMATION_CLASS SystemInformationClass,
  __inout    PVOID                    SystemInformation,
  __in       ULONG_PTR                SystemInformationLength,
  __out_opt  PULONG_PTR               ReturnLength
  );

// 20120519 1750 。
XLIB_EXTERNC NTSTATUS XLIB_NTAPI ZwProtectVirtualMemory(
  __in_opt   HANDLE       ProcessHandle,
  __inout    PVOID*       BaseAddress,
  __inout    PULONG_PTR   NumberOfBytesToProtect,
  __in       ULONG_PTR    NewAccessProtection,
  __out      PULONG_PTR   OldAccessProtection
  );

#ifdef FOR_RING0                      /// 以下函数只有 Ring0 提供。

  // 20120906 1127 。
  XLIB_EXTERNC LPCSTR XLIB_NTAPI PsGetProcessImageFileName(PEPROCESS Process);

#else   // FOR_RING0

  /// 以下函数 Ring0 下导出， Ring3 下未导出。函数定义来自于 Ring0 。

  // 20120522 1718 。
  VOID XLIB_NTAPI RtlInitUnicodeString(
    __out     PUNICODE_STRING DestinationString,
    __in_opt  PCWSTR          SourceString
    );

  // 20120523 1718 。
  VOID XLIB_NTAPI RtlInitAnsiString(
    __out     PANSI_STRING  DestinationString,
    __in_opt  PCSZ          SourceString
    );

  // 20120523 1718 。
  NTSTATUS XLIB_NTAPI RtlUnicodeStringToAnsiString(
    __inout PANSI_STRING      DestinationString,
    __in    PCUNICODE_STRING  SourceString,
    __in    BOOLEAN           AllocateDestinationString
    );

  // 20120523 1718 。
  NTSTATUS XLIB_NTAPI RtlAnsiStringToUnicodeString(
    __inout PUNICODE_STRING DestinationString,
    __in    PCANSI_STRING   SourceString,
    __in    BOOLEAN         AllocateDestinationString
    );

  // 20120523 1718 。
  VOID XLIB_NTAPI RtlFreeUnicodeString(
    __inout PUNICODE_STRING UnicodeString
    );

  // 20120523 1718 。
  VOID XLIB_NTAPI RtlFreeAnsiString(
    __inout PANSI_STRING AnsiString
    );

  // 20130702 1117 。
  NTSTATUS XLIB_NTAPI ZwQueryInformationProcess(
    __in HANDLE             ProcessHandle,
    __in PROCESSINFOCLASS   ProcessInformationClass,
    __out_bcount_opt(ProcessInformationLength) PVOID ProcessInformation,
    __in ULONG_PTR          ProcessInformationLength,
    __out_opt PULONG_PTR    ReturnLength
    );

  // 20130702 1117 。
  NTSTATUS XLIB_NTAPI ZwQueryObject(
    _In_opt_ HANDLE               Handle,
    _In_ OBJECT_INFORMATION_CLASS ObjectInformationClass,
    _Out_writes_bytes_opt_(ObjectInformationLength) PVOID ObjectInformation,
    _In_ ULONG                    ObjectInformationLength,
    _Out_opt_ PULONG              ReturnLength
    );

  // 20190625 2044
  NTSTATUS XLIB_NTAPI LdrLoadDll(
    IN  PWCHAR            PathToFile,
    IN  ULONG             Flags,
    IN  PUNICODE_STRING   ModuleFileName,
    OUT PHANDLE           ModuleHandle
    );

  // 20190625 2044
  NTSTATUS XLIB_NTAPI LdrUnloadDll(IN HANDLE ModuleHandle);
#endif  // FOR_RING0

#endif  // _WIN32

#endif  // _XLIB_NT_H_
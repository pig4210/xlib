#include "xlib_nt.h"

#if defined(_WIN32) && !defined(FOR_RING0)

#define DEFINE_FUNCTION_START(rettype, calltype, funcname, ...) \
  rettype calltype funcname(__VA_ARGS__)\
    {\
    typedef rettype (calltype * nt_##funcname)(__VA_ARGS__);\
    static const nt_##funcname gk_##funcname =\
      (nt_##funcname)Get_NTDLL_Proc(#funcname);\
    return gk_##funcname(
#define DEFINE_FUNCTION_END(...) \
                         __VA_ARGS__);\
    }

static void* Get_NTDLL_Proc(__in LPCSTR lpProcName)
  {
  static const HMODULE gk_hmod_ntdll = GetModuleHandle(TEXT("ntdll"));
  return GetProcAddress(gk_hmod_ntdll, lpProcName);
  }


DEFINE_FUNCTION_START(NTSTATUS, XLIB_NTAPI, ZwQuerySystemInformation,
  __in       SYSTEM_INFORMATION_CLASS SystemInformationClass,
  __inout    PVOID                    SystemInformation,
  __in       ULONG_PTR                SystemInformationLength,
  __out_opt  PULONG_PTR               ReturnLength
  )
DEFINE_FUNCTION_END(SystemInformationClass, SystemInformation,
  SystemInformationLength, ReturnLength)


DEFINE_FUNCTION_START(NTSTATUS, XLIB_NTAPI, ZwProtectVirtualMemory,
  __in_opt    HANDLE      ProcessHandle,
  __inout     PVOID*      BaseAddress,
  __inout     PULONG_PTR  NumberOfBytesToProtect,
  __in        ULONG_PTR   NewAccessProtection,
  __out       PULONG_PTR  OldAccessProtection
  )
DEFINE_FUNCTION_END(ProcessHandle, BaseAddress,
  NumberOfBytesToProtect, NewAccessProtection, OldAccessProtection)


DEFINE_FUNCTION_START(VOID, XLIB_NTAPI, RtlInitUnicodeString,
  __out     PUNICODE_STRING DestinationString,
  __in_opt  PCWSTR          SourceString
  )
DEFINE_FUNCTION_END(DestinationString, SourceString)


DEFINE_FUNCTION_START(VOID, XLIB_NTAPI, RtlInitAnsiString,
  __out     PANSI_STRING  DestinationString,
  __in_opt  PCSZ          SourceString
  )
DEFINE_FUNCTION_END(DestinationString, SourceString)


DEFINE_FUNCTION_START(NTSTATUS, XLIB_NTAPI, RtlUnicodeStringToAnsiString,
  __inout PANSI_STRING      DestinationString,
  __in    PCUNICODE_STRING  SourceString,
  __in    BOOLEAN           AllocateDestinationString
  )
DEFINE_FUNCTION_END(DestinationString, SourceString,
  AllocateDestinationString)

  
DEFINE_FUNCTION_START(NTSTATUS, XLIB_NTAPI, RtlAnsiStringToUnicodeString,
  __inout PUNICODE_STRING DestinationString,
  __in    PCANSI_STRING   SourceString,
  __in    BOOLEAN         AllocateDestinationString
  )
DEFINE_FUNCTION_END(DestinationString, SourceString,
  AllocateDestinationString)

  
DEFINE_FUNCTION_START(VOID, XLIB_NTAPI, RtlFreeUnicodeString,
  __inout PUNICODE_STRING UnicodeString
  )
DEFINE_FUNCTION_END(UnicodeString)


// 注意 Ring3 下不提供 RtlFreeAnsiString 。
VOID XLIB_NTAPI RtlFreeAnsiString(
  __inout PANSI_STRING AnsiString
  )
  {
  RtlFreeUnicodeString((PUNICODE_STRING)AnsiString);
  }


DEFINE_FUNCTION_START(NTSTATUS, XLIB_NTAPI, ZwQueryInformationProcess,
  __in HANDLE             ProcessHandle,
  __in PROCESSINFOCLASS   ProcessInformationClass,
  __out_bcount_opt(ProcessInformationLength) PVOID ProcessInformation,
  __in ULONG_PTR          ProcessInformationLength,
  __out_opt PULONG_PTR    ReturnLength
  )
DEFINE_FUNCTION_END(ProcessHandle, ProcessInformationClass,
                    ProcessInformation, ProcessInformationLength,
                    ReturnLength)


DEFINE_FUNCTION_START(NTSTATUS, XLIB_NTAPI, ZwQueryObject,
  __in_opt   HANDLE                   Handle,
  __in       OBJECT_INFORMATION_CLASS ObjectInformationClass,
  _Out_writes_bytes_opt_(ObjectInformationLength) PVOID ObjectInformation,
  __in       ULONG_PTR                ObjectInformationLength,
  __out_opt  PULONG_PTR               ReturnLength
  )
DEFINE_FUNCTION_END(Handle, ObjectInformationClass, ObjectInformation,
                    ObjectInformationLength, ReturnLength)


DEFINE_FUNCTION_START(NTSTATUS, XLIB_NTAPI, ZwDuplicateObject,
  __in   HANDLE   hSourceProcessHandle,
  __in   HANDLE   hSourceHandle,
  __in   HANDLE   hTargetProcessHandle,
  __out  LPHANDLE lpTargetHandle,
  __in   DWORD    dwDesiredAccess,
  __in   BOOL     bInheritHandle,
  __in   DWORD    dwOptions
  )
DEFINE_FUNCTION_END(hSourceProcessHandle, hSourceHandle,
  hTargetProcessHandle, lpTargetHandle, dwDesiredAccess,
  bInheritHandle, dwOptions)


DEFINE_FUNCTION_START(NTSTATUS, XLIB_NTAPI, LdrLoadDll,
  IN  PWCHAR            PathToFile,
  IN  ULONG             Flags,
  IN  PUNICODE_STRING   ModuleFileName,
  OUT PHANDLE           ModuleHandle
  )
DEFINE_FUNCTION_END(PathToFile, Flags, ModuleFileName, ModuleHandle)


DEFINE_FUNCTION_START(NTSTATUS, XLIB_NTAPI, LdrUnloadDll, IN HANDLE ModuleHandle)
DEFINE_FUNCTION_END(ModuleHandle);

#undef DEFINE_FUNCTION_START
#undef DEFINE_FUNCTION_END

#endif  // _WIN32 && !FOR_RING0
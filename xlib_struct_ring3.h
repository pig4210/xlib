/*!
  \file  xlib_struct_ring3.h
  \brief xlib_struct_ring3.h定义Ring0下已提供，但Ring3下没有提供的结构，供Ring3使用

  - 结构来自于Ring0的windows定义，每个结构前列举了加入时间

  \author   triones
  \date     2011-4-8
*/
#ifndef _XLIB_STRUCT_RING3_H_
#define _XLIB_STRUCT_RING3_H_

#if defined(_WIN32) && !defined(FOR_RING0)

//20120518 1201
typedef LONG NTSTATUS;

//20120323  1144
#define STATUS_SUCCESS  ((NTSTATUS)0x00000000L)

//20130702  1053
#define STATUS_INFO_LENGTH_MISMATCH      ((NTSTATUS)0xC0000004L)

//20120517 1019
#define ERROR_SUCCESS 0L

//20120522  1144
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

//20120517 1019
#define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004L)

//20120522  1144
typedef struct _UNICODE_STRING {
  USHORT Length;
  USHORT MaximumLength;
#ifdef MIDL_PASS
  [size_is(MaximumLength / 2), length_is((Length) / 2) ] USHORT * Buffer;
#else // MIDL_PASS
  __field_bcount_part(MaximumLength, Length) PWCH   Buffer;
#endif // MIDL_PASS
  } UNICODE_STRING;
typedef UNICODE_STRING *PUNICODE_STRING;
typedef const UNICODE_STRING *PCUNICODE_STRING;

//20120522  1144
typedef struct _STRING {
  __maybevalid USHORT Length;
  __maybevalid USHORT MaximumLength;
#ifdef MIDL_PASS
  [size_is(MaximumLength), length_is(Length) ]
#endif // MIDL_PASS
  __field_bcount_part_opt(MaximumLength, Length) PCHAR Buffer;
  } STRING;
typedef STRING *PSTRING;
typedef STRING ANSI_STRING;
typedef PSTRING PANSI_STRING;
typedef PSTRING PCANSI_STRING;
typedef CHAR *PSZ;
typedef CONST char *PCSZ;

//20120517 1019
#pragma warning(push)

#ifdef __INTEL_COMPILER
#   pragma warning(disable:94)//the size of an array must be greater than zero
#else
#   pragma warning(disable:4200)//使用了非标准扩展 : 结构/联合中的零大小数组
#endif

typedef struct _OBJECT_NAME_INFORMATION {

  UNICODE_STRING          Name;
  WCHAR                   NameBuffer[0];

  } OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;

#pragma warning(pop)

//20120517 1019
typedef struct _OBJECT_BASIC_INFORMATION {

  ULONG                   Attributes;
  ACCESS_MASK             DesiredAccess;
  ULONG                   HandleCount;
  ULONG                   ReferenceCount;
  ULONG                   PagedPoolUsage;
  ULONG                   NonPagedPoolUsage;
  ULONG                   Reserved[3];
  ULONG                   NameInformationLength;
  ULONG                   TypeInformationLength;
  ULONG                   SecurityDescriptorLength;
  LARGE_INTEGER           CreationTime;

  } OBJECT_BASIC_INFORMATION, *POBJECT_BASIC_INFORMATION;

//20130702 1119
typedef struct _PEB *PPEB;

//20130702 1120
typedef TLONG KPRIORITY;

//20130702 1117
typedef struct _PROCESS_BASIC_INFORMATION {
  NTSTATUS ExitStatus;
  PPEB PebBaseAddress;
  ULONG_PTR AffinityMask;
  KPRIORITY BasePriority;
  ULONG_PTR UniqueProcessId;
  ULONG_PTR InheritedFromUniqueProcessId;
  } PROCESS_BASIC_INFORMATION,*PPROCESS_BASIC_INFORMATION;

//20130702 1121
typedef enum _PROCESSINFOCLASS {
  ProcessBasicInformation,
  ProcessQuotaLimits,
  ProcessIoCounters,
  ProcessVmCounters,
  ProcessTimes,
  ProcessBasePriority,
  ProcessRaisePriority,
  ProcessDebugPort,
  ProcessExceptionPort,
  ProcessAccessToken,
  ProcessLdtInformation,
  ProcessLdtSize,
  ProcessDefaultHardErrorMode,
  ProcessIoPortHandlers,          // Note: this is kernel mode only
  ProcessPooledUsageAndLimits,
  ProcessWorkingSetWatch,
  ProcessUserModeIOPL,
  ProcessEnableAlignmentFaultFixup,
  ProcessPriorityClass,
  ProcessWx86Information,
  ProcessHandleCount,
  ProcessAffinityMask,
  ProcessPriorityBoost,
  ProcessDeviceMap,
  ProcessSessionInformation,
  ProcessForegroundInformation,
  ProcessWow64Information,
  ProcessImageFileName,
  ProcessLUIDDeviceMapsEnabled,
  ProcessBreakOnTermination,
  ProcessDebugObjectHandle,
  ProcessDebugFlags,
  ProcessHandleTracing,
  ProcessIoPriority,
  ProcessExecuteFlags,
  ProcessTlsInformation,
  ProcessCookie,
  ProcessImageInformation,
  ProcessCycleTime,
  ProcessPagePriority,
  ProcessInstrumentationCallback,
  ProcessThreadStackAllocation,
  ProcessWorkingSetWatchEx,
  ProcessImageFileNameWin32,
  ProcessImageFileMapping,
  ProcessAffinityUpdateMode,
  ProcessMemoryAllocationMode,
  ProcessGroupInformation,
  ProcessTokenVirtualizationEnabled,
  ProcessConsoleHostProcess,
  ProcessWindowInformation,
  MaxProcessInfoClass             // MaxProcessInfoClass should always be the last enum
  } PROCESSINFOCLASS;

//系统对象信息枚举  20120517 1019
//http://msdn.microsoft.com/zh-cn/library/windows/hardware/ff550964(v=vs.85).aspx
//http://undocumented.ntinternals.net/UserMode/Undocumented%20Functions/NT%20Objects/Type%20independed/OBJECT_INFORMATION_CLASS.html
typedef enum _OBJECT_INFORMATION_CLASS
  {
  ObjectBasicInformation,
  ObjectNameInformation,
  ObjectTypeInformation,
  ObjectAllTypesInformation,
  ObjectHandleInformation
  } OBJECT_INFORMATION_CLASS;

//CLIENT_ID 20140218 1112
typedef struct _CLIENT_ID
  {
  HANDLE UniqueProcess;
  HANDLE UniqueThread;
  } CLIENT_ID;
typedef CLIENT_ID *PCLIENT_ID;

//KWAIT_REASON 20140218 1115
typedef enum _KWAIT_REASON
  {
  Executive,
  FreePage,
  PageIn,
  PoolAllocation,
  DelayExecution,
  Suspended,
  UserRequest,
  WrExecutive,
  WrFreePage,
  WrPageIn,
  WrPoolAllocation,
  WrDelayExecution,
  WrSuspended,
  WrUserRequest,
  WrSpare0,
  WrQueue,
  WrLpcReceive,
  WrLpcReply,
  WrVirtualMemory,
  WrPageOut,
  WrRendezvous,
  WrKeyedEvent,
  WrTerminated,
  WrProcessInSwap,
  WrCpuRateControl,
  WrCalloutStack,
  WrKernel,
  WrResource,
  WrPushLock,
  WrMutex,
  WrQuantumEnd,
  WrDispatchInt,
  WrPreempted,
  WrYieldExecution,
  WrFastMutex,
  WrGuardedMutex,
  WrRundown,
  WrAlertByThreadId,
  WrDeferredPreempt,
  MaximumWaitReason
  } KWAIT_REASON;

//VM_COUNTERS 20140218 1117
typedef struct _VM_COUNTERS
  {
  SIZE_T PeakVirtualSize;
  SIZE_T VirtualSize;
  TULONG PageFaultCount;
  SIZE_T PeakWorkingSetSize;
  SIZE_T WorkingSetSize;
  SIZE_T QuotaPeakPagedPoolUsage;
  SIZE_T QuotaPagedPoolUsage;
  SIZE_T QuotaPeakNonPagedPoolUsage;
  SIZE_T QuotaNonPagedPoolUsage;
  SIZE_T PagefileUsage;
  SIZE_T PeakPagefileUsage;
  } VM_COUNTERS;
typedef VM_COUNTERS *PVM_COUNTERS;

#endif  // _WIN32 !FOR_RING0

#endif  // _XLIB_STRUCT_RING3_H_
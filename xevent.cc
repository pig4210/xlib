#include "xevent.h"

#ifdef _WIN32

#ifdef FOR_RING0

xevent::xevent(BOOL bManualReset,BOOL bInitialState)
  {
  KeInitializeEvent(&kevent,
    bManualReset ? NotificationEvent : SynchronizationEvent,
    bInitialState == TRUE);
  }

xevent::~xevent()
  {
  KeClearEvent(&kevent);
  }

bool xevent::wait()
  {
  return STATUS_SUCCESS ==
    KeWaitForSingleObject(&kevent, Executive, KernelMode, FALSE, nullptr);
  }

void xevent::set()
  {
  KeSetEvent(&kevent, IO_NO_INCREMENT, FALSE);
  }

void xevent::reset()
  {
  KeResetEvent(&kevent);
  }

#else   // FOR_RING0

xevent::xevent(BOOL     bManualReset,
               BOOL     bInitialState,
               LPCTSTR  lpName)
:handle(CreateEvent(nullptr, bManualReset, bInitialState, lpName))
  {
  ;
  }

xevent::~xevent()
  {
  CloseHandle(handle);
  }

bool xevent::wait(const DWORD time)
  {
  return WAIT_OBJECT_0 == WaitForSingleObject(handle, time);
  }

bool xevent::set()
  {
  return SetEvent(handle) == TRUE;
  }

bool xevent::reset()
  {
  return ResetEvent(handle) == TRUE;
  }

#endif  // FOR_RING0

#endif  // _WIN32
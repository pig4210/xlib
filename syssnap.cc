#include "syssnap.h"

#ifdef _WIN32

#include "xlib_nt.h"

SysSnap::SysSnap(SYSTEM_INFORMATION_CLASS SystemInformationClass)
  {
  NTSTATUS dwRet;
  ULONG_PTR len = 0;
  while((dwRet = ZwQuerySystemInformation(SystemInformationClass,
    (PVOID)c_str(), (ULONG_PTR)capacity(), &len)) == STATUS_INFO_LENGTH_MISMATCH)
    {
    reserve(capacity() + len + 0x10);
    }
  if(dwRet == STATUS_SUCCESS)
    {
    append(c_str(), len);
    }
  else
    {
    clear();
    }
  }

SysProcessSnap::const_iterator::const_iterator(const SYSTEM_PROCESS_INFORMATION* p)
: _p(p)
  {
  ;
  }

bool SysProcessSnap::const_iterator::operator==(const SysProcessSnap::const_iterator& it) const
  {
  return (_p == it._p);
  }

bool SysProcessSnap::const_iterator::operator!=(const SysProcessSnap::const_iterator& it) const
  {
  return (_p != it._p);
  }

SysProcessSnap::const_iterator& SysProcessSnap::const_iterator::operator++()
  {
  if(_p != nullptr)
    {
    if((_p->NextEntryOffset) == 0)
      {
      _p = nullptr;
      }
    else
      {
      _p = (const SYSTEM_PROCESS_INFORMATION*)((const uint8*)_p + _p->NextEntryOffset);
      }
    }
  return *this;
  }

const SYSTEM_PROCESS_INFORMATION* SysProcessSnap::const_iterator::operator->() const
  {
  return _p;
  }

const SYSTEM_PROCESS_INFORMATION& SysProcessSnap::const_iterator::operator*() const
  {
  return *_p;
  }

SysProcessSnap::SysProcessSnap() :SysSnap(SystemProcessInformation)
  {
  ;
  }

SysProcessSnap::const_iterator SysProcessSnap::begin() const
  {
  if(empty())  return nullptr;
  return (const SYSTEM_PROCESS_INFORMATION*)(c_str());
  }

SysProcessSnap::const_iterator SysProcessSnap::end() const
  {
  return nullptr;
  }

SysThreadSnap::SysThreadSnap(HANDLE PID)
:_p(nullptr)
  {
  for(const SYSTEM_PROCESS_INFORMATION& p : (*(SysProcessSnap*)this))
    {
    if(p.ProcessId == PID)
      {
      _p = &p;
      }
    }
  }

const SYSTEM_THREAD* SysThreadSnap::begin() const
  {
  if(_p == nullptr) return nullptr;
  return _p->Threads;
  }

const SYSTEM_THREAD* SysThreadSnap::end() const
  {
  if(_p == nullptr) return nullptr;
  return &(_p->Threads[_p->NumberOfThreads]);
  }

SysDriverSnap::SysDriverSnap()
:SysSnap(SystemModuleInformation)
  {
  ;
  }

const SYSTEM_MODULE* SysDriverSnap::begin() const
  {
  if(empty())  return nullptr;
  const PSYSTEM_MODULE_INFORMATION p = (PSYSTEM_MODULE_INFORMATION)c_str();
  if(p->NumberOfModules <= 0) return nullptr;
  return p->Information;
  }

const SYSTEM_MODULE* SysDriverSnap::end() const
  {
  if(empty())  return nullptr;
  const PSYSTEM_MODULE_INFORMATION p = (PSYSTEM_MODULE_INFORMATION)c_str();
  if(p->NumberOfModules <= 0) return nullptr;
  return &(p->Information[p->NumberOfModules]);
  }

#ifdef _XLIB_TEST_

#include "ws_s.h"

#pragma warning(push)

#pragma warning(disable:4127)  // C4127: 条件表达式是常量 。
#pragma warning(disable:4312)  // C4312: “类型强制转换”: 从“const DWORD”转换到更大的“void *” 。
#define sysnap_test_out if(false) cout    // 默认不输出。

ADD_XLIB_TEST(SYSSNAP)
  {
  SHOW_TEST_INIT;

  auto done = false;

  int count = 0;

  SHOW_TEST_HEAD("SysProcessSnap");
  sysnap_test_out << endl;
  count = 0;
  SysProcessSnap sps;
  for(const auto& ppi : sps)
    {
    TCHAR* lp = (ppi.ImageName.Buffer == nullptr) ? L"[System Process]" : ppi.ImageName.Buffer;
    sysnap_test_out << "\t" << (void*)ppi.ProcessId << "\t" << ws2s(lp) << endl;
    ++count;
    }
  sysnap_test_out << endl;
  done = (0 != count);
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("SysThreadSnap");
  sysnap_test_out << endl;
  count = 0;
  SysThreadSnap sts(0);
  for(const auto& st : sts)
    {
    sysnap_test_out << "\t" << st.StartAddress << endl;
    ++count;
    }
  sysnap_test_out << endl;
  done = (0 != count);
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("SysDriverSnap");
  sysnap_test_out << endl;
  count = 0;
  SysDriverSnap sds;
  for(const auto& st : sds)
    {
    sysnap_test_out << "\t" << st.ImageBaseAddress << "\t" << (char*)(st.Name + st.NameOffset) << endl;
    ++count;
    }
  sysnap_test_out << endl;
  done = (0 != count);
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("ProcessSnap");
  sysnap_test_out << endl;
  count = 0;
  ProcessSnap ps;
  for(const auto& it : ps)
    {
    sysnap_test_out << "\t" << (void*)it.th32ProcessID << "\t" << ws2s(it.szExeFile) << endl;
    ++count;
    }
  sysnap_test_out << endl;
  done = (0 != count);
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("ThreadSnap");
  sysnap_test_out << endl;
  count = 0;
  ThreadSnap ts(GetCurrentProcessId());
  for(const auto& it : ts)
    {
    sysnap_test_out << "\t" << (void*)it.th32ThreadID << endl;
    ++count;
    }
  sysnap_test_out << endl;
  done = (0 != count);
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("ModuleSnap");
  sysnap_test_out << endl;
  count = 0;
  ModuleSnap ms(GetCurrentProcessId());
  for(const auto& it : ms)
    {
    sysnap_test_out << "\t" << (void*)it.modBaseAddr << "\t" << ws2s(it.szModule) << endl;
    ++count;
    }
  sysnap_test_out << endl;
  done = (0 != count);
  SHOW_TEST_RESULT(done);
  }

#pragma warning(pop)

#endif  // _XLIB_TEST_

#endif  // _WIN32
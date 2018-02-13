/*!
  \file  syssnap.h
  \brief syssnap.h用于枚举系统信息

  - SysXXXSnap使用ZwQuerySystemInformation获取系统信息
  - XXXSnap使用CreateToolhelp32Snapshot获取系统信息

  \version    2.0.1402.1917
  \note       For All

  \author     triones
  \date       2013-08-06
*/
#ifndef _XLIB_SYSSNAP_H_
#define _XLIB_SYSSNAP_H_

#include <string>

#include "xlib_base.h"

#ifdef _WIN32

//! 用于基本Snapshot
class SysSnap : public std::string
  {
  public:
    SysSnap(SYSTEM_INFORMATION_CLASS SystemInformationClass);
  };
 
/*!
  用于进程枚举

  \code
    SysProcessSnap sps;
    for(const auto& ppi : sps)
      {
      TCHAR* lp = (ppi.ImageName.Buffer == nullptr) ? L"[System Process]" : ppi.ImageName.Buffer;
      cout << "\t" << (void*)ppi.ProcessId << "\t" << ws2s(lp) << endl;
      }
  \endcode
*/
class SysProcessSnap : public SysSnap
  {
  public:
    class const_iterator
      {
      public:
        const_iterator(const SYSTEM_PROCESS_INFORMATION* p);
        bool operator==(const const_iterator& it) const;
        bool operator!=(const const_iterator& it) const;
        const_iterator& operator++();
        const SYSTEM_PROCESS_INFORMATION* operator->() const;
        const SYSTEM_PROCESS_INFORMATION& operator*() const;
      private:
        const SYSTEM_PROCESS_INFORMATION* _p;
      };
  public:
    SysProcessSnap();
    const_iterator begin() const;
    const_iterator end() const;
  };

/*!
  用于线程枚举

  \code
    SysThreadSnap sts(GetCurrentProcess());
    for(const auto& st : sts)
      {
      cout << "\t" << st.StartAddress << endl;
      }
  \endcode
*/
class SysThreadSnap : public SysProcessSnap
  {
  public:
    SysThreadSnap(HANDLE PID);
    const SYSTEM_THREAD* begin() const;
    const SYSTEM_THREAD* end() const;
  private:
    const SYSTEM_PROCESS_INFORMATION* _p;
  };

/*!
  用于驱动枚举

  \code
    SysDriverSnap sds;
    for(const auto& st : sds)
      {
      cout << "\t" << st.ImageBaseAddress << "\t" << (char*)(st.Name + st.NameOffset) << endl;
      }
  \endcode
*/
class SysDriverSnap : public SysSnap
  {
  public:
    SysDriverSnap();
    const SYSTEM_MODULE* begin() const;
    const SYSTEM_MODULE* end() const;
  };

#ifndef FOR_RING0

#include <Tlhelp32.h>
//! 快照枚举模版，仿标准容器操作
/*!

  \code
    ProcessSnap ps;
    for(const auto& it : ps)
      {
      wcout << it->szExeFile << endl;
      }
  \endcode
*/
template<
        typename  ST,
        void*     FirstFunc,
        void*     NextFunc,
        DWORD     Flags
        >
class Snapshot
  {
  public:
    typedef typename BOOL(WINAPI *func_snap)(HANDLE hSnapshot, ST* lpst);
  public:
    //! 内嵌类，用以实现仿迭代操作
    class const_iterator
      {
      public:
        const_iterator(Snapshot* s):_s(s)
          {
          ;
          }
        bool operator==(const const_iterator& it) const
          {
          return (_s == it._s);
          }
        bool operator!=(const const_iterator& it) const
          {
          return (_s != it._s);
          }
        const_iterator& operator++()
          {
          if(_s != nullptr)
            {
            const func_snap Next = (func_snap)NextFunc;
            if(!Next(_s->hSnapshot, &(_s->st)))
              _s = nullptr;
            }
          return *this;
          }
        const ST* operator->() const
          {
          if(_s != nullptr)  return &(_s->st);
          return nullptr;
          }
        const ST& operator*() const
          {
          return *operator->();
          }
      private:
        Snapshot* _s;
      };
  public:
    Snapshot(const DWORD pid = 0)
      :hSnapshot(INVALID_HANDLE_VALUE)
      {
      st.dwSize = sizeof(st);
      do
        {
        hSnapshot = CreateToolhelp32Snapshot(Flags, pid);
        if(hSnapshot != INVALID_HANDLE_VALUE) break;
        if(GetLastError() != ERROR_PARTIAL_COPY)  break;
        } while(hSnapshot == INVALID_HANDLE_VALUE);
      }
    ~Snapshot()
      {
      CloseHandle(hSnapshot);
      }
    const_iterator begin()
      {
      const func_snap First = (func_snap)FirstFunc;
      if(First(hSnapshot, &st))  return this;
      return nullptr;
      }
    const_iterator end() const
      {
      return nullptr;
      }
  private:
    HANDLE hSnapshot;
    ST st;
  };

//! 进程快照
typedef Snapshot<
  PROCESSENTRY32,
  Process32First,
  Process32Next,
  TH32CS_SNAPPROCESS>     ProcessSnap;

//! 线程快照
typedef Snapshot<
  THREADENTRY32,
  Thread32First,
  Thread32Next,
  TH32CS_SNAPTHREAD>      ThreadSnap;

//! 模块快照
typedef Snapshot<
  MODULEENTRY32,
  Module32First,
  Module32Next,
  TH32CS_SNAPMODULE>      ModuleSnap;

#endif  // FOR_RING0

#endif  // _WIN32

#endif  // _XLIB_SYSSNAP_H_
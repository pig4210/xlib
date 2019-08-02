/**
  \file  xevent.h
  \brief 定义了通用的事件对象。

  \version    1.0.0.130301
  \note       For All

  \author     triones
  \date       2013-03-01
  
  \section history 版本记录

  - 2013-03-01 新建 xevent 。 1.0 。
*/
#ifndef _XLIB_XEVENT_H_
#define _XLIB_XEVENT_H_

#ifdef _WIN32

#include "xlib_base.h"

#ifdef FOR_RING0

/// Ring0 下的 xevent ，暂未查到如何释放 xevent 资源。
class xevent
  {
  public:
    /// 默认自动复位，初始未激发。
    xevent(BOOL bManualReset = FALSE,
           BOOL bInitialState = FALSE);
    ~xevent();
  public:
    bool wait();          /// 等待事件。
    void set();           /// 激发事件。
    void reset();         /// 设置事件未激发。
  public:
    xevent(const xevent&) = delete;
    xevent& operator=(const xevent&) = delete;
  private:
    KEVENT kevent;
  };

#else   // FOR_RING0

/// Ring3 下的 xevent ,与 Ring0 有一定区别。
class xevent
  {
  public:
    /// 默认自动复位，初始未激发，无名
    xevent(BOOL    bManualReset = FALSE,
           BOOL    bInitialState = FALSE,
           LPCTSTR lpName = nullptr);
    ~xevent();
  public:
    bool wait(const DWORD time = INFINITE);     /// 等待事件。
    bool set();                                 /// 激发事件。
    bool reset();                               /// 设置事件未激发。
  public:
    xevent(const xevent&) = delete;
    xevent& operator=(const xevent&) = delete;
  private:
    const HANDLE handle;
  };

#endif  // FOR_RING0

#endif  // _WIN32

#endif  // _XLIB_XEVENT_H_
/**
  \file  xlog.h
  \brief 定义了日志组织与输出相关的类。

  \version    2.2.0.201112

  \author     triones
  \date       2011-07-22

  \section history 版本记录

  - 2011-07-22 新建 xlog 。 0.1 。
  - 2011-07-30 增加 xlog::dynamic_lvl 用以控制动态输出等级。 0.1.1 。
  - 2011-07-30 增加 xlog_static_lvl 宏用以控制静态输出等级。
  - 2011-07-30 新加 endx 函数，及相应操作符。
  - 2011-07-30 基本实现完全替换 dbgout 的功能，故从 LIB 中去除 dbgout 。
  - 2011-08-20 引入 mem_base_base 使 xlog 继承之。 0.2 。
  - 2011-09-08 修复 prt 函数中自动扩展的 BUG 。 0.2.1 。
  - 2011-10-17 将 xlog 的格式化处理提取形成 xlog_base ，方便继承用作其它用途。 0.3 。
  - 2011-12-12 移植入 COMMON ，对 xlog 新增一个静态变量用以类型控制，同时增加一批相关控制宏。
  - 2011-12-12 自此引入类型控制输出功能。 0.4 。
  - 2012-04-24 对 xlog_base 新加一个输入函数 operator<<(const xlog_base& v) 。 0.5 。
  - 2012-06-06 把 xlog_base 分离出新文件，重命名为 xmsg 。 xlog 继承之。 0.6 。
  - 2012-09-25 决定三类输出控制分别独立。 1.0 。
  - 2012-10-24 由于 xmsg 包含结尾 0 ， out 函数不再追加。 1.0.1 。
  - 2016-11-15 适配 Linux g++ 。 1.1 。
  - 2019-07-05 添加消息分段功能，以应对 DebugView 在多条消息过长时，出现卡顿。 1.1.1 。
  - 2019-09-26 重构 xlog 。去除动态控制与类型控制。 2.0 。
  - 2019-11-05 改进声明。 2.1 。
  - 2020-11-12 适配 xmsg 升级。 2.2 。
*/
#ifndef _XLIB_XLOG_H_
#define _XLIB_XLOG_H_

#include "xmsg.h"

/// 允许设置 XLOGOUT 改变 xlog 默认输出行为。
#ifndef XLOGOUT
  #ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #define XLOGOUT(msg) OutputDebugStringA(msg);
  #else
    #include <iostream>
    #define XLOGOUT(msg) std::wcout << (msg) << std::endl;
  #endif
#endif

// 允许设置 XLOG_MAX_BYTES 用于消息过长分段输出。
#ifndef XLOG_MAX_BYTES
#define XLOG_MAX_BYTES 0
#endif
/**
  xlog 用于基本的调试信息输出。
  
  - 一般不直接使用，而是通过宏定义间接使用。\n
  - **注意** 类本身没有输出控制（节省资源，加快运行），需要通过宏完成。（宏的具体操作参见之后说明）
  - 如果需要扩展功能，如输出到文件等，可选择继承之，或仿造实现之。
*/
class xlog : public xmsg
  {
  public:
    enum xlog_level
      {
      off,    ///< 屏蔽输出。
      fatal,  ///< 致命错误，程序无法继续执行。
      error,  ///< 反映错误，例如一些 API 的调用失败。
      warn,   ///< 反映某些需要注意的可能有潜在危险的情况，可能会造成崩溃或逻辑错误之类。
      info,   ///< 表示程序进程的信息。
      debug,  ///< 普通的调试信息，这类信息发布时一般不输出。
      trace,  ///< 最精细的调试信息，多用于定位错误，查看某些变量的值。
      on,     ///< 全输出。
      };
  public:
    virtual ~xlog()
      {
      do_out();
      }
    xlog& do_out()
      {
      if(empty())  return *this;
      if constexpr (0 == XLOG_MAX_BYTES)
        {
        XLOGOUT(c_str());
        clear();
        return *this;
        }
      // 定宽的 UNICODE 避免多字节字符串切分出现截断现象。
#if XMSG_BASE == XMSG_BASE_UTF8
      const std::wstring buf(u82ws(*this));
#elif XMSG_BASE == XMSG_BASE_UNICODE
      const std::wstring buf(*this);
#else
      const std::wstring buf(as2ws(*this));
#endif
      clear();
      for(size_t i = 0; i < buf.size();)
        {
        // 如果内部自带换行，则避免过多的切分。
        const auto it = buf.begin() + i;
        const auto pos = buf.find(L'\n', i);
        if((npos != pos) && ((pos - i) <= (XLOG_MAX_BYTES)))
          {
          const auto xit = buf.begin() + pos + 1;
          XLOGOUT(xmsg_base_ws(std::wstring(it, xit)).c_str());
          i = pos + 1;
          continue;
          }
        const auto xit = (it + (XLOG_MAX_BYTES) < buf.end()) ? it + (XLOG_MAX_BYTES) : buf.end();
        XLOGOUT(xmsg_base_ws(std::wstring(it, xit)).c_str());
        i += (XLOG_MAX_BYTES);
        }
      return *this;
      }
  };

/**
  控制静态编译结果。**注意** 宏的作用是局部的，不同 CPP 可以设置不同的静态控制等级。

  - 前置设置控制等级：
  \code
    #define xlog_static_lvl 1 // 静态控制等级为 fatal ，只输出最严重的错误。
    #include "xlog.h"
  \endcode
  
  - 后置修改控制等级：
  \code
    #include "xlog.h"
    #undef xlog_static_lvl
    #define xlog_static_lvl xlog::warn
  \endcode
*/
#ifndef xlog_static_lvl
#define xlog_static_lvl xlog::on
#endif

/**
  以下宏用于分级静态控制编译结果，根据 xlog_static_lvl 以决定指定调试信息是否被编译。

  \code
    xerr << "xerr";   // 当 xlog_static_lvl < xlog::error 时，此句不被编译。
    xfail << "xfail"; // 除非 xlog_static_lvl == xlog::off ，否则此句输出。
  \endcode
*/
#define xlog_do(v) if constexpr ((v) <= xlog_static_lvl) xlog()

#define xtrace  xlog_do(xlog::trace)
#define xdbg    xlog_do(xlog::debug)
#define xinfo   xlog_do(xlog::info)
#define xwarn   xlog_do(xlog::warn)
#define xerr    xlog_do(xlog::error)
#define xfail   xlog_do(xlog::fatal)

/**
  便捷宏，用于便捷插入函数名及行号。

  \code
    xerr << xfuninfo << "这里出错";
  \endcode
*/
#define xfuninfo XMSG_TEXT("[" __FUNCTIONW__ "][") << __LINE__ << XMSG_TEXT("]: ")
/**
  便捷宏，用于便捷插入异常产生的函数。

  \code
    xerr << xfunexpt;
  \endcode
*/
#define xfunexpt XMSG_TEXT("[" __FUNCTION__ "]: exception.")

#endif  // _XLIB_XLOG_H_
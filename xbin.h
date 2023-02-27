/**
  \file  xbin.h
  \brief 定义了便捷数据组织的类，常用于封包的组织与解析。

  \version    2.1.1.230227

  \author     triones
  \date       2010-03-26

  \section more 额外说明

  - 数据不足的情况下读取数据默认将抛出 runtime_error 异常。
  - 如不愿意抛出异常，请在包含前 #define XBIN_NOEXCEPT 。注意，此时将读出未知数据。
  - xbin 类似 struct 库。不采用 pack 、 unpack 形式，有如下理由：
      - C 的类型明确， pack 时无需再指定格式化。
      - C 无法在 unpack 时返回多值。

  \section history 版本记录

  - 2010-03-30 新加 end() 、 remain() 操作及 += 、 -= 操作。 0.0.1 。
  - 2010-04-02 增强 setstr() 功能。
  - 2010-04-03 增强 getstr() 功能。 0.0.2 。
  - 2010-04-12 新加复制构造函数，现在可以应用于容器。另对几个函数加上 const 以适应一些操作。 0.0.3 。
  - 2010-05-10 本要改成 TCHAR* 异常，但考虑特殊性，决定不采用，为记。
  - 2010-06-09 考虑到有初始时就需要非网络顺序，新加一个构造函数控制，而不去改变默认构造函数。
  - 2010-06-09 新增加一个功能 line& operator<<(line& (*pfn)(line&)) 。 0.0.4 。
  - 2010-10-08 增加一些成员函数以避免类型转换时可能导致困惑。 0.0.5 。
  - 2010-10-08 但考虑还是保留原类型转换以保持向下兼容。
  - 2010-11-06 移植入 LIB ，除了模板保留在头文件，其它小函数也不再 inline ，全部转移到 CPP 。 0.1 。
  - 2010-11-23 对 Ring0 支持。 Ring0 下，异常不抛出，但也暂时不知如何处理，结果无法预料。 0.2 。
  - 2010-11-23 实际改动只是移除异常抛出而已。
  - 2010-11-27 加进一些重载，增加灵活性。另外考虑隐式转换，重新调整函数声明顺序。 0.2.1 。
  - 2010-12-11 发现 bswap 对 bool 、结构体类型的支持上的 BUG ，修正之。 0.2.2 。
  - 2010-12-21 发现 isempty 函数错误，改正之。 0.2.3 。
  - 2010-12-21 发现 >> 重载与模板作用冲突，模板覆盖重载，暂时没有解决方案。
  - 2010-12-28 由于 setlen 函数名称的歧义，改造之并将原始功能定义为 lptolen 。
  - 2010-12-28 删除原内置模板 bswap 。采用外部 bswap 模板。 0.3 。
  - 2010-12-28 考虑把 >> 、 << 操作符转移到外部函数，考虑良久，暂不行动。为记。
  - 2011-02-11 改动异常的抛出为 TCHAR 。 0.3.1 。
  - 2011-02-18 提升 netline 为 line 类。 0.4 。
  - 2011-02-18 类中嵌入快捷缓冲（闪存），多数情况下能在堆栈直接分配空间，避免经常分配内存。
  - 2011-02-18 注意这次的改动，数据格式默认为非网络顺序。
  - 2011-02-18 去除数据类型转换操作，不再支持隐式转换。
  - 2011-02-18 一旦申请缓冲，所有数据都将会在缓冲中，不再使用闪存。
  - 2011-02-18 这次更新，缓冲只扩展，不再缩减。
  - 2011-02-18 优化 getbuf 、 end 、 getstr 、 setstr ，优化对指针的处理。
  - 2011-04-18 屡次考虑变化计数器为 us 以缩减类大小，但权衡下保持为 ui。为记。
  - 2011-04-18 决定缩减快捷缓冲大小以减小堆栈压力。
  - 2011-04-19 重新调整 net 、 fix 标志，并增加 zeroend 、 otherhead 、 headsize 标志。 0.5 。
  - 2011-04-19 为新加的标志提供实现功能； 0.6 。
  - 2011-04-19 本来考虑设置统一函数管理标志，这样导致代码不清晰及使用困难，作罢，为记；
  - 2011-04-19 重新调整构造函数，使 line 在构造时，能影响多个标志，保持了向下兼容的 line(true) 。
  - 2011-04-20 修正昨天功能升级后的处理指针流 BUG 。
  - 2011-04-20 顺便改了 setmax 的一些小细节，优化处理流程以提高运行效率。 0.6.1 。
  - 2011-04-20 正式移除 LIB 中的 netline 类。
  - 2011-05-26 增加成员函数 copystate 以复制状态。 0.6.2 。
  - 2011-06-18 修改 getstr 、 setstr 使其具有返回值，返回值即处理数据长度。 0.6.3 。
  - 2011-06-22 新加 cnull 宏，方便处理 ASCII 时追加结尾 0 。 0.6.4 。
  - 2011-07-15 调整 line_block ， size_head ；增加构造初始化对 size_head 的控制；
  - 2011-07-15 调整构造函数；新增 hideerr 标志；调整 needmax 、 needlen 函数；
  - 2011-07-15 新增 initstate 函数；调整 init 函数；
  - 2011-07-15 去除 lptolen 这个危险函数；调整 setmax 函数；增加 snull 宏。 0.6.5 。
  - 2011-07-16 修改函数命名以贴近标准容器操作， getbuf == begin ； getlen == size ； isempty == empty ；
  - 2011-07-16 去除 operator=(const unsigned int) ；去除 operator+=(const unsigned int) ；
  - 2011-07-16 调整 operator=(const line& nline) ；调整 operator+=(const line& nline) ；
  - 2011-07-16 调整 setstr 、 getstr 函数；添加 operator>>(T const&) 。 0.7 。
  - 2011-07-20 修复 setstr 转入 CPP 造成的诡异无法解释的编码错误。 0.7.1 。
  - 2011-08-20 调整 line 类继承 mem_base_base 类。 0.8 。
  - 2011-08-20 此次封装提取了一些共同函数。同时删除了 fix 属性。 0.8.1 。
  - 2011-09-16 修复 setstr 前缀处理的 BUG 。 0.8.2 。
  - 2011-10-31 增加函数 mkhead 。 0.8.3 。
  - 2011-12-14 移入 COMMON ，由于 mem_base 的变化，删除两个不再需要的重载。 0.8.4 。
  - 2012-01-30 发现 mkhead 的一个 BUG ，已修正。 0.8.5 。
  - 2012-05-25 状态修改函数添加前缀 re 。 0.8.6 。
  - 2012-05-28 针对基类变化做了一些调整。 0.8.7 。
  - 2012-06-06 重新设计 line ，升级为 xline 。 1.0 。
  - 2012-06-06 此次升级，所有状态以模版参数设置，不再接受状态临时变化。
  - 2012-06-06 保留 line 的定义，同时增加 netline 的定义。
  - 2012-09-06 修复 >>(void*) 的隐藏 BUG 。 1.0.1 。
  - 2013-03-15 修复 <<(netline) 的 size 两次翻转的 BUG 。 1.0.2 。
  - 2013-09-30 转移 setstr 、 getstr 到 xvec 成为 put 、 pick 。 1.0.3 。
  - 2014-01-13 引入 SGISTL ，作适应性改动。 1.1 。
  - 2015-01-22 为数据不足情况补充可选择的异常机制。 1.2 。
  - 2016-11-15 适配 Linux g++ 。添加接口。 1.3 。
  - 2017-09-06 改进处理 >>(T*) 。 1.3.1 。
  - 2019-10-17 重构，升级为 xbin 。 2.0 。
  - 2019-11-06 再次重构，合并 vbin 。 2.1 。
  - 2020-03-13 适配 varint 优化。 2.1.1 。
*/
#ifndef _XLIB_XBIN_H_
#define _XLIB_XBIN_H_

#include <string.h>

#include <stdexcept>
#include <string>

#include "xswap.h"
#include "xvarint.h"

namespace xlib {

/**
  xbin 用于便捷的数据组织操作。
  \param  headtype    数据头类型， byte , word , dword , qword , void。
  \param  headself    指示数据头是否包含自身长度。
  \param  bigendian   输入输出的数据是否转换成大端序（默认小端）。
  \param  zeroend     指示处理流时是否追加处理结尾 0 。
*/
template <typename headtype, bool headself, bool bigendian, bool zeroend>
class xbin : public std::basic_string<uint8_t> {
 public:
  /*========================  数据输入  ========================*/
  /**
    \code
      xbin << (void*)p;
    \endcode
  */
  xbin& operator<<(const void* const p) {
    return operator<<((size_t)p);
  }
  xbin& operator<<(void* const p) {
    return operator<<((size_t)p);
  }
  /**
    \code
      xbin << true;
    \endcode
  */
  xbin& operator<<(bool const b) {
    return operator<<((uint8_t)b);
  }
  /**
    \code
      xbin << "2121321";
      xbin << L"12312";
    \endcode
  */
  template <typename T>
  xbin& operator<<(const T* const str) {
    if (str == nullptr) return *this;

    size_t len = 0;
    while (str[len]) ++len;
    if constexpr (!std::is_void_v<headtype>) len += zeroend ? 1 : 0;

    append((xbin::const_pointer)str, len * sizeof(T));
    return *this;
  }
  /**
    \code
      xbin << xbin;
    \endcode
  */
  xbin& operator<<(const xbin& bin) {
    if (&bin == this) return mkhead();

    if constexpr (!std::is_void_v<headtype>) {
      const size_t nlen = bin.size() + (headself ? sizeof(headtype) : 0);
      operator<<((headtype)nlen);
    } else {
      operator<<(bin.size());
    }

    append(bin.begin(), bin.end());
    return *this;
  }
  /**
    \code
      xbin << string("12345678");
    \endcode
  */
  template <typename T>
  xbin& operator<<(const std::basic_string<T>& s) {
    append((xbin::const_pointer)s.c_str(), s.size() * sizeof(T));
    return *this;
  }
  /**
    \code
      xbin << dword << word << byte;
    \endcode
  */
  template <typename T>
  std::enable_if_t<std::is_integral_v<T> || std::is_enum_v<T>, xbin>&
  operator<<(const T& argvs) {
    if constexpr (!std::is_void_v<headtype>) {
      const auto v = bigendian ? bswap(argvs) : argvs;
      append((xbin::const_pointer)&v, sizeof(v));
    } else {
      const xvarint v(argvs);
      append((xbin::const_pointer)v.data(), v.size());
    }
    return *this;
  }

  xbin& mkhead() {
    if constexpr (!std::is_void_v<headtype>) {
      const auto nlen = size() + (headself ? sizeof(headtype) : 0);
      const auto v = (bigendian ? bswap((headtype)nlen) : (headtype)nlen);
      insert(0, (xbin::const_pointer)&v, sizeof(v));
    } else {
      const xvarint v(size());
      insert(0, (xbin::const_pointer)v.data(), v.size());
    }
    return *this;
  }

 public:
  /*========================  数据输出  ========================*/
  /**
    \code
      xbin >> (void*)p;
    \endcode
  */
  xbin& operator>>(void*& p) {
    return operator>>((size_t&)p);
  }
  /**
    \code
      bool b;
      xbin >> b;
    \endcode
  */
  xbin& operator>>(bool& b) {
    return operator>>(*(uint8_t*)&b);
  }
  /**
    \code
      xbin >> (char*)lpstr;
    \endcode
    \exception  数据不足时，抛出 runtime_error 异常。
    \note       允许空指针，这样将丢弃一串指定类型的数据。
  */
  template <typename T>
  xbin& operator>>(T* str) {
    const T* lpstr = (const T*)c_str();

    size_t strlen = 0;
    while (lpstr[strlen]) ++strlen;
    if constexpr (!std::is_void_v<headtype>) strlen += zeroend ? 1 : 0;
    strlen *= sizeof(T);

#ifndef XBIN_NOEXCEPT
    if (strlen > size()) {
      throw std::runtime_error("xbin >> T* not enough data");
    }
#endif

    if (nullptr != str) memcpy(str, lpstr, strlen);

    erase(0, strlen);

    return *this;
  }
  /**
    \code
      xbin >> xbin;
    \endcode
    \exception  数据不足时，抛出 runtime_error 异常。
    \note       当操作自身时，按数据头长度截断。
  */
  xbin& operator>>(xbin& bin) {
    size_t nlen;
    if constexpr (!std::is_void_v<headtype>) {
      headtype xlen;
      operator>>(xlen);

      nlen = (headtype)xlen;
      nlen -= (headself ? sizeof(headtype) : 0);
    } else {
      operator>>(nlen);
    }

#ifndef XBIN_NOEXCEPT
    if (nlen > size()) {
      throw std::runtime_error("xbin >> xbin& not enough data");
    }
#endif

    if (&bin == this) {
      erase(begin() + nlen, end());
      return *this;
    }

    bin.assign(c_str(), nlen);
    erase(0, nlen);

    return *this;
  }
  /**
    模板适用于标准库字符串。数据倾倒。

    \code
      xbin >> string;
    \endcode
  */
  template <typename T>
  xbin& operator>>(std::basic_string<T>& s) {
    s.assign((const T*)c_str(), size() / sizeof(T));
    clear();
    return *this;
  }
  /**
    模板适用于内置类型，结构体等。

    \code
      xbin >> dword >> word >> byte;
    \endcode
    \exception 数据不足时，抛出 runtime_error 异常。
  */
  template <typename T>
  inline std::enable_if_t<std::is_integral_v<T> || std::is_enum_v<T>, xbin>&
  operator>>(T& argvs) {
    if constexpr (!std::is_void_v<headtype>) {
#ifndef XBIN_NOEXCEPT
      if (sizeof(T) > size()) {
        throw std::runtime_error("xbin >> T& not enough data");
      }
#endif
      memcpy(&argvs, c_str(), sizeof(T));
      erase(0, sizeof(T));
      argvs = bigendian ? bswap(argvs) : argvs;
    } else {
      const xvarint<T> vi(data());
      argvs = vi;
      const size_t typesize = vi.size();
#ifndef XBIN_NOEXCEPT
      if (typesize == 0 || typesize > size()) {
        throw std::runtime_error("xbin >> T& not enough data / data error");
      }
#endif
      erase(0, typesize);
    }

    return *this;
  }
  /**
    目的用以跳过某些不需要的数据。

    \code
      xbin >> cnull >> snull >> 0;
    \endcode
    \exception 数据不足时，抛出 runtime_error 异常。
  */
  template <typename T>
  xbin& operator>>(const T&) {
    if constexpr (!std::is_void_v<headtype>) {
#ifndef XBIN_NOEXCEPT
      if (sizeof(T) > size()) {
        throw std::runtime_error("xbin >> xbin& not enough data");
      }
#endif
      erase(0, sizeof(T));
    } else {
      T argvs;
      return operator>>(argvs);
    }

    return *this;
  }
};

/// lbin 数据头为 word ，不包含自身，小端序，不处理结尾 0 。
using lbin = xbin<uint16_t, false, false, false>;
/// gbin 数据头为 word ，不包含自身，大端顺序，处理结尾 0 。
using gbin = xbin<uint16_t, false, true, true>;
/// vbin 为 xvarint 格式，忽略后继所有设置。
using vbin = xbin<void, false, false, false>;

}  // namespace xlib

#endif  // _XLIB_XBIN_H_
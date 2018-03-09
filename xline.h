/*!
  \file  xline.h
  \brief xline.h定义了便捷数据组织的类，常用于封包的组织与解析

  - 数据不足的情况下读取数据默认将抛出runtime_error异常
  - 如不愿意抛出异常，请在包含前#define xline_noexcept。注意，此时将读出未知数据
  - Ring0不使用异常机制
  - xline类似struct库，但由于C无法返回多值，故采用此形式

  \version    10.1.1709.0616
  \note       For All

  \author     triones
  \date       2010-03-26
*/
#ifndef _XLIB_XLINE_H_
#define _XLIB_XLINE_H_

#include <string.h>
#include <string>
#include <stdexcept>

#include "swap.h"

/*!
  xline用于便捷的数据组织操作
  \param  headtype    数据头类型，byte, word, dword, qword
  \param  headself    指示数据头是否包含自身长度
  \param  bigendian   输入输出的数据是否转换成大端序（默认小端）
  \param  zeroend     指示处理流时是否追加处理结尾0
*/
template<
          typename  headtype,
          bool      headself,
          bool      bigendian,
          bool      zeroend
        >
class xline : public std::basic_string<uint8>
  {
  public:
    typedef xline<headtype, headself, bigendian, zeroend>   _Myt;
    typedef std::basic_string<uint8>                        _Mybase;
  public:
    /*========================  数据输入  ========================*/
    /*!
      \code
        xline << (void*)p;
      \endcode
    */
    _Myt& operator<<(void* p)
      {
      return this->operator<<((size_t)p);
      }
    /*!
      格式串输入\n
      注意：输入数据内容，根据标志处理结尾0

      \code
        xline << "2121321";
        xline << L"12312";
      \endcode
    */
    template<typename T> _Myt& operator<<(T* str)
      {
      if(str == nullptr)  return *this;

      size_type len = 0;
      while(str[len])  ++len;

      len += zeroend ? 1 : 0;

      _Mybase::append((const uint8*)str, len * sizeof(T));

      return *this;
      }
    /*!
      先输入数据长度（输入格式由net决定），再输入数据内容\n
      注意：当操作自身时，相当于mkhead()

      \code
        xline << xline;
      \endcode
    */
    _Myt& operator<<(const _Myt& nline)
      {
      if(&nline == this)
        {
        return mkhead();
        }

      const size_type nlen = nline.size() + (headself ? sizeof(headtype) : 0);

      this->operator<<((headtype)nlen);
      _Mybase::append(nline.begin(), nline.end());

      return *this;
      }
    /*!
      模板适用于标准库字符串

      \code
        xline << string("12345678");
      \endcode
    */
    template<typename T> _Myt& operator<<(const std::basic_string<T>& s)
      {
      _Mybase::append((const uint8*)s.c_str(), s.size() * sizeof(T));

      return *this;
      }
    /*!
      模板适用于内置类型，结构体等

      \code
        xline << dword << word << byte;
      \endcode
    */
    template<typename T> _Myt& operator<<(const T& argvs)
      {
      const T v = bigendian ? bswap(argvs) :argvs;

      _Mybase::append((const uint8*)&v, sizeof(v));

      return *this;
      }
    //! 扩展功能，使得line可以执行一个函数
    _Myt& operator<<(_Myt& (*pfn)(_Myt&))
      {
      return pfn(*this);
      }
    //! 扩展功能，使得不借助两个line就能完成头部添加
    _Myt& mkhead()
      {
      const size_type nlen = _Mybase::size() + (headself ? sizeof(headtype) : 0);
      const headtype v = (bigendian ? bswap((headtype)nlen) : (headtype)nlen);

      _Mybase::insert(0, (const uint8*)&v, sizeof(v));

      return *this;
      }
  public:
    /*========================  数据输出  ========================*/
    /*!
      \code
        xline >> (void*)p;
      \endcode
    */
    _Myt& operator>>(void*& p)
      {
      return this->operator>>((size_t&)p);
      }
    /*!
      格式串输出，\n
      输出数据内容，根据标志处理结尾0\n
      允许空指针，这样将丢弃一串指定类型的数据

      \code
        xline >> (char*)lpstr;
      \endcode
      \exception 数据不足时，抛出runtime_error异常
    */
    template<typename T> _Myt& operator>>(T* str)
      {
      const T* lpstr = (const T*)_Mybase::c_str();

      size_type strlen = 0;
      while(lpstr[strlen])++strlen;
      strlen += zeroend ? 1 : 0;
      strlen *= sizeof(T);

#if !defined(FOR_RING0) && !defined(xline_noexcept)
      if(strlen > _Mybase::size())
        {
        throw std::runtime_error("xline >> T* 数据不足");
        }
#endif

      if(str != nullptr)
        memcpy(str, lpstr, strlen);

      _Mybase::erase(0, strlen);

      return *this;
      }
    /*!
      将重置参数line\n
      执行后line缓冲存放数据内容，line的缓冲长度即为数据长度。\n
      注意：当操作自身时，按数据头长度截断

      \code
        xline >> xline;
      \endcode
      \exception 数据不足时，抛出runtime_error异常
    */
    _Myt& operator>>(_Myt& nline)
      {
      headtype xlen;
      this->operator>>(xlen);

      size_type nlen = xlen;
      nlen -= (headself ? sizeof(headtype) : 0);

#if !defined(FOR_RING0) && !defined(xline_noexcept)
      if(nlen > _Mybase::size())
        {
        throw std::runtime_error("xline >> _Myt& 数据不足");
        }
#endif

      if(&nline == this)
        {
        auto it = _Mybase::begin() + nlen;
        _Mybase::erase(it, _Mybase::end());
        return *this;
        }

      nline.assign(_Mybase::c_str(), nlen);
      _Mybase::erase(0, nlen);

      return *this;
      }
    /*!
      模板适用于标准库字符串。数据倾倒

      \code
        xline >> string;
      \endcode
    */
    template<typename T> _Myt& operator>>(std::basic_string<T>& s)
      {
      s.assign((const T*)_Mybase::c_str(), _Mybase::size() / sizeof(T));
      _Mybase::clear();

      return *this;
      }
    /*!
      模板适用于内置类型,结构体等

      \code
        xline >> dword >> word >> byte;
      \endcode
      \exception 数据不足时，抛出runtime_error异常
    */
    template<typename T> _Myt& operator>>(T& argvs)
      {
      size_type typesize = sizeof(T);

#if !defined(FOR_RING0) && !defined(xline_noexcept)
      if(typesize > _Mybase::size())
        {
        throw std::runtime_error("xline >> T& 数据不足");
        }
#endif

      memcpy(&argvs, _Mybase::c_str(), typesize);
      _Mybase::erase(0, typesize);
      argvs = bigendian ? bswap(argvs) : argvs;

      return *this;
      }
    /*!
      目的用以跳过某些不需要的数据

      \code
        xline >> cnull >> snull >> 0;
      \endcode
      \exception 数据不足时，抛出runtime_error异常
    */
    template<typename T> _Myt& operator>>(T const&)
      {
#if !defined(FOR_RING0) && !defined(xline_noexcept)
      if(sizeof(T) > _Mybase::size())
        {
        throw std::runtime_error("xline >> _Myt& 数据不足");
        }
#endif
      _Mybase::erase(0, sizeof(T));

      return *this;
      }
  };

//! line数据头为word，不包含自身，小端序，不处理结尾0
typedef xline<uint16, false, false, false>   line;
//! netline数据头为word,不包含自身，大端顺序，处理结尾0
typedef xline<uint16, false, true, true>     netline;

#endif  // _XLIB_XLINE_H_
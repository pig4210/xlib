/*!
  \file  vline.h
  \brief vline.h定义了便捷数据组织的类，常用于封包的组织与解析

  - 数据不足的情况下读取数据默认将抛出runtime_error异常
  - 如不愿意抛出异常，请在包含前#define vline_noexcept。注意，此时将读出未知数据
  - Ring0不使用异常机制
  - 注意与xline的区别。vline可用于protobuf

  \version    1.0.1709.0609
  \note       For All

  \author     triones
  \date       2017-09-06
*/
#ifndef _XLIB_VLINE_H_
#define _XLIB_VLINE_H_

#include <string.h>
#include <string>
#include <stdexcept>

#include "varint.h"

/*!
  vline用于便捷的数据组织操作
  */
class vline : public std::basic_string<uint8>
  {
  public:
    typedef vline                     _Myt;
    typedef std::basic_string<uint8>  _Mybase;
  public:
    /*========================  数据输入  ========================*/
    /*!
      \code
        vline << (void*)p;
      \endcode
    */
    _Myt& operator<<(void* p)
      {
      return this->operator<<((size_t)p);
      }
    /*!
      \code
        vline << true;
      \endcode
    */
    _Myt& operator<<(const bool b)
      {
      return this->operator<<((uint8)b);
      }
    /*!
      格式串输入\n
      注意：输入数据内容，不处理结尾0

      \code
        vline << "2121321";
        vline << L"12312";
      \endcode
    */
    template<typename T> _Myt& operator<<(T* str)
      {
      if(str == nullptr)  return *this;

      size_type len = 0;
      while(str[len])  ++len;

      _Mybase::append((const uint8*)str, len * sizeof(T));

      return *this;
      }
    /*!
      先输入数据长度，再输入数据内容\n
      注意：当操作自身时，相当于mkhead()

      \code
        vline << vline;
      \endcode
    */
    _Myt& operator<<(const _Myt& nline)
      {
      if(&nline == this)
        {
        return mkhead();
        }

      this->operator<<(nline.size());
      _Mybase::append(nline.begin(), nline.end());

      return *this;
      }
    /*!
      模板适用于标准库字符串

      \code
        vline << string("12345678");
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
        vline << dword << word << byte;
      \endcode
    */
    template<typename T>
    typename std::enable_if<std::is_class<T>::value, _Myt>::type&
      operator<<(const T& argvs)
      {
      const uint8* lp = (const uint8*)&argvs;
      size_t len = sizeof(argvs);
      while(len != 0)
        {
        switch(len)
          {
          case 1:
            this->operator<<(tovarint(*(uint8*)lp));
            lp += sizeof(uint8);
            len -= sizeof(uint8);
            break;
          case 2:
          case 3:
            this->operator<<(tovarint(*(uint16*)lp));
            lp += sizeof(uint16);
            len -= sizeof(uint16);
            break;
          case 4:
          case 5:
          case 6:
          case 7:
            this->operator<<(tovarint(*(uint32*)lp));
            lp += sizeof(uint32);
            len -= sizeof(uint32);
            break;
          default:
            this->operator<<(tovarint(*(uint64*)lp));
            lp += sizeof(uint64);
            len -= sizeof(uint64);
            break;
          }
        }
      return *this;
      }
    template<typename T>
    typename std::enable_if<!std::is_class<T>::value, _Myt>::type&
      operator<<(const T& argvs)
      {
      this->operator<<(tovarint(argvs));
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
      const auto s = tovarint(_Mybase::size());

      _Mybase::insert(0, (const uint8*)s.c_str(), s.size());

      return *this;
      }
  public:
    /*========================  数据输出  ========================*/
    /*!
      \code
        vline >> (void*)p;
      \endcode
    */
    _Myt& operator>>(void*& p)
      {
      return this->operator>>((size_t&)p);
      }
    /*!
      \code
        vline >> (bool)b;
      \endcode
    */
    _Myt& operator>>(bool& b)
      {
      return this->operator>>((uint8&)b);
      }
    /*!
      格式串输出，注意不能特化void* !\n
      输出数据内容，根据标志处理结尾0\n
      允许空指针，这样将丢弃一串指定类型的数据

      \code
        vline >> (unsigned char*)lpstr;
      \endcode
      \exception 数据不足时，抛出runtime_error异常
    */
    template<typename T> _Myt& operator>>(T* str)
      {
      T* lpstr = (T*)_Mybase::c_str();

      size_type strlen = 0;
      while(lpstr[strlen])++strlen;
      strlen *= sizeof(T);

#if !defined(FOR_RING0) && !defined(xline_noexcept)
      if(strlen > _Mybase::size())
        {
        throw std::runtime_error("vline >> T* 数据不足");
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
        vline >> vline;
      \endcode
      \exception 数据不足时，抛出runtime_error异常
    */
    _Myt& operator>>(_Myt& nline)
      {
      size_type nlen;
      this->operator>>(nlen);

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
        vline >> string;
      \endcode
    */
    template<typename T> _Myt& operator>>(std::basic_string<T>& s)
      {
      s.assign((const T*)_Mybase::c_str(), _Mybase::size() / sizeof(T));
      _Mybase::clear();

      return *this;
      }
    /*!
      模板适用于内置类型，结构体等

      \code
        vline >> dword >> word >> byte;
      \endcode
      \exception 数据不足时，抛出runtime_error异常
      */
    template<typename T>
    typename std::enable_if<std::is_class<T>::value, _Myt>::type&
      operator>>(T& argvs)
      {
      const uint8* lp = (const uint8*)&argvs;
      size_t len = sizeof(argvs);
      while(len != 0)
        {
        switch(len)
          {
          case 1:
            this->operator>>(*(uint8*)lp);
            lp += sizeof(uint8);
            len -= sizeof(uint8);
            break;
          case 2:
          case 3:
            this->operator>>(*(uint16*)lp);
            lp += sizeof(uint16);
            len -= sizeof(uint16);
            break;
          case 4:
          case 5:
          case 6:
          case 7:
            this->operator>>(*(uint32*)lp);
            lp += sizeof(uint32);
            len -= sizeof(uint32);
            break;
          default:
            this->operator>>(*(uint64*)lp);
            lp += sizeof(uint64);
            len -= sizeof(uint64);
            break;
          }
        }
      return *this;
      }
    template<typename T>
    typename std::enable_if<!std::is_class<T>::value, _Myt>::type&
      operator>>(T& argvs)
      {
      size_type typesize = getvarint(argvs, *this);
#if !defined(FOR_RING0) && !defined(xline_noexcept)
      if(typesize == 0 || typesize > _Mybase::size())
        {
        throw std::runtime_error("vline >> T& 数据错误/不足");
        }
#endif
      _Mybase::erase(0, typesize);
      return *this;
      }
    /*!
      目的用以跳过某些不需要的数据

      \code
        vline >> cnull >> snull >> 0;
      \endcode
      \exception 数据不足时，抛出runtime_error异常
    */
    template<typename T> _Myt& operator>>(T const&)
      {
      T argvs;
      return this->operator>>(argvs);
      }
  };

#endif  // _XLIB_VLINE_H_
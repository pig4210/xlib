/*!
  \file  md5.h
  \brief md5.h定义了md5算法

  \version    3.0.1711.1715
  \note       For All

  \author     triones
  \date       2013-01-04
*/
#ifndef _XLIB_MD5_H_
#define _XLIB_MD5_H_

#include <string>
#include <string.h>

#include "xlib_base.h"

typedef uint32  MD5_WORD;

//! MD5值是一个128位的散列值，由四个32位分组级联组成
class MD5_VALUE
  {
  public:
    MD5_VALUE(const MD5_WORD a,
              const MD5_WORD b,
              const MD5_WORD c,
              const MD5_WORD d);
    template<typename T> operator std::basic_string<T>()
      {
      return std::basic_string<T>((const T*)this, sizeof(*this) / sizeof(T));
      }
  public:
    MD5_WORD A;
    MD5_WORD B;
    MD5_WORD C;
    MD5_WORD D;
  };

//! md5算法
/*!
  \param data   需要计算MD5的数据指针
  \param size   需要计算MD5的数据大小
  \return       返回md5结果

  \code
    std::string data;
    std::cin >> data;
    std::cout << showbin(string(md5(data))) << endl;
    //MD5 ("") = d41d8cd98f00b204e9800998ecf8427e
　　//MD5 ("a") = 0cc175b9c0f1b6a831c399e269772661
　　//MD5 ("abc") = 900150983cd24fb0d6963f7d28e17f72
　　//MD5 ("message digest") = f96b697d7cb7938d525a2f31aaf161d0
　　//MD5 ("abcdefghijklmnopqrstuvwxyz") = c3fcd3d76192e4007dfb496cca67e13b
　　//MD5 ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz") =
　　//f29939a25efabaef3b87e2cbfe641315
  \endcode
*/
MD5_VALUE md5(const void* data, const size_t size);

//////////////////////////////////////////////////////////////////////////
template<typename T> MD5_VALUE md5(const std::basic_string<T>& s)
  {
  return md5(s.c_str(), s.size() * sizeof(T));
  }

#endif  // _XLIB_MD5_H_
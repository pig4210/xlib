/*!
  \file  des.h
  \brief des.h定义了des算法

  - 参考来自http://pangzi.is-programmer.com/posts/25161.html
  - 参考来自http://www.cnblogs.com/erwin/archive/2009/04/14/1435288.html

  \version    3.0.1707.2509
  \note       For All

  \author     triones
  \date       2013-10-21
*/
#ifndef _XLIB_DES_H_
#define _XLIB_DES_H_

#include <string>
#include <string.h>

#include "xlib_base.h"

class DesKey
  {
  public:
    /*!
      指定key串。8 byte （64 bit）\n
      当size == 0时，视key为0结尾的ASCII字符串\n
      截断过多数据；以\x00补足不足数据
    */
    DesKey(const void* key, size_t size = 0);
    template<typename T> DesKey(const std::basic_string<T>& key)
      {
      memset(Keys, 0, sizeof(Keys));
      size_t size = key.size() * sizeof(T);
      if(size > sizeof(Keys)) size = sizeof(Keys);
      memcpy(Keys, key.c_str(), size);
      }
  public:
    uint8 Keys[64 / 8];
  };

//! DES加密
/*!
  \param data   需要加密的数据指针
  \param size   需要加密的数据大小（需要64 bit对齐，不足部分则丢弃）
  \param key    参与加密数据的密钥
  \return       返回数据加密结果

  \code
    std::string data, key;
    std::cin >> data >> key;
    std::cout << showbin(DesEncrypt(data, key));
  \endcode
*/
std::string DesEncrypt(const void* data, const size_t size, const DesKey& key);

//! DES解密
/*!
  \param data   需要解密的数据指针
  \param size   需要解密的数据大小（需要128 bit对齐，不足部分则不处理）
  \param key    参与解密数据的密钥
  \return       返回数据解密结果

  \code
    std::string data, key;
    std::cin >> data >> key;
    std::cout << showbin(DesDecrypt(data, key));
  \endcode
*/
std::string DesDecrypt(const void* data, const size_t size, const DesKey& key);

//////////////////////////////////////////////////////////////////////////
template<typename T>
std::string DesEncrypt(const std::basic_string<T>& data, const DesKey& key)
  {
  return DesEncrypt(data.c_str(), data.size() * sizeof(T), key);
  }

template<typename T>
std::string DesDecrypt(const std::basic_string<T>& data, const DesKey& key)
  {
  return DesDecrypt(data.c_str(), data.size() * sizeof(T), key);
  }

#endif  // _XLIB_DES_H_
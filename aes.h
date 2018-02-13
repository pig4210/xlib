/*!
  \file  aes.h
  \brief aes.h定义了aes算法

  - 参考来自http://mingcn.cnblogs.com/archive/2010/10/31/aes_c.html
  - 源作者：xelz

  \version    3.0.1707.2418
  \note       For All

  \author     triones
  \date       2013-10-21
*/
#ifndef _XLIB_AES_H_
#define _XLIB_AES_H_

#include <string>
#include <string.h>

#include "xlib_base.h"

class AesKey
  {
  public:
    /*!
      指定key串。128 bit (16 byte)\n
      当size == 0时，视key为0结尾的ASCII字符串\n
      截断过多数据；以\x00补足不足数据
    */
    AesKey(const void* key, size_t size = 0);
    template<typename T> AesKey(const std::basic_string<T>& key)
      {
      memset(Keys, 0, sizeof(Keys));
      size_t size = key.size() * sizeof(T);
      if(size > sizeof(Keys)) size = sizeof(Keys);
      memcpy(Keys, key.c_str(), size);
      }
  public:
    uint8 Keys[128 / 8];
  };

//! AES加密
/*!
  \param data   需要加密的数据指针
  \param size   需要加密数据的长度（需要128 bit对齐，不成块部分则丢弃）
  \param key    参与加密数据的密钥
  \return       返回数据加密结果

  \code
    std::string data, key;
    std::cin >> data >> key;
    std::cout << showbin(AesEncrypt(data, key));
  \endcode
*/
std::string AesEncrypt(const void* data, const size_t size, const AesKey& key);

//! AES解密
/*!
  \param data   需要解密的数据指针
  \param size   需要解密数据的长度（需要128 bit对齐，不成块部分则不处理）
  \param key    参与解密数据的密钥
  \return       返回数据解密结果

  \code
    std::string data, key;
    std::cin >> data >> key;
    std::cout << showbin(AesDecrypt(data, key));
  \endcode
  */
std::string AesDecrypt(const void* data, const size_t size, const AesKey& key);

//////////////////////////////////////////////////////////////////////////
template<typename T>
std::string AesEncrypt(const std::basic_string<T>& data, const AesKey& key)
  {
  return AesEncrypt(data.c_str(), data.size() * sizeof(T), key);
  }

template<typename T>
std::string AesDecrypt(const std::basic_string<T>& data, const AesKey& key)
  {
  return AesDecrypt(data.c_str(), data.size() * sizeof(T), key);
  }

#endif  // _XLIB_AES_H_
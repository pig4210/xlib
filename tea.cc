#include "tea.h"

using std::string;

#include "xrand.h"
#include "swap.h"

TEA_DATA::TEA_DATA()
:A(0), B(0)
  {
  }

TEA_DATA::TEA_DATA(const TEA_WORD a, const TEA_WORD b)
:A(a), B(b)
  {
  }

TEA_DATA::TEA_DATA(const void* datas)
  {
  memcpy(Data, datas, sizeof(Data));
  }

TEA_KEY::TEA_KEY()
:K1(0), K2(0), K3(0), K4(0)
  {
  }

TEA_KEY::TEA_KEY(const TEA_WORD k1,
                 const TEA_WORD k2,
                 const TEA_WORD k3,
                 const TEA_WORD k4)
:K1(k1), K2(k2), K3(k3), K4(k4)
  {
  }

TEA_KEY::TEA_KEY(const void* key, size_t size)
  {
  memset(Key, 0, sizeof(Key));
  if(size == 0)
    {
    const uint8* k = (const uint8*)key;
    while(k[size] != '\0') ++size;
    }
  if(size > sizeof(Key))  size = sizeof(Key);
  memcpy(Key, key, size);
  }

//! 魔术常量 == 实数部分(2^32 * 黄金比例ψ)。ψ == sqrt(5/4) - 1/2 ≈ 0.618034;
static const TEA_WORD gk_tean_delta = 0x9E3779B9;
static const TEA_WORD gk_xtean_delta = 0x57E89147;

//! TEA加密轮数，16轮，一般32轮，推荐64轮
static const size_t gk_tean_round = 0x10;
static const size_t gk_xtean_round = 0x20;

//! 一个分组数据的大小，以byte计，共8 byte
static const size_t gk_tea_data_size = sizeof(TEA_DATA);

//! 算法用于大尾机器，小尾机器上需要翻转
static const uint16 gk_4210 = 0x4210;

//! 这个常数如果做成static const bool无法被编译器优化
#define gk_need_bswap ((*(const uint8*)&gk_4210) == 0x10)

TEA_DATA TeaEncipher(const TEA_DATA&    encrypt_data,
                     const TEA_KEY&     key,
                     const TEA_WORD     delta,
                     const size_t       xtea_round)
  {
  TEA_DATA data(
    (gk_need_bswap ? bswap(encrypt_data.A) : encrypt_data.A),
    (gk_need_bswap ? bswap(encrypt_data.B) : encrypt_data.B));

  TEA_WORD sum = delta;
  for(size_t i = 0; i < xtea_round; ++i)
    {
    data.A += ((data.B << 4) + key.K1) ^
      (data.B + sum) ^
      ((data.B >> 5) + key.K2);
    data.B += ((data.A << 4) + key.K3) ^
      (data.A + sum) ^
      ((data.A >> 5) + key.K4);
    sum += delta;
    }

  return TEA_DATA(
    (gk_need_bswap ? bswap(data.A) : data.A),
    (gk_need_bswap ? bswap(data.B) : data.B));
  }

TEA_DATA TeanEncipher(const TEA_DATA& encrypt_data, const TEA_KEY& key)
  {
  return TeaEncipher(encrypt_data, key, gk_tean_delta, gk_tean_round);
  }

static TEA_DATA TeaEncipherRound(TEA_DATA&          data,
                                 TEA_DATA&          pre_data,
                                 const TEA_KEY&     key,
                                 TEA_DATA&          cipher,
                                 const TEA_WORD     delta,
                                 const size_t       xtea_round)
  {
  data.A ^= cipher.A;
  data.B ^= cipher.B;     //当前数据与上组密文xor
  cipher = TeaEncipher(data, key, delta, xtea_round);
  cipher.A ^= pre_data.A;
  cipher.B ^= pre_data.B; //当前密文与上组数据xor
  pre_data = data;        //保存当前数据
  return cipher;
  }

string TeaEncrypt(const void*       encrypt_data,
                  const size_t      encrypt_size,
                  const TEA_KEY&    encrypt_key,
                  const TEA_WORD    delta,
                  const size_t      xtea_round)
  {
  string rets;
  if(encrypt_size == 0)  return rets;

  const TEA_KEY key(
    (gk_need_bswap ? bswap(encrypt_key.K1) : encrypt_key.K1),
    (gk_need_bswap ? bswap(encrypt_key.K2) : encrypt_key.K2),
    (gk_need_bswap ? bswap(encrypt_key.K3) : encrypt_key.K3),
    (gk_need_bswap ? bswap(encrypt_key.K4) : encrypt_key.K4));

  TEA_DATA tail(0, 0);        //数据尾，初始为全0

  const uint8* lp_encrypt = (const uint8*)encrypt_data;

  //数据最后一个字节与7个00成组
  tail.Data[0] = lp_encrypt[encrypt_size - 1];

  //补充填充数据个数，0~7
  const size_t padding_num = (gk_tea_data_size - ((encrypt_size + 1 + 2 + 7)
    % gk_tea_data_size)) % gk_tea_data_size;

  //加密轮数，最少2轮
  const size_t round = (encrypt_size + 0xA + padding_num) / 8;

  TEA_DATA header[2];        //填充数据头，1或2组
  //随机数填充数据头
  for(size_t i = 0; i < gk_tea_data_size * 2; ++i)
    {
#ifndef _XLIB_TEST_
    header[i / gk_tea_data_size].Data[i % gk_tea_data_size] = xrand() & 0xFF;
#else
    header[i / gk_tea_data_size].Data[i % gk_tea_data_size] = 0 & 0xFF;
#endif
    }

  //第一字节写入填充数据个数，3bit
  header[0].Data[0] &= 0xF8;
  header[0].Data[0] |= padding_num;

  //填充数据与正式数据成组，可能情况下会多读取一组正式数据或读取一组无效数据
  for(size_t i = 3 + padding_num; i < gk_tea_data_size * 2; ++i)
    {
    header[i / gk_tea_data_size].Data[i % gk_tea_data_size] = *lp_encrypt;
    ++lp_encrypt;
    }

  TEA_DATA data(0, 0);       //用于写入当前轮加密数据
  TEA_DATA pre_data(data);   //用于保存上轮加密数据
  TEA_DATA cipher(data);     //用于保存上轮密文，注意初始化为0保证数据头计算正确

  for(size_t i = 1; i <= round; ++i)
    {
    if(i == round)    //最后一轮加密时使用数据尾，注意round可能为2，所以判断必须在前
      {
      data = tail;
      }
    else
      {
      if(i <= 2)      //轮数大于2时，头两轮使用数据头，轮数大于1时，首轮使用数据头
        {
        data = header[i - 1];
        }
      else           //其余轮使用正式数据本身
        {
        data = TEA_DATA(lp_encrypt);
        lp_encrypt += gk_tea_data_size;
        }
      }
    const TEA_DATA TD(TeaEncipherRound(data, pre_data, key, cipher, delta, xtea_round));
    rets.append((const char*)TD.Data, sizeof(TD.Data));
    }

  return rets;
  }

string TeanEncrypt(const void* data, const size_t size, const TEA_KEY& key)
  {
  return TeaEncrypt(data, size, key, gk_tean_delta, gk_tean_round);
  }

TEA_DATA TeaDecipher(const TEA_DATA&    decrypt_data,
                     const TEA_KEY&     key,
                     const TEA_WORD     delta,
                     const size_t       xtea_round)
  {
  TEA_DATA data(
    (gk_need_bswap ? bswap(decrypt_data.A) : decrypt_data.A),
    (gk_need_bswap ? bswap(decrypt_data.B) : decrypt_data.B));

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:4307)   //“*”: 整型常量溢出
#endif
  TEA_WORD sum = delta * (TEA_WORD)xtea_round;
#ifdef _WIN32
#pragma warning(pop)
#endif

  for(size_t i = 0; i < xtea_round; ++i)
    {
    data.B -= ((data.A << 4) + key.K3) ^
      (data.A + sum) ^
      ((data.A >> 5) + key.K4);
    data.A -= ((data.B << 4) + key.K1) ^
      (data.B + sum) ^
      ((data.B >> 5) + key.K2);
    sum -= delta;
    }

  return TEA_DATA(
    (gk_need_bswap ? bswap(data.A) : data.A),
    (gk_need_bswap ? bswap(data.B) : data.B));
  }

TEA_DATA TeanDecipher(const TEA_DATA& decrypt_data, const TEA_KEY& key)
  {
  return TeaDecipher(decrypt_data, key, gk_tean_delta, gk_tean_round);
  }

//! 轮解密，返回解密结果
static TEA_DATA TeaDecipherRound(TEA_DATA&        cipher,
                                 TEA_DATA&        pre_cipher,
                                 const TEA_KEY&   key,
                                 TEA_DATA&        pre_data,
                                 const TEA_WORD   delta,
                                 const size_t     xtea_round)
  {
  pre_data.A ^= cipher.A;
  pre_data.B ^= cipher.B;       //当前密文与上轮解密数据异或
  pre_data = TeaDecipher(pre_data, key, delta, xtea_round);

  TEA_DATA data(
    pre_data.A ^ pre_cipher.A,
    pre_data.B ^ pre_cipher.B
    );  //明文由解密后数据与上轮密文异或得到
  //注意，pre_data保留解密数据，不是保留明文
  pre_cipher = cipher;          //保存当前密文用于下轮运算

  return data;
  }

string TeaDecrypt(const void*       decrypt_data,
                  const size_t      decrypt_size,
                  const TEA_KEY&    decrypt_key,
                  const TEA_WORD    delta,
                  const size_t      xtea_round)
  {
  string rets;
  if(decrypt_size == 0)  return rets;

  //解密串必须>=16并且为8倍数
  if(decrypt_size < (2 * gk_tea_data_size) || (decrypt_size % gk_tea_data_size))
    return rets;

  const TEA_KEY key(
    (gk_need_bswap ? bswap(decrypt_key.K1) : decrypt_key.K1),
    (gk_need_bswap ? bswap(decrypt_key.K2) : decrypt_key.K2),
    (gk_need_bswap ? bswap(decrypt_key.K3) : decrypt_key.K3),
    (gk_need_bswap ? bswap(decrypt_key.K4) : decrypt_key.K4));

  TEA_DATA pre_cipher(0, 0);  //用于保存上轮解密前数据
  TEA_DATA pre_data(0, 0);    //用于保存上轮解密后数据

  const uint8* lp_decrypt = (const uint8*)decrypt_data;

  for(size_t i = 0; i < (decrypt_size / gk_tea_data_size); ++i)
    {
    TEA_DATA cipher(&lp_decrypt[i * gk_tea_data_size]);
    const TEA_DATA TD(TeaDecipherRound(cipher, pre_cipher, key, pre_data, delta, xtea_round));
    rets.append((const char*)TD.Data, sizeof(TD.Data));
    }

  //需要验证结尾0，并去除结尾7 byte 0
  for(size_t i = 0; i < 7; ++i)
    {
    if(*(rets.rbegin()) != '\x00')
      {
      rets.clear();
      return rets;
      }
    rets.pop_back();
    }

  const size_t padding_num = (*rets.begin() & 7) + 3;
  //得到数据填充数据大小，当总填充数超过剩余数据大小时，出错
  if(padding_num >= rets.size())
    {
    rets.clear();
    return rets;
    }
  rets.erase(0, padding_num); //丢弃填充数据头

  //最后一次确认填充数
  const size_t chk_padding_num =
     (gk_tea_data_size - ((rets.size() + 1 + 2 + 7) % gk_tea_data_size)) %
     gk_tea_data_size + 3;
  if(padding_num != chk_padding_num)
    {
    rets.clear();
    return rets;
    }

  return rets;
  }

string TeanDecrypt(const void* data, const size_t size, const TEA_KEY& key)
  {
  return TeaDecrypt(data, size, key, gk_tean_delta, gk_tean_round);
  }


string XTeanEncrypt(const void* data, const size_t size, const TEA_KEY& key)
  {
  return TeaEncrypt(data, size, key, gk_xtean_delta, gk_xtean_round);
  }

string XTeanDecrypt(const void* data, const size_t size, const TEA_KEY& key)
  {
  return TeaDecrypt(data, size, key, gk_xtean_delta, gk_xtean_round);
  }


string XxTeaEncrypt(const void*     encrypt_data,
                    const size_t    encrypt_size,
                    const TEA_KEY&  encrypt_key)
  {
  string rets((const char*)encrypt_data, encrypt_size);
  TEA_WORD* data = (TEA_WORD*)rets.c_str();
  const size_t block_count = encrypt_size / sizeof(TEA_WORD);
  if(block_count <= 1)  return rets;
  intptr_t rounds = 6 + 52 / block_count;

  TEA_WORD sum = 0;

  TEA_WORD z = data[block_count - 1];
  TEA_WORD y;
  while(rounds-- > 0)
    {
    sum += gk_tean_delta;
    const TEA_WORD e = (sum >> 2) & 3;
    for(size_t i = 0; i < block_count - 1; ++i)
      {
      const TEA_WORD k = *(&(encrypt_key.K1) + ((i & 3) ^ e));
      y = data[i + 1];
      data[i] += (((z >> 5) ^ (y << 2)) + ((y >> 3) ^ (z << 4))) ^
        ((sum ^ y) + (k ^ z));
      z = data[i];
      }
    const TEA_WORD k = *(&(encrypt_key.K1) + (((block_count - 1) & 3) ^ e));
    y = data[0];
    data[block_count - 1] += (((z >> 5) ^ (y << 2)) + ((y >> 3) ^ (z << 4))) ^
      ((sum ^ y) + (k ^ z));
    z = data[block_count - 1];
    }
  return rets;
  }

string XxTeaDecrypt(const void*     decrypt_data,
                    const size_t    decrypt_size,
                    const TEA_KEY&  decrypt_key)
  {
  string rets((const char*)decrypt_data, decrypt_size);
  TEA_WORD* data = (TEA_WORD*)rets.c_str();
  const size_t block_count = decrypt_size / sizeof(TEA_WORD);
  if(block_count <= 1)  return rets;
  const size_t rounds = 6 + 52 / block_count;

  TEA_WORD sum = (TEA_WORD)(rounds * gk_tean_delta);

  TEA_WORD z;
  TEA_WORD y = data[0];
  while(sum != 0)
    {
    const TEA_WORD e = (sum >> 2) & 3;
    for(auto i = block_count - 1; i > 0; --i)
      {
      const TEA_WORD k = *(&(decrypt_key.K1) + ((i & 3) ^ e));
      z = data[i - 1];
      data[i] -= (((z >> 5) ^ (y << 2)) + ((y >> 3) ^ (z << 4))) ^
        ((sum ^ y) + (k ^ z));
      y = data[i];
      }
    const TEA_WORD k = *(&(decrypt_key.K1) + e);
    z = data[block_count - 1];
    data[0] -= (((z >> 5) ^ (y << 2)) + ((y >> 3) ^ (z << 4))) ^
      ((sum ^ y) + (k ^ z));
    y = data[0];
    sum -= gk_tean_delta;
    }
  return rets;
  }

#ifdef _XLIB_TEST_

ADD_XLIB_TEST(TEA)
  {
  SHOW_TEST_INIT;

  auto done = false;

  const string data("1234");
  const TEA_KEY key("0123456789ABCDEF");
  const TEA_DATA dd(0x12345678, 0x12345678);

  TEA_DATA dr;
  string rets;

  SHOW_TEST_HEAD("TeaEncipher");
  dr = TeanEncipher(dd, key);
  done = (dr.A == 0xA10CD3F2) && (dr.B == 0x59572BB1);
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("TeaDecipher");
  dr = TeanDecipher(dr, key);
  done = (dr.A == dd.A) && (dr.B == dd.B);
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("TeanEncrypt");
  rets = TeanEncrypt(data, key);
  done = (rets == string("\x17\x9C\x88\xFA\xF1\x6F\xAA\xE5\xE0\xB0\x45\x58\x3D\x3C\xF3\x56"));
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("TeanDecrypt");
  rets = TeanDecrypt(rets, key);
  done = (rets == data);
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("XTeanEncrypt");
  rets = XTeanEncrypt(data, key);
  done = (rets == string("\x7E\x0C\xC1\xC8\xF9\xD2\xA7\x4F\x4D\x97\x7A\xC5\xBE\x8A\xE3\x1F"));
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("XTeanDecrypt");
  rets = XTeanDecrypt(rets, key);
  done = (rets == data);
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("XxTeaEncrypt");
  rets = XxTeaEncrypt(string("0123456789ABCDEF"), key);
  done = (rets == string("\x7E\x3B\x41\xAE\xFF\x81\x12\x1C\x3F\x1B\x67\x13\x37\xE3\x2D\xDD"));
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("XxTeaDecrypt");
  rets = XxTeaDecrypt(rets, key);
  done = (rets == string("0123456789ABCDEF"));
  SHOW_TEST_RESULT(done);
  }

#endif  // _XLIB_TEST_
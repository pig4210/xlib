#include "md5.h"

#include "swap.h"

#ifdef _WIN32
#   define ROTATE(a, n)    _lrotl(a, n)
#else
#   define ROTATE(a, n)    (((a)<<(n))|(((a)&0xffffffff)>>(32-(n))))
#endif

MD5_VALUE::MD5_VALUE(const MD5_WORD a,
                     const MD5_WORD b,
                     const MD5_WORD c,
                     const MD5_WORD d)
  :A(a), B(b), C(c), D(d)
  {

  }

//! 主循环有四轮，每轮16次
static const size_t gk_step = 16 + 16 + 16 + 16;

//! 每次计算使用的常数ti，ti = floor(abs(sin(1...64) * 2^32);
static const MD5_WORD gk_ti_tb[gk_step] =
  {
  0xD76AA478, 0xE8C7B756, 0x242070DB, 0xC1BDCEEE,
  0xF57C0FAF, 0x4787C62A, 0xA8304613, 0xFD469501,
  0x698098D8, 0x8B44F7AF, 0xFFFF5BB1, 0x895CD7BE,
  0x6B901122, 0xFD987193, 0xA679438E, 0x49B40821,

  0xF61E2562, 0xC040B340, 0x265E5A51, 0xE9B6C7AA,
  0xD62F105D, 0x02441453, 0xD8A1E681, 0xE7D3FBC8,
  0x21E1CDE6, 0xC33707D6, 0xF4D50D87, 0x455A14ED,
  0xA9E3E905, 0xFCEFA3F8, 0x676F02D9, 0x8D2A4C8A,

  0xFFFA3942, 0x8771F681, 0x6D9D6122, 0xFDE5380C,
  0xA4BEEA44, 0x4BDECFA9, 0xF6BB4B60, 0xBEBFBC70,
  0x289B7EC6, 0xEAA127FA, 0xD4EF3085, 0x04881D05,
  0xD9D4D039, 0xE6DB99E5, 0x1FA27CF8, 0xC4AC5665,

  0xF4292244, 0x432AFF97, 0xAB9423A7, 0xFC93A039,
  0x655B59C3, 0x8F0CCC92, 0xFFEFF47D, 0x85845DD1,
  0x6FA87E4F, 0xFE2CE6E0, 0xA3014314, 0x4E0811A1,
  0xF7537E82, 0xBD3AF235, 0x2AD7D2BB, 0xEB86D391
  };

//! 每次计算使用16个消息即 16 * 4 * 8 == 512 bit
static const size_t gk_round = 512 / 8 / sizeof(MD5_WORD);

//! 每次计算使用消息索引0~15
static const MD5_WORD gk_data_index[gk_step] =
  {
  0,   1,   2,   3,   4,   5,   6,   7,
  8,   9,  10,  11,  12,  13,  14,  15,

  1,   6,  11,   0,   5,  10,  15,   4,
  9,  14,   3,   8,  13,   2,   7,  12,

  5,   8,  11,  14,   1,   4,   7,  10,
 13,   0,   3,   6,   9,  12,  15,   2,

  0,   7,  14,   5,  12,   3,  10,   1,
  8,  15,   6,  13,   4,  11,   2,   9
  };

//! 每次计算使用左环移数
static const uint32 gk_lrotl_num[gk_step] =
  {
  7,  12,  17,  22,  7,  12,  17,  22,
  7,  12,  17,  22,  7,  12,  17,  22,

  5,   9,  14,  20,  5,   9,  14,  20,
  5,   9,  14,  20,  5,   9,  14,  20,

  4,  11,  16,  23,  4,  11,  16,  23,
  4,  11,  16,  23,  4,  11,  16,  23,

  6,  10,  15,  21,  6,  10,  15,  21,
  6,  10,  15,  21,  6,  10,  15,  21
  };

//! 用于循环
static void Cyclic_MD5_VALUE(MD5_VALUE& m)
  {
  const MD5_WORD t = m.D;
  m.D = m.C;  m.C = m.B;  m.B = m.A;  m.A = t;
  }

//! 级联
static void Cascade_MD5_VALUE(MD5_VALUE& ma, const MD5_VALUE& mb)
  {
  ma.A += mb.A;   ma.B += mb.B;
  ma.C += mb.C;   ma.D += mb.D;
  }

//! md5组消息循环算法，64轮计算
static void MD5_Algorithm(MD5_VALUE& mcv, const MD5_WORD data[gk_round])
  {
  MD5_VALUE mm(mcv);
  // FF(a, b, c, d, Mj, s, ti）表示 a = b + ((a + F(b, c, d) + Mj + ti) << s)
  for(size_t i = 0; i < gk_step; ++i)
    {
    switch(i)
      {
      case 0:   case 1:   case 2:   case 3:   case 4:   case 5:   case 6:   case 7:
      case 8:   case 9:   case 10:  case 11:  case 12:  case 13:  case 14:  case 15:
        // 非线性函数F(b, c, d) == (x & y) | ((~x) & z)
        mm.A += ((mm.B & mm.C) | ((~mm.B) & mm.D));
        break;
      case 16:  case 17:  case 18:  case 19:  case 20:  case 21:  case 22:  case 23:
      case 24:  case 25:  case 26:  case 27:  case 28:  case 29:  case 30:  case 31:
        // 非线性函数G(b, c, d) == (x & z) | (y & (~z))
        mm.A += ((mm.B & mm.D) | (mm.C & (~mm.D)));
        break;
      case 32:  case 33:  case 34:  case 35:  case 36:  case 37:  case 38:  case 39:
      case 40:  case 41:  case 42:  case 43:  case 44:  case 45:  case 46:  case 47:
        // 非线性函数H(b, c, d) == x ^ y ^ z
        mm.A += (mm.B ^ mm.C ^ mm.D);
        break;
      //case 48:  case 49:  case 50:  case 51:  case 52:  case 53:  case 54:  case 55:
      //case 56:  case 57:  case 58:  case 59:  case 60:  case 61:  case 62:  case 63:
      default:
        // 非线性函数H(b, c, d) == y ^ (x | (~z))
        mm.A += (mm.C ^ (mm.B | (~mm.D)));
      }
    mm.A += data[gk_data_index[i]];               // + Mj
    mm.A += gk_ti_tb[i];                          // + ti
    mm.A = mm.B + ROTATE(mm.A, gk_lrotl_num[i]);  // << s
    Cyclic_MD5_VALUE(mm);
    }
  Cascade_MD5_VALUE(mcv, mm);
  }

//! 用于判定数据是否小尾，在大尾机器上，运算数据需要翻转
static const uint16 gk_4210 = 0x4210;

//! 这个常数如果做成static const bool无法被编译器优化
#define gk_need_bswap ((*(const uint8*)&gk_4210) != 0x10)

MD5_VALUE md5(const void* data, const size_t size)
  {
  MD5_VALUE mcv(0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476);

  MD5_WORD buf[gk_round];                         //临时数据缓冲
  const size_t ground_size = sizeof(buf);         //一组计算所需数据
  const MD5_WORD* lp = (const MD5_WORD*)data;

  //计算前面不用填充的数据，共size / ground_size组
  for(size_t k = size / ground_size; k > 0; --k)
    {
    for(size_t i = 0; i < gk_round; ++i)
      {
      buf[i] = gk_need_bswap ? bswap(*lp) : *lp;
      ++lp;
      }
    MD5_Algorithm(mcv, buf);
    }

  const size_t remain = size % ground_size;       //分组后剩余数据
  //补充未能成组的数据
  for(size_t i = 0; i < (remain / sizeof(MD5_WORD)); ++i)
    {
    buf[i] = gk_need_bswap ? bswap(*lp) : *lp;
    ++lp;
    }

  //最后数据填充，可能有，也可能没有
  MD5_WORD& adds = buf[remain / sizeof(MD5_WORD)];
  adds = *lp;
  const MD5_WORD aff = (MD5_WORD)-1;
  //清除多余数据
  if(remain % sizeof(MD5_WORD) == 0)
    {
    adds &= 0;    //注意，当remain刚好为4倍数时，shr xx,20 == shr xx,0
    }
  else
    {
    adds &= (aff >> ((sizeof(MD5_WORD) - (remain % sizeof(MD5_WORD))) * 8));
    }

  //附加1和几个0
  const MD5_WORD a80 = 0x80;
  adds |= (a80 << ((remain % sizeof(MD5_WORD)) * 8));

  //补上长度
  const uint64 count = size * 8;                  //数据位数(注意64)
  const size_t needleft = ground_size - sizeof(count); //补齐长度448bit==56byte
  if(remain >= needleft)   //如果数据填充后过长，下次计算
    {
    for(size_t i = (remain / sizeof(MD5_WORD)) + 1; i < gk_round; ++i)
      {
      buf[i] = 0;
      }
    MD5_Algorithm(mcv, buf);
    //填充448 bit 0
    for(size_t i = 0; i < (needleft / sizeof(MD5_WORD)); ++i)
      {
      buf[i] = 0;
      }
    }
  else
    {
    //继续填充0至448bit
    for(size_t i = (remain / sizeof(MD5_WORD)) + 1;
      i < needleft / sizeof(MD5_WORD);  ++i)
      {
      buf[i] = 0;
      }
    }

  //附加长度数据
  uint64* lpcount = (uint64*)&buf[needleft / sizeof(MD5_WORD)];
  *lpcount = gk_need_bswap ? bswap(count) : count;
  MD5_Algorithm(mcv, buf);

  return mcv;
  }

#ifdef _XLIB_TEST_

using std::string;

ADD_XLIB_TEST(MD5)
  {
  SHOW_TEST_INIT;

  auto done = false;

  SHOW_TEST_HEAD("md5");
  const string rets = md5(string("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"));
  done = (rets == "\xF2\x99\x39\xA2\x5E\xFA\xBA\xEF\x3B\x87\xE2\xCB\xFE\x64\x13\x15");
  SHOW_TEST_RESULT(done);
  }

#endif  // _XLIB_TEST_
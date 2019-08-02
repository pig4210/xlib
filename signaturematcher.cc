#ifdef _WIN32

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:4512)     // SIG_STL 无法生成赋值运算符。
#pragma warning(disable:4100)
#pragma warning(disable:4311)     // “类型强制转换”: 从“void *”到“uint32”的指针截断。
#pragma warning(disable:4302)     // “类型强制转换”: 从“void *”到“uint”截断。
#endif

#include "signaturematcher.h"

#include "hex_bin.h"
#include "pe.h"
#include "syssnap.h"
#include "swap.h"
#include <bitset>

using std::bitset;
using std::string; 
using std::vector;

#ifndef _WIN32
#include <iostream>
using std::clog;
using std::endl;
#endif


/////////////////////////// 调试信息输出设置 ///////////////////////////////
namespace signaturematcher
  {
  class smlog : public xmsg
    {
    public:
      virtual ~smlog()
        {
        if(empty())  return;
        if(func != nullptr)
          {
          append("\r\n");
          func(c_str());
          clear();
          return;
          }
#ifdef _WIN32
#ifndef FOR_RING0
        OutputDebugStringA(c_str());
#else
        DbgPrint("%s\n", c_str());
#endif
#else
        clog << c_str() << endl;
#endif 
        clear();
        }
    public:
      static log_out_func func;
    };

  log_out_func smlog::func = nullptr;

#ifdef SIGNATUREMATCHER_DEBUG
  #define xsdbg  smlog()
#else
  #define xsdbg  if(false) smlog()
#endif
  #define xserr  smlog()
  }

//////////////////////////// 特征码字符串类 ////////////////////////////////
namespace signaturematcher
  {
  /// 特征码字符串类，用于操纵字符串的遍历。
  class Sign
    {
    public:
      Sign(SIGNATURE sig)
        :sign(sig), scaner(0)
        {
        ;
        }
      /// 返回当前处理的字符。
      char ch() const
        {
        return sign[scaner];
        }
      /// 向后移动一个字符。
      void step()
        {
        ++scaner;
        }
    private:
      friend static xmsg& operator<<(xmsg& msg, const Sign& s);
    private:
      SIGNATURE   sign;             ///< 存放特征码字符串指针。
      intptr_t    scaner;           ///< 指示当前解析字符起始索引。
    };

#ifdef _WIN32
#pragma warning(disable:4239)
#endif
  /// 重载输出操作以输出当前处理信息
  xmsg& operator<<(xmsg& msg, const Sign& s)
    {
    intptr_t row = 1;
    intptr_t col = 1;
    const intptr_t lp = s.scaner;
    for(intptr_t i = 0; i < lp; ++i)
      {
      switch(s.sign[i])
        {
        // 注意只识别 \n 以兼容 Unix。
        case '\n':
          col = 1;  ++row;
          break;
        case '\0':
          return msg << "第" << row << "行" << col << "列[越界错误]";
        default:
          ++col;
        }
      }
    return msg << "第" << row << "行" << col << "列:";
    }
  }

/////////////////////////////// 词法类型 ///////////////////////////////////
namespace signaturematcher
  {
  enum LexicalType : uint8
    {
    LT_Error,       ///< 错误词法。
    LT_End,         ///< end          -> \0
                    ///< ws           -> [ \t\n\r]*           // 空白词法不生成类型。
                    ///< note         -> #[^\n\0]*(\n|\0)     // 注释词法不生成类型。
                    ///< 以下词法不单独成词。
                    ///< hex          -> [0-9A-Fa-f]
                    ///< range_value  -> {hex}({ws}{hex}){1,7}// x64 下为 {hex}({ws}{hex}){1,15} 。
                    ///< range        -> {ws}([\*\+\?])|(\{{ws}{range_value}?{ws},?{ws}{range_value}?{ws}\})
                    ///< hexhex       -> ({hex}{ws}{hex}) | (\:[^\0])
    LT_String,      ///< string       -> [L|l]?{ws}\'[^\0]+\'
    LT_Quote,       ///< quote        -> [L|l]?{ws}\"[^\0]+\"
    LT_Dot,         ///< dot          -> \.{range}?
    LT_MarkRef,     ///< markreg      -> {hexhex}({ws}@{ws}L)?({ws}@{ws}R)?
                    ///< refreg       -> {hexhex}({ws}${ws}[1-7]{ws}L)?({ws}${ws}[1-7]{ws}R)?
    LT_Ucbit,       ///< ucbit        -> {hexhex}({ws}[\-\|\&]{ws}{hexhex})*{range}?
    LT_ConstHex,    ///< consthex     -> {hexhex}{range}?
                    ///< collection   -> \[\^?{hexhex}({ws}[\&\|\&]?{ws}{hexhex})*{ws}\]{range}?
    LT_Record_Addr,
    LT_Record_Offset,
    LT_Record_Qword,
    LT_Record_Dword,
    LT_Record_Word,
    LT_Record_Byte, ///< record       -> \<{ws}\^?{ws}[AFQDWB]{ws}[^ \t\n\r\0\>]*{ws}[^ \t\n\r\0\>]*{ws}\>
    };
  }

/////////////////////////// range 标识结构 //////////////////////////////////
namespace signaturematcher
  {
  typedef intptr_t      RangeType;       ///< 范围类型。

  /// 范围指示结构。
  struct Range
    {
    RangeType  Min;     ///< 最小匹配次数。
    RangeType  Max;     ///< 最大匹配次数。
    /// 简单初始化。
    Range(const RangeType& min, const RangeType& max)
      :Min(min), Max(max)
      {
      ;
      }
    /// 指定固定范围。
    Range(const RangeType& min)
      :Min(min), Max(min)
      {
      ;
      }
    };

  /// 用以标识最大范围指示值。
  static const RangeType gk_max_rangetype =
#ifdef _WIN64
    0x7FFFFFFFFFFFFFFF;
#else
    0x7FFFFFFF;
#endif

  /// 用以标识错误范围指示值。
  static const RangeType gk_error_rangetype = -1;

  /// 用以标识初始范围指示值。
  static const RangeType gk_init_rangetype = -1;

  /// 用以标识错误的范围。
  static const Range  gk_err_range = { gk_error_rangetype, gk_error_rangetype };

  /// 用以标识缺省的范围。
  static const Range  gk_min_range = { 1, 1 };

  /// 用以标识空的范围。
  static const Range gk_none_range = { 0, 0 };

  /// 比较范围是否相同。
  static bool operator==(const Range& ra, const Range& rb)
    {
    return (ra.Min == rb.Min) && (ra.Max == rb.Max);
    }

  /// 范围融合。
  static Range& operator+=(Range& ra, const Range& rb)
    {
    const auto Min = ra.Min + rb.Min;
    ra.Min = (Min < ra.Min) ? gk_max_rangetype : Min;

    const auto Max = ra.Max + rb.Max;
    ra.Max = (Max < ra.Max) ? gk_max_rangetype : Max;

    return ra;
    }
  }

/////////////////////////////// 词法基类 ///////////////////////////////////
namespace signaturematcher
  {
  class LexicalBase
    {
    public:
      LexicalBase(LexicalType t = LT_Error, const Range& r = gk_min_range)
        :type(t), range(r), match_count(gk_init_rangetype)
        {
        ;
        }
      /// 空虚析构，必要存在，以使继承类能正确释放资源。
      virtual ~LexicalBase()
        {
        ;
        }
      virtual const char* type_name() const
        {
        return "词法end/未命名/错误类型";
        }
      /// 用以指示是否有效词法。
      bool valid() const
        {
        return type != LT_Error;
        }
      /// 用以整合同类型，返回表示融合失败，融合成功后注意释放资源。默认不融合。
      virtual bool fusion(LexicalBase*)
        {
        return false;
        }
      /// 用以指示是否特征点，默认不是。
      virtual bool record() const
        {
        return false;
        }
      /// 指定内存范围和索引，进行匹配，匹配失败返回 false ，失败且 lp > blk.size() 时，彻底失败。
      bool match(const xblk& blk, RangeType& lp)
        {
        void* pp = (uint8*)blk.start() + lp;
        // 如果匹配达到最大，指针回退，允许继续。
        if(match_count >= range.Max)
          {
          xsdbg << pp << ' ' << type_name()
            << " 匹配范围达到最大值 " << (void*)range.Max << " ，回退。";
          lp -= match_count;
          match_count = gk_init_rangetype;
          return false;
          }

        if(match_count < range.Min)
          {
          match_count = gk_init_rangetype;
          // 如果内存范围已经不足以进行最低匹配，则彻底失败。
          if((RangeType)blk.size() < (lp + range.Min))
            {
            xsdbg << pp << ' ' << type_name()
              << " 无法实现最低 " << (void*)range.Min << " 次匹配，失败！";
            lp = gk_max_rangetype;
            return false;
            }
          xsdbg << pp << ' ' << type_name()
            << " 正在进行最低 " << (void*)range.Min << " 次匹配。";
          // 最低匹配失败，允许回退继续。
          if(!test(xblk(pp, range.Min)))
            {
            xsdbg << pp << ' ' << type_name() << " 最低匹配失败，回退。";
            return false;
            }
          match_mem = pp;
          match_count = range.Min;
          lp += match_count;
          return true;
          }

        // 如果内存范围已经不足以进行递进匹配，则彻底失败。
        if((RangeType)blk.size() < (lp + 1))
          {
          xsdbg << pp << ' ' << type_name() << " 已经无法递进匹配了，失败！";
          lp = gk_max_rangetype;
          return false;
          }
        xsdbg << pp << ' ' << type_name() << " 正在进行递进匹配：" << match_count + 1;
        // 递进匹配失败，允许回退继续。
        if(!test(xblk(pp, 1)))
          {
          xsdbg << pp << ' ' << type_name() << " 递进匹配失败，回退。";
          lp -= match_count;
          match_count = gk_init_rangetype;
          return false;
          }
        ++lp;
        ++match_count;
        return true; 
        }
      /// 设计防止内存不可读产生异常。
      virtual bool readable(const xblk& blk) const
        {
#ifdef FOR_RING0
        // ring0 下直接读取某非法地址可能不抛出异常，故而只能采用此法。
        const size_t base_page = 0x1000;
        if(blk.size() == 0) return false;
        size_t s = (size_t)blk.start() / base_page * base_page;
        size_t e = ((size_t)blk.end() - 1) / base_page * base_page;
        do 
          {
          if(!MmIsAddressValid((PVOID)s)) return false;
          s += base_page;
          }while(s <= e);
        return true;
#else
        return FALSE == IsBadReadPtr(blk.start(), blk.size());
#endif
        }
      /// 用以给定内存段细节匹配，默认匹配。需要细节匹配则重载之。
      virtual bool test(const xblk& blk) const
        {
        return readable(blk);
        }
      /// 输出原子细节。
      virtual void to_atom(vline&) const
        {
        return;
        }
      /// 用以输出原子。
      void create_atom(vline& atom) const
        {
        atom << type << range.Min << range.Max;
        to_atom(atom);
        }
    public:
      LexicalType   type;             ///< 指示词法类型。
      Range         range;            ///< 指示词法匹配内存大小。
      RangeType     match_count;      ///< 指示词法在匹配过程中匹配的大小。
      void*         match_mem;        ///< 用以记录匹配位置。
    };
  }

//////////////////// 词法 end 、 ws 、 note 、 hex 识别函数 ////////////////////////
namespace signaturematcher
  {
  /// 识别词法 end ，成功返回 LT_End ，否则返回 nullptr 。
  static LexicalBase* match_end(Sign& sig)
    {
    if(sig.ch() != '\0')  return nullptr;
    sig.step();
    xsdbg << "识别到词法 end 。";
    return new LexicalBase(LT_End);
    }
  /// 识别词法 ws ，只是跳过 ws ，无论成功与否都返回 nullptr。
  static LexicalBase* match_ws(Sign& sig)
    {
    while(true)
      {
      switch(sig.ch())
        {
        case ' ':     case '\t':   case '\n':    case '\r':
          sig.step(); continue;
        default:;
        }
      break;
      };
    return nullptr;
    }
  /// 识别词法 note ，只是跳过 note ，无论成功与否都返回 nullptr 。
  static LexicalBase* match_note(Sign& sig)
    {
    // 建立循环是为了处理连续注释的情况。
    while(sig.ch() != '\0')
      {
      if(sig.ch() != '#') return nullptr;
      sig.step();
      while(sig.ch() != '\n' && sig.ch() != '\0')
        sig.step();
      xsdbg << " 跳过词法 note 。";
      match_ws(sig);
      }
    return nullptr;
    }
  /// 匹配 hex 词法，返回值 < 0 表示非此词法。
  static char match_hex(Sign& sig)
    {
    switch(sig.ch())
      {
      case '0':     case '1':    case '2':     case '3':
      case '4':     case '5':    case '6':     case '7':
      case '8':     case '9':
      case 'A':     case 'B':    case 'C':     case 'D':
      case 'E':     case 'F':
      case 'a':     case 'b':    case 'c':     case 'd':
      case 'e':     case 'f':
        {
        char hex = sig.ch() & 0x0F;
        if(sig.ch() > '9')  hex += 0x09;
        sig.step();
        return hex;
        }
      default:;
      }
    return -1;
    }
  }

/////////////////////////// 词法 range 提取函数 //////////////////////////////
namespace signaturematcher
  {
  /// 匹配词法 range_value ，返回值 < 0 表示非此词法。
  static RangeType match_range_value(Sign& sig)
    {
    auto hex = match_hex(sig);
    if(hex < 0) return gk_error_rangetype;
    RangeType r = hex;
    
    for(size_t i = 0; i < (2 * sizeof(RangeType) - 1); ++i)
      {
      match_ws(sig);
      hex = match_hex(sig);
      if(hex < 0) return r;
      r = (r << 4) | hex;
      }

    return r;
    }
  /// 匹配词法 range，返回值 == gk_err_range 时，匹配错误。
  static Range match_range(Sign& sig)
    {
    match_ws(sig);

    switch(sig.ch())  // 先识别单一字符的范围指示。
      {
      case '*':
        xsdbg << "  识别到范围 * 。";
        sig.step();
        return Range(0, gk_max_rangetype);
      case '+':
        xsdbg << "  识别到范围 + 。";
        sig.step();
        return Range(1, gk_max_rangetype);
      case '?':
        xsdbg << "  识别到范围 ? 。";
        sig.step();
        return Range(0, 1);
      case '{':
        sig.step();
        break;
      default:
        xsdbg << "  无范围指示，返回默认范围 {1,1} 。";
        return gk_min_range;
      }
    match_ws(sig);

    auto Min = match_range_value(sig);
    decltype(Min) Max = 0;

    match_ws(sig);
    bool need_max = true;
    if(Min >= 0)                    // 存在 N 值。
      {
      xsdbg << "  识别到范围 N 值：" << (void*)Min;
      if(sig.ch() != ',')   // 存在 N 值，但没有分隔符 {N} 。
        {
        xsdbg << "    范围分隔符不存在，不再提取 M 值。";
        Max = Min;
        need_max = false;               // 没有分隔符的情况下，不能继续提取 M 值。
        }
      else
        {
        sig.step();                       // 存在 N 值且有分隔符的情况下，可能是 {N, } | {N, M} 。
        }
      }
    else                                //不存在N值
      {
      xsdbg << "  没有识别到范围 N 值。";
      if(sig.ch() != ',')   // 不存在 N 值且无分隔符，非法 {} 。
        {
        xserr << sig << "    范围指示识别失败！";
        return gk_err_range;
        }
      sig.step();
      xsdbg << "     存在分隔符，N = 0 。";
      Min = 0;                          // 不存在 N 值但有分隔符，可能是 {, } | {, M} 。
      }

    if(need_max)
      {
      match_ws(sig);
      Max = match_range_value(sig);
      if(Max >= 0)                      // 存在 M 值 {N, M} | {, M} 。
        {
        xsdbg << "  识别到范围 M 值：" << (void*)Max;
        }
      else                              // 不存在 M 值 {N, } | { , } 。
        {
        xsdbg << "  没有识别到范围 M 值， M 为最大值。";
        Max = gk_max_rangetype;
        }
      }

    match_ws(sig);
    if(sig.ch() != '}')
      {
      xserr << sig << " 范围指示 缺少 '}' 结束/存在非法字符/超越最大允许位数！";
      return gk_err_range;
      }
    sig.step();

    if(Min == 0 && Max == 0)
      {
      xserr << sig << " 非法的空范围，请检查范围指示！";
      return gk_err_range;
      }

    seqswap(Min, Max);
    xsdbg << "  确定范围：{ " << (void*)Min << ", " << (void*)Max << " } 。";
    return Range(Min, Max);
    }
  }

///////////////////// 词法 string 、 quote 类及识别函数 ////////////////////////
namespace signaturematcher
  {
  class LexicalString : public LexicalBase
    {
    public:
      LexicalString(const string& s)
        :LexicalBase(LT_String, Range(s.size())), str(s)
        {
        ;
        }
      virtual const char* type_name() const
        {
        return "词法string";
        }
      virtual bool fusion(LexicalBase* lb)
        {
        if(lb->type != LT_String) return false;
        str.append(((LexicalString*)lb)->str);
        range = Range(str.size());    // 注意不是 += 。
        return true;
        }
      virtual bool test(const xblk& blk) const
        {
        if(!readable(blk)) return false;
        xsdbg << "    匹配 string ：\r\n"
          << showbin(str)
          << showbin(blk.start(), blk.size(), HC_ASCII, true, 6);
        if(blk.size() != str.size())  return false;
        return memcmp(str.c_str(), blk.start(), str.size()) == 0;
        }
      virtual void to_atom(vline& nline) const
        {
        nline << str.size() << str;
        }
    public:
      string str;
    };
  class LexicalQuote : public LexicalBase
    {
    public:
      LexicalQuote(const string& s)
        :LexicalBase(LT_Quote, Range(sizeof(void*))), str(s)
        {
        ;
        }
      virtual const char* type_name() const
        {
        return "词法quote";
        }
      // 继承 funcsion、test 。其实就是不融合，无需 test 。
      virtual void to_atom(vline& nline) const
        {
        nline << str.size() << str;
        }
    public:
      string str;
    };
  /// 识别词法 string 、 quote ，成功返回 LT_String 或 LT_Quote ，非词法返回 nullptr ，失败返回 LT_Error 。
  static LexicalBase* match_string_quote(Sign& sig)
    {
    bool unicode = false;
    if(sig.ch() == 'L' || sig.ch() == 'l')
      {
      sig.step();
      unicode = true;
      match_ws(sig);
      xsdbg << "识别到 Unicode 标识。";
      }

    const char ch = sig.ch();
    if(ch != '\"' && ch != '\'')
      {
      if(!unicode)  return nullptr;
      xserr << sig << " 不允许单独的 Unicode 标识 'L' 。";
      return new LexicalBase(LT_Error);
      }
    sig.step();

    string str;
    while(sig.ch() != '\0')
      {
      if(sig.ch() != ch)
        {
        // 简单处理 \' 或 \" 的情况。
        if(sig.ch() == '\\')
          {
          sig.step();
          if(sig.ch() == ch)
            {
            str.append(1, ch);
            sig.step();
            }
          else
            {
            str.append(1, '\\');
            }
          continue;
          }

        str.append(1, sig.ch());
        sig.step();
        continue;
        }

      if(str.empty())
        {
        xserr << sig << " 字符串/引用串 不可以为空。";
        return new LexicalBase(LT_Error);
        }
      sig.step();

      xsdbg << "字符串提取完成：" << str;
      const string s(escape(str));

      xsdbg << "字符串转换完成：\r\n" << showbin(s, (size_t)2);

      if(unicode)
        {
        const auto ws(s2ws(s));
        str.assign((const char*)ws.c_str(), ws.size() * sizeof(charucs2_t));
        xsdbg << "字符串转换为 Unicode ：\r\n"
          << showbin(str.c_str(), str.size(), HC_UNICODE, true, 2);
        }
      else
        {
        str = s;
        }

      if(ch == '\"')  return new LexicalQuote(str);
      return new LexicalString(str);
      }

    xserr << sig << " 缺少 " << ch << " 来界定结束。";
    return new LexicalBase(LT_Error);
    }
  }

//////////////////////// 词法 dot 类及识别函数 ///////////////////////////////
namespace signaturematcher
  {
  class LexicalDot : public LexicalBase
    {
    public:
      LexicalDot(const Range& range)
        :LexicalBase(LT_Dot, range)
        {
        ;
        }
      virtual const char* type_name() const
        {
        return "词法dot";
        }
      virtual bool fusion(LexicalBase* lb)
        {
        if(lb->type != LT_Dot)  return false;
        range += ((LexicalDot*)lb)->range;
        return true;
        }
    };
  /// 识别词法 dot ，成功返回 LT_Dot ，非词法返回 nullptr ，失败返回 LT_Error 。
  static LexicalBase* match_dot(Sign& sig)
    {
    if(sig.ch() != '.') return nullptr;
    sig.step();
    xsdbg << "识别到词法 dot 。";
    Range range = match_range(sig);
    if(range == gk_err_range) return new LexicalBase(LT_Error);
    return new LexicalDot(range);
    }
  }

////////////////////////////// ucbit 类 /////////////////////////////////////
namespace signaturematcher
  {
  class ucbit : public bitset<0x100>
    {
    public:
      /// 添加模糊匹配组。
      ucbit& set_obfus(const uint8 TA, const uint8 TB)
        {
        for(size_t i = 0; i < size(); ++i)
          {
          if(!((TA ^ i) & (TB ^ i)))
            set(i);
          }
        return *this;
        }
      /// 添加连续组。
      ucbit& set_queue(uint8 TA, uint8 TB)
        {
        seqswap(TA, TB);
        for(size_t i = TA; i <= TB; ++i)
          set(i);
        return *this;
        }
    };
  }

///////////////// 词法 ucbit 、 consthex 、 markreg 、 refreg 类 ///////////////////
namespace signaturematcher
  {
  class LexicalUcbit : public LexicalBase
    {
    public:
      LexicalUcbit(const ucbit& u, const Range& range = gk_min_range)
        :LexicalBase(LT_Ucbit, range), uc(u)
        {
        ;
        }
      virtual const char* type_name() const
        {
        return "词法ucbit";
        }
      virtual bool fusion(LexicalBase* lb)
        {
        if(lb->type != LT_Ucbit)  return false;
        if(uc != ((LexicalUcbit*)lb)->uc) return false;
        range += ((LexicalUcbit*)lb)->range;
        return true;
        }
      virtual bool test(const xblk& blk) const
        {
        if(!readable(blk)) return false;
        const uint8* lp = (const uint8*)blk.start();
        xsdbg << "    匹配 ucbit ：\r\n"
          << showbin(blk.start(), blk.size(), HC_ASCII, true, 6);
        xsdbg << showbin((void*)&uc, sizeof(uc), HC_ASCII, true, 6);
        for(size_t i = 0; i < blk.size(); ++i)
          {
          if(!uc.test(lp[i])) return false;
          }
        return true;
        }
      virtual void to_atom(vline& nline) const
        {
        const int32* lpis = (const int32*)&uc;
        const int32* lpie = lpis + (sizeof(uc) / sizeof(int32));
        for(; lpis != lpie; ++lpis)
          {
          nline << *lpis;
          }
        }
    public:
      ucbit uc;
    };
  class LexicalConstHex : public LexicalBase
    {
    public:
      LexicalConstHex(const uint8 h, const Range& range = gk_min_range)
        :LexicalBase(LT_ConstHex, range), hex(h)
        {
        ;
        }
      virtual const char* type_name() const
        {
        return "词法consthex";
        }
      virtual bool fusion(LexicalBase* lb)
        {
        if(lb->type != LT_ConstHex) return false;
        if(hex != ((LexicalConstHex*)lb)->hex) return false;
        range += ((LexicalConstHex*)lb)->range;
        return true;
        }
      virtual bool test(const xblk& blk) const
        {
        if(!readable(blk)) return false;
        const uint8* lp = (const uint8*)blk.start();
        xsdbg << "    匹配 consthex ： " << hex << " ：\r\n"
          << showbin(blk.start(), blk.size(), HC_ASCII, true, 6);
        for(size_t i = 0; i < blk.size(); ++i)
          {
          if(lp[i] != hex) return false;
          }
        return true;
        }
      virtual void to_atom(vline& nline) const
        {
        nline << hex;
        }
    public:
      uint8 hex;
    };
  struct MarkRef
    {
    bool mark_right : 1;
    bool mark_left : 1;
    char ref_right : 4;
    char ref_left : 4;
    };
  static const MarkRef gk_none_mark_ref = { false, false, 0, 0 };
  static const MarkRef gk_err_mark_ref = { true, true, 1, 1 };
  static bool operator==(const MarkRef& mra, const MarkRef& mrb)
    {
    return (mra.ref_left == mrb.ref_left) &&
      (mra.mark_left == mrb.mark_left) &&
      (mra.ref_right == mrb.ref_right) &&
      (mra.mark_right == mrb.mark_right);
    }
  static bool operator!=(const MarkRef& mra, const MarkRef& mrb)
    {
    return !(mra == mrb);
    }
  class LexicalMarkRef : public LexicalBase
    {
    public:
      LexicalMarkRef(const ucbit& u, const MarkRef& mr)
        :LexicalBase(LT_MarkRef, gk_min_range), uc(u), mark_ref(mr)
        {
        ;
        }
      virtual const char* type_name() const
        {
        return "词法markref";
        }
      virtual bool test(const xblk& blk) const
        {
        if(!readable(blk)) return false;
        const uint8* lp = (const uint8*)blk.start();
        xsdbg << "    匹配 markref ：\r\n"
          << showbin(blk.start(), blk.size(), HC_ASCII, true, 6);
        xsdbg << showbin((void*)&uc, sizeof(uc), HC_ASCII, true, 6);
        for(size_t i = 0; i < blk.size(); ++i)
          {
          if(!uc.test(lp[i])) return false;
          }
        return true;
        }
      virtual void to_atom(vline& nline) const
        {
        const int32* lpis = (const int32*)&uc;
        const int32* lpie = lpis + (sizeof(uc) / sizeof(int32));
        for(; lpis != lpie; ++lpis)
          {
          nline << *lpis;
          }
        nline << mark_ref;
        }
    public:
      ucbit     uc;
      MarkRef   mark_ref;
    };
  }

////////// 词法 hexhex 、 ucbit 、 consthex 、 markreg 、 refreg 识别函数 ////////////
namespace signaturematcher
  {
  /// 识别 hexhex ，成功返回 string 包含一个字符，非词法为空，错误则包含不只一个字符。
  static string match_hexhex(Sign& sig)
    {
    const char hhex = match_hex(sig);

    if(hhex < 0)
      {
      if(sig.ch() != ':')   return string();
      sig.step();
      const char hex = sig.ch();
      if(hex == '\0')
        {
        xserr << sig << " \":\" 后提前结束。";
        return string("err");
        }
      sig.step();
      return string(1, hex);
      }

    match_ws(sig);

    const char lhex = match_hex(sig);

    if(lhex < 0)
      {
      xserr << sig << " hexhex 配对失败。";
      return string("err");
      }

    return string(1, hhex << 4 | lhex);
    }
  /// 识别 markreg 和 refreg 的后缀词法，返回 gk_err_mark_ref 、 gk_none_mark_ref 或结果。
  static MarkRef match_markref(Sign& sig)
    {
    match_ws(sig);
    MarkRef mr = gk_none_mark_ref;
    while(sig.ch() != '\0')
      {
      switch(sig.ch())
        {
        case '@':
          {
          sig.step();
          match_ws(sig);
          switch(sig.ch())
            {
            case 'L':       case 'l':
              if(mr.mark_left == true)
                {
                xserr << sig << " 重复标识左寄存器。";
                return gk_err_mark_ref;
                }
              if(mr.ref_left != 0)
                {
                xserr << sig << " 已引用左寄存器，不能继续标识，请指明其一。";
                return gk_err_mark_ref;
                }
              mr.mark_left = true;
              xsdbg << "  识别到 mark_left 。";
              break;
            case 'R':       case 'r':
              if(mr.mark_right == true)
                {
                xserr << sig << " 重复标识右寄存器。";
                return gk_err_mark_ref;
                }
              if(mr.ref_right != 0)
                {
                xserr << sig << " 已引用右寄存器，不能继续标识，请指明其一。";
                return gk_err_mark_ref;
                }
              mr.mark_right = true;
              xsdbg << "  识别到 mark_right 。";
              break;
            default:
              xserr << sig << " 未知的标记符：" << sig.ch() << "，请指定 [LR] 。";
              return gk_err_mark_ref;
            }
          sig.step();
          }
          break;
        case '$':
          {
          sig.step();
          match_ws(sig);
          char hex = match_hex(sig);
          if(hex <= 0)
            {
            hex = 1;
            }
          match_ws(sig);
          switch(sig.ch())
            {
            case 'L':   case 'l':
              if(mr.mark_left == true)
                {
                xserr << sig << " 已标识左寄存器，不能引用，请指明其一。";
                return gk_err_mark_ref;
                }
              if(mr.ref_left != 0)
                {
                xserr << sig << " 已引用左寄存器。";
                return gk_err_mark_ref;
                }
              mr.ref_left = hex;
              xsdbg << "  识别到 ref_left ： " << (uint8)hex;
              break;
            case 'R':   case 'r':
              if(mr.mark_right == true)
                {
                xserr << sig << " 已标识右寄存器，不能引用，请指明其一。";
                return gk_err_mark_ref;
                }
              if(mr.ref_right != 0)
                {
                xserr << sig << " 已引用右寄存器。";
                return gk_err_mark_ref;
                }
              mr.ref_right = hex;
              xsdbg << "  识别到 ref_right ： " << (uint8)hex;
              break;
            default:
              xserr << sig << " 未知的引用符或索引：" << sig.ch() << "，请指定 [1-F][LR] 。";
              return gk_err_mark_ref;
            }
          sig.step();
          }
          break;
        default:  return mr;
        }
      }
    return mr;
    }
  /**
    优化识别结果。
    当非模糊匹配时，如果范围固定，转化为 LT_String ，否则转化为 LT_ConstHex 。
    当模糊匹配全部时，转化为 LT_Dot ，否则为 LT_Ucbit 。
  */
  static LexicalBase* ucbit_optimization(const ucbit& uc, const Range& range)
    {
    if(uc.count() == 1)
      {
      size_t i = 0;
      for(; i < uc.size(); ++i)
        {
        if(uc.test(i))  break;
        }
      if(range.Min == range.Max)
        {
        xsdbg << "非模糊匹配且范围固定，转换为 string 。";
        return new LexicalString(string(range.Min, (char)i));
        }
      xsdbg << "非模糊匹配，但范围不定，转换为 consthex 。";
      return new LexicalConstHex((char)i, range);
      }
    if(uc.count() == uc.size())
      {
      xsdbg << "模糊匹配全部，优化为 dot 词法。";
      return new LexicalDot(range);
      }
    xsdbg << "无法优化，返回词法 ucbit 。";
    return new LexicalUcbit(uc, range);
    }
  /// 识别 ucbit 、 consthex 、 markreg 、 refreg 词法，返回结果参考 ucbit_optimization 。
  static LexicalBase* match_hexhexs(Sign& sig)
    {
    string ls(match_hexhex(sig));

    if(ls.empty())  return nullptr;
    if(ls.size() != 1) return new LexicalBase(LT_Error);

    uint8 lhex = *(ls.c_str());
    xsdbg << "识别到 hexhex 左值：" << (void*)lhex;

    const MarkRef mr = match_markref(sig);
    if(mr == gk_err_mark_ref) return new LexicalBase(LT_Error);

    ucbit uc;

    if(mr != gk_none_mark_ref)
      {
      xsdbg << "  识别到 markref： mark_left " << mr.mark_left
        << " mark_right " << mr.mark_right
        << " ref_left:" << (uint8)mr.ref_left
        << " ref_right:" << (uint8)mr.ref_right;
      uint8 ahex = lhex;
      uint8 bhex = lhex;
      if((mr.mark_right == true) || (mr.ref_right != 0))
        {
        ahex &= 0xF8;
        bhex |= 0x07;
        }
      if((mr.mark_left == true) || (mr.ref_left != 0))
        {
        ahex &= 0xC7;
        bhex |= 0x38;
        }
      xsdbg << "  处理 mrakref： " << (void*)ahex << " & " << (void*)bhex;
      uc.set_obfus(ahex, bhex);
      return new LexicalMarkRef(uc, mr);
      }

    // 注意不直接使用 while，以避免遇到结尾 0 ，而前值未处理的情况。
    do
      {
      match_ws(sig);
      char oper = sig.ch();
      switch(oper)
        {
        case '&':     case '|':    case '-':
          sig.step(); break;
        default:      oper = 0;
        }
      // 无连接符则退出。
      if(oper == 0)
        {
        uc.set(lhex);
        break;
        }

      match_ws(sig);
      ls = match_hexhex(sig);
      if(ls.empty())
        {
        // 处理 &L &R 的情况。
        if(oper == '&')
          {
          switch(sig.ch())
            {
            case 'L':     case 'l':
              uc.set_obfus(lhex & 0xC7, lhex | 0x38);
              sig.step();
              break;
            case 'R':     case 'r':
              uc.set_obfus(lhex & 0xF8, lhex | 0x07);
              sig.step();
              break;
            default:
              xserr << sig << " 操作符 '&' 后需要一个完整 hexhex 或 [LR] 标记。";
              return new LexicalBase(LT_Error);
            }
          continue;
          }
        else
          {
          xserr << sig << " 操作符 '" << oper << "' 后需要一个完整 hexhex 。";
          return new LexicalBase(LT_Error);
          }
        }
      if(ls.size() != 1) return new LexicalBase(LT_Error);

      const uint8 rhex = *(ls.c_str());
      xsdbg << "  识别到 hexhex 右值 " << oper << (void*)rhex;

      switch(oper)
        {
        case '&':   uc.set_obfus(lhex, rhex);     break;
        case '|':   uc.set(lhex);  uc.set(rhex);  break;
        case '-':   uc.set_queue(lhex, rhex);     break;
        default:;
        }

      lhex = rhex;
      } while(sig.ch() != '\0');

    const Range range = match_range(sig);
    if(range == gk_err_range)  return new LexicalBase(LT_Error);

    return ucbit_optimization(uc, range);
    }
  }

//////////////////////// 词法 collection 识别函数 ////////////////////////////
namespace signaturematcher
  {
  /// 识别 collection 词法，返回结果参考 ucbit_optimization 。
  static LexicalBase* match_collection(Sign& sig)
    {
    if(sig.ch() != '[') return nullptr;
    sig.step();
    match_ws(sig);
    xsdbg << "识别到词法 collection 。";

    // 识别可能存在的翻转请求。
    const bool reversal = (sig.ch() == '^');
    if(reversal)         
      {
      sig.step();
      match_ws(sig);
      xsdbg << "  识别到集合翻转指示 ^ 。";
      }

    uint8 lhex;
    ucbit uc;
    while(sig.ch() != ']' || sig.ch() != '\0')
      {
      match_ws(sig);
      string ls(match_hexhex(sig));

      if(ls.empty())  break;
      if(ls.size() != 1) return new LexicalBase(LT_Error);
      lhex = *(ls.c_str());
      xsdbg << "  识别到 hexhex 左值：" << (void*)lhex;

      char oper = sig.ch();
      switch(oper)
        {
        case '&':     case '|':    case '-':
          sig.step(); break;
        default:      oper = 0;
        }
      if(oper == 0)
        {
        uc.set(lhex);
        continue;
        }

      match_ws(sig);
      ls = match_hexhex(sig);
      if(ls.empty())
        {
        xserr << sig << " 操作符 '" << oper << "' 后需要一个完整 hexhex 。";
        return new LexicalBase(LT_Error);
        }
      if(ls.size() != 1) return new LexicalBase(LT_Error);

      const uint8 rhex = *(ls.c_str());
      xsdbg << "  识别到 hexhex 右值 " << oper << (void*)rhex;

      switch(oper)
        {
        case '&':   uc.set_obfus(lhex, rhex);     break;
        case '|':   uc.set(lhex);  uc.set(rhex);  break;
        case '-':   uc.set_queue(lhex, rhex);     break;
        default:;
        }
      lhex = rhex;
      }

    match_ws(sig);
    if(sig.ch() != ']')
      {
      xserr << sig << " 集合需要 ']' 符以标识结束。";
      }
    sig.step();

    if(reversal)  uc.flip();

    if(uc.none())
      {
      xserr << sig << " 集合不允许空匹配，请检查。";
      return new LexicalBase(LT_Error);
      }

    xsdbg << "词法 collection 成功读取。";

    const Range range = match_range(sig);
    if(range == gk_err_range)  return new LexicalBase(LT_Error);

    return ucbit_optimization(uc, range);
    }
  }

//////////////////////// 词法 record 类及识别函数 ////////////////////////////
namespace signaturematcher
  {
  class LexicalRecord : public LexicalBase
    {
    public:
      LexicalRecord(const LexicalType type, const Range& r, const string& n, const bool offset)
        :LexicalBase(type, r), name(n), isoff(offset)
        {
        ;
        }
      virtual bool record() const
        {
        return true;
        }
      virtual REPORT_VALUE pick_value(void* start) const = 0;
      virtual void to_atom(vline& nline) const
        {
        nline << isoff << name.size() << name;
        }
    public:
      string name;
      bool   isoff;
    };
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:4244)  // C4244: 可能丢失数据。
#endif
  class LexicalRecordAddr : public LexicalRecord
    {
    public:
      LexicalRecordAddr(const string& n, const bool offset)
        :LexicalRecord(LT_Record_Addr, gk_none_range, n, offset)
        {
        ;
        }
      virtual const char* type_name() const
        {
        return "词法record_addr";
        }
      virtual REPORT_VALUE pick_value(void* start) const
        {
        REPORT_VALUE rv;
        rv.t = 'p';
        rv.q = 0;
        rv.p = match_mem;
        if(isoff) rv.p = (void*)((size_t)rv.p - (size_t)start);
        return rv;
        }
    };
  class LexicalRecordOffset : public LexicalRecord
    {
    public:
      LexicalRecordOffset(const string& n, const bool offset)
        :LexicalRecord(LT_Record_Offset, Range(sizeof(uint32)), n, offset)
        {
        ;
        }
      virtual const char* type_name() const
        {
        return "词法record_offset";
        }
      virtual REPORT_VALUE pick_value(void* start) const
        {
        REPORT_VALUE rv;
        const int32 off = *(int32*)match_mem;
        rv.t = 'p';
        rv.q = 0;
        rv.p = (void*)((uint8*)match_mem + off + sizeof(off));
        if(isoff) rv.p = (void*)((size_t)rv.p - (size_t)start);
        return rv;
        }
    };
  class LexicalRecordQword : public LexicalRecord
    {
    public:
      LexicalRecordQword(const string& n, const bool offset)
        :LexicalRecord(LT_Record_Qword, Range(sizeof(uint64)), n, offset)
        {
        ;
        }
      virtual const char* type_name() const
        {
        return "词法record_qword";
        }
      virtual REPORT_VALUE pick_value(void* start) const
        {
        REPORT_VALUE rv;
        rv.t = 'q';
        rv.q = *(uint64*)match_mem;
        if(isoff) rv.q = rv.q - (uint64)start;
        return rv;
        }
    };
  class LexicalRecordDword : public LexicalRecord
    {
    public:
      LexicalRecordDword(const string& n, const bool offset)
        :LexicalRecord(LT_Record_Dword, Range(sizeof(uint32)), n, offset)
        {
        ;
        }
      virtual const char* type_name() const
        {
        return "词法record_dword";
        }
      virtual REPORT_VALUE pick_value(void* start) const
        {
        REPORT_VALUE rv;
        rv.t = 'd';
        rv.q = 0;
        rv.d = *(uint32*)match_mem;
        if(isoff) rv.d = rv.d - (uint32)start;
        return rv;
        }
    };
  class LexicalRecordWord : public LexicalRecord
    {
    public:
      LexicalRecordWord(const string& n, const bool offset)
        :LexicalRecord(LT_Record_Word, Range(sizeof(uint16)), n, offset)
        {
        ;
        }
      virtual const char* type_name() const
        {
        return "词法record_word";
        }
      virtual REPORT_VALUE pick_value(void* start) const
        {
        REPORT_VALUE rv;
        rv.t = 'w';
        rv.q = 0;
        rv.w = *(uint16*)match_mem;
        if(isoff) rv.w = rv.w - (uint16)start;
        return rv;
        }
    };
  class LexicalRecordByte : public LexicalRecord
    {
    public:
      LexicalRecordByte(const string& n, const bool offset)
        :LexicalRecord(LT_Record_Byte, Range(sizeof(uint8)), n, offset)
        {
        ;
        }
      virtual const char* type_name() const
        {
        return "词法record_byte";
        }
      virtual REPORT_VALUE pick_value(void* start) const
        {
        REPORT_VALUE rv;
        rv.t = 'b';
        rv.q = 0;
        rv.b = *(uint8*)match_mem;
        if(isoff) rv.b = rv.b - (uint8)start;
        return rv;
        }
    };
#ifdef _WIN32
#pragma warning(pop)
#endif
  /// 识别 record 词法。
  static LexicalBase* match_record(Sign& sig)
    {
    if(sig.ch() != '<') return nullptr;
    sig.step();
    match_ws(sig);
    const bool offset = sig.ch() == '^';
    if(offset)
      {
      sig.step();
      match_ws(sig);
      }
    LexicalRecord* lr = nullptr;
    const string nullstr;
    switch(sig.ch())
      {
      case 'A':       case 'a':
        lr = new LexicalRecordAddr(nullstr, offset);  break;
      case 'F':       case 'f':
        lr = new LexicalRecordOffset(nullstr, offset);  break;
      case 'Q':       case 'q':
        lr = new LexicalRecordQword(nullstr, offset);  break;
      case 'D':       case 'd':
        lr = new LexicalRecordDword(nullstr, offset);  break;
      case 'W':       case 'w':
        lr = new LexicalRecordWord(nullstr, offset);  break;
      case 'B':       case 'b':
        lr = new LexicalRecordByte(nullstr, offset);  break;
      default:
        xserr << sig << " 特征点请指定 [AFQDWB] 。";
        return new LexicalBase(LT_Error);
      }
    sig.step();

    match_ws(sig);

    string& name = lr->name;
    do
      {
      char ch = sig.ch();
      switch(ch)
        {
        case '\r':  case '\n':  case '\0':  case '>':  case ' ':
          ch = 0;
          break;
        default:
          break;
        }
      if(ch == 0) break;
      sig.step();
      name.append(1, ch);
      }while(sig.ch() != '\0');

    match_ws(sig);
    if(sig.ch() != '>')
      {
      xserr << sig << " 特征点需要 '>' 结束。";
      return new LexicalBase(LT_Error);
      }
    sig.step();
    match_ws(sig);

    xsdbg << "识别到 " << lr->type_name() << (offset ? '^' : ' ') << name;
    return lr;
    }
  }

////////////////////////////// 词法识别 ////////////////////////////////////
namespace signaturematcher
  {
  /// 词法识别函数格式。
  typedef LexicalBase* (*LexicalMatcher)(Sign& sig);
  /// 词法识别函数组。
  static const LexicalMatcher gk_lexical_matcher[] = {
    &match_ws,
    &match_note,    // 注意， note 词法应放在其它词法之前。
    &match_dot,
    &match_hexhexs,
    &match_record,
    &match_collection,
    &match_string_quote,
    &match_end,     // 注意， end 词法应放在最后。
    };

  vector<LexicalBase*> create_lexer(SIGNATURE signature)
    {
    Sign sig(signature);
    vector<LexicalBase*> vec;
    size_t matcher_lp = 0;
    size_t mark_count = 0;
    xsdbg << "----------------------- 词法分析开始 -----------------------";
    for(; matcher_lp < _countof(gk_lexical_matcher); ++matcher_lp)
      {
      LexicalBase* lb = gk_lexical_matcher[matcher_lp](sig);

      // 非此词法转下条。
      if(lb == nullptr) continue;

      xsdbg << "词法生成：" << lb->type << '-' << lb->type_name();

      // 非法词法，停止。
      if(!lb->valid())
        {
        delete lb;
        break;
        }

      if(lb->type == LT_End)
        {
        delete lb;

        xsdbg << "----------------------- 词法分析结束 -----------------------";

        if(vec.empty()) return vec;

        // 判定至少要存在一个 record ，否则添加。
        bool need_add = true;

        for(auto& lbb : vec)
          {
          if(lbb->record())
            {
            need_add = false;
            break;
            }
          }
        if(need_add)
          {
          xsdbg << "由于没有特征点，头部自动添加。";
          const string nullstr;
          vec.insert(vec.begin(), new LexicalRecordAddr(nullstr, false));
          }

        return vec;
        }

      // 对引用索引进行校验。
      if(lb->type == LT_MarkRef)
        {
        LexicalMarkRef* lmr = (LexicalMarkRef*)lb;
        if(lmr->mark_ref.mark_left)   ++mark_count;
        if(lmr->mark_ref.mark_right)  ++mark_count;
        xsdbg << "可能存在索引，之前标记个数：" << mark_count;
        // 判定索引位是否超过标记个数。
        if((size_t)(lmr->mark_ref.ref_left) > mark_count ||
           (size_t)(lmr->mark_ref.ref_right) > mark_count)
          {
          delete lb;
          xserr << sig << "索引位置无法确定，请检查索引。";
          break;
          }
        xsdbg << "索引检查通过。";
        }

      // 词法非空时，考虑同类型融合。
      if(!vec.empty())
        {
        LexicalBase* olb = *(vec.rbegin());
        // 尝试融合，整合成功则释放资源不加入 vec 。
        if(olb->fusion(lb))
          {
          xsdbg << "   成功融合。\r\n\r\n";
          delete lb;
          lb = nullptr;
          }
        }

      if(lb != nullptr)
        {
        vec.push_back(lb);
        xsdbg << "    加入类型： " << lb->type_name() << " 。\r\n\r\n";
        }

      // 词法成功识别，则从头开始。
      matcher_lp = 0;
      --matcher_lp;
      }

    if(matcher_lp >= _countof(gk_lexical_matcher))
      xserr << sig << " 无法识别的词法。";

    xsdbg << "----------------------- 词法分析失败 -----------------------";
    // 词法识别失败，释放资源。
    for(auto& lb : vec)
      {
      delete lb;
      }
    vec.clear();
    return vec;
    }
  }

//////////////////////// 原子生成、原子识别 ////////////////////////////////
namespace signaturematcher
  {
  vline create_atom(SIGNATURE signature)
    {
    vector<LexicalBase*> vec(create_lexer(signature));
    if(vec.empty()) return vline();
    vline nline;
    for(auto& lb : vec)
      {
      lb->create_atom(nline);
      delete lb;
      }
    nline << LT_End << gk_err_range.Min << gk_err_range.Max;
    return nline;
    }
  /// 指定原子，生成词法组。
  static vector<LexicalBase*> LexicalAtom(vline& atom)
    {
    vector<LexicalBase*> vec;
    bool end = false;
    while(!atom.empty() && !end)
      {
      LexicalType type;
      atom >> type;
      Range range(gk_err_range);
      atom >> range.Min >> range.Max;
      switch(type)
        {
        case LT_End:
          xsdbg << "识别到词法 end。";
          end = true;
          break;
        case LT_String:
          {
          size_t size;
          atom >> size;
          const string str((const char*)atom.c_str(), size);
          atom.erase(0, size);
          xsdbg << "识别到词法string ：\r\n" << showbin(str, (size_t)2);
          vec.push_back(new LexicalString(str));
          }
          break;
        case LT_Quote:
          {
          size_t size;
          atom >> size;
          const string str((const char*)atom.c_str(), size);
          atom.erase(0, size);
          xsdbg << "识别到词法quote ：\r\n" << showbin(str, (size_t)2);
          vec.push_back(new LexicalQuote(str));
          }
          break;
        case LT_Dot:
          xsdbg << "识别到词法dot ： { " << (void*)range.Min << ", " << (void*)range.Max << " }";
          vec.push_back(new LexicalDot(range));
          break;
        case LT_MarkRef:
          {
          ucbit uc;
          MarkRef mr;
          int32* lpis = (int32*)&uc;
          const int32* lpie = lpis + (sizeof(uc) / sizeof(int32));
          for(; lpis != lpie; ++lpis)
            {
            atom >> *lpis;
            }
          atom >> mr;
          xsdbg << "识别到词法markref ： mark_left " << mr.mark_left
            << " mark_right " << mr.mark_right
            << " ref_left: " << (void*)mr.ref_left
            << " ref_right: " << (void*)mr.mark_right;
          xsdbg << showbin((void*)&uc, sizeof(uc), HC_ASCII, true, 2);
          vec.push_back(new LexicalMarkRef(uc, mr));
          }
          break;
        case LT_Ucbit:
          {
          ucbit uc;
          int32* lpis = (int32*)&uc;
          const int32* lpie = lpis + (sizeof(uc) / sizeof(int32));
          for(; lpis != lpie; ++lpis)
            {
            atom >> *lpis;
            }
          xsdbg << "识别到词法ucbit： { " << (void*)range.Min << ", " << (void*)range.Max << " }";
          xsdbg << showbin((void*)&uc, sizeof(uc), HC_ASCII, true, 2);
          vec.push_back(new LexicalUcbit(uc, range));
          }
          break;
        case LT_ConstHex:
          {
          uint8 hex;
          atom >> hex;
          xsdbg << "识别到词法consthex ： "<< (void*)hex
            << "{ " << (void*)range.Min << ", " << (void*)range.Max << " }";
          vec.push_back(new LexicalConstHex(hex, range));
          }
          break;
        case LT_Record_Addr:
        case LT_Record_Offset:
        case LT_Record_Qword:
        case LT_Record_Dword:
        case LT_Record_Word:
        case LT_Record_Byte:
          {
          bool offset;
          atom >> offset;
          size_t size;
          atom >> size;
          const string name((const char*)atom.c_str(), size);
          atom.erase(0, size);
          LexicalBase* lb = nullptr;
          switch(type)
            {
            case LT_Record_Addr:
              lb = new LexicalRecordAddr(name, offset);
              break;
            case LT_Record_Offset:
              lb = new LexicalRecordOffset(name, offset);
              break;
            case LT_Record_Qword:
              lb = new LexicalRecordQword(name, offset);
              break;
            case LT_Record_Dword:
              lb = new LexicalRecordDword(name, offset);
              break;
            case LT_Record_Word:
              lb = new LexicalRecordWord(name, offset);
              break;
            case LT_Record_Byte:
              lb = new LexicalRecordByte(name, offset);
              break;
            default:;
            }
          xsdbg << "识别到 "
            << (offset ? " ^ " : " ")
            << lb->type_name() << "  " << name;
          vec.push_back(lb);
          }
          break;
        default:
          {
          xserr << "未知类型： " << type << " ，无法继续从原子生成词法。";
          xserr << showbin(atom);
          for(auto& lb : vec)
            {
            delete lb;
            }
          vec.clear();
          return vec;
          }
        }
      }
    return vec;
    }
  }

////////////////////////// 特征码匹配后继校验 //////////////////////////////
namespace signaturematcher
  {
  /// 校验引用。
  static bool match_check_quote(const LexicalQuote* lb)
    {
    XLIB_TRY
      {
      void* quote = *(void**)lb->match_mem;
      xsdbg << "校验引用串：\r\n"
        << showbin(lb->str, (size_t)2)
        << showbin(quote, lb->str.size(), HC_ASCII, true, 2);
      if(memcmp(quote, lb->str.c_str(), lb->str.size()) != 0)
        {
        xsdbg << "引用校验 " << lb->match_mem << " ： " << quote << " 无法通过。";
        return false;
        }
      return true;
      }
    XLIB_CATCH
      {
      ;
      }
    return false;
    }
  /// 校验 ref 。
  static bool match_check_ref(vector<LexicalBase*>::const_iterator itt,
                              intptr_t refs,
                              const uint8 ch_ref)
    {
    void* srclp = (*itt)->match_mem;
    for(; refs > 0; --itt)
      {
      const LexicalBase* lb_ref = *itt;
      if(lb_ref->type != LT_MarkRef)  continue;
      const LexicalMarkRef* lmr_ref = (const LexicalMarkRef*)lb_ref;

      if(lmr_ref->mark_ref.mark_right)
        {
        --refs;
        if(refs == 0)
          {
          const uint8 ch = *(uint8*)lmr_ref->match_mem;
          xsdbg << "  找到了 ref ，是 @R " << lmr_ref->match_mem << ':' << ch;
          if(ch_ref != (ch & 0x07))
            {
            xserr << "原值 " << srclp << " ： " << ch_ref
              << " @R 引用值 " << lb_ref->match_mem << " ："
              << (uint8)(ch & 0x07)
              << " 无法匹配。";
            return false;
            }
          break;
          }
        xsdbg << "  跳过一个 @R 。";
        }

      if(lmr_ref->mark_ref.mark_left)
        {
        --refs;
        if(refs == 0)
          {
          const uint8 ch = *(uint8*)lmr_ref->match_mem;
          xsdbg << "  找到了 ref ，是 @L " << lmr_ref->match_mem << ':' << ch;
          if(ch_ref != ((ch & 0x38) >> 3))
            {
            xserr << "原值 " << srclp << " ： " << (void*)ch_ref
              << " @L 引用值 " << lb_ref->match_mem << " ："
              << (uint8)((ch & 0x38) >> 3)
              << " 无法匹配。";
            return false;
            }
          break;
          }
        xsdbg << "  跳过一个 @L。";
        }
      }
    xsdbg << "匹配，通过。";
    return true;
    }
  static bool match_check_markref(vector<LexicalBase*>::const_iterator it)
    {
    XLIB_TRY
      {
      const LexicalMarkRef* lmr = (const LexicalMarkRef*)*it;
      vector<LexicalBase*>::const_iterator itt = it;

      const uint8 ch_ref = *(uint8*)lmr->match_mem;
      // 检查是否存在左引用。
      if(lmr->mark_ref.ref_left != 0)
        {
        const intptr_t refs = lmr->mark_ref.ref_left;
        xsdbg << "发现 $" << refs << "L  : " << lmr->match_mem << ':' << ch_ref;

        if(!match_check_ref(itt, refs, ((ch_ref & 0x38) >> 3))) return false;
        }
      // 检查是否存在右引用。
      if(lmr->mark_ref.ref_right != 0)
        {
        const intptr_t refs = lmr->mark_ref.ref_right;
        xsdbg << "发现 $" << refs << "R  : " << lmr->match_mem << ':' << ch_ref;

        if(!match_check_ref(itt, refs, ch_ref & 0x07)) return false;
        }
      return true;
      }
    XLIB_CATCH
      {
      ;
      }
    return false;
    }
  /// 总校验。
  static vector<LexicalBase*>::const_iterator match_check(const vector<LexicalBase*>& vec)
    {
    XLIB_TRY
      {
      xsdbg << "----------- 正在校验 -----------";
      // 开始引用校验。
      for(auto it = vec.begin(); it != vec.end(); ++it)
        {
        const LexicalBase* lb = *it;
        if(lb->type != LT_Quote)  continue;
        if(!match_check_quote((const LexicalQuote*)lb))  return it;
        }

      // 开始 refreg 校验。
      for(auto it = vec.begin(); it != vec.end(); ++it)
        {
        const LexicalBase* lb = *it;
        if(lb->type != LT_MarkRef)  continue;
        if(!match_check_markref(it)) return it;
        }
      xsdbg << "----------- 校验完成 -----------";
      return vec.end();
      }
    XLIB_CATCH
      {
      xsdbg << "-----------校验异常-----------";
      }
    return vec.end() + 1;
    }
  }

////////////////////////// 特征点数据收集相关 //////////////////////////////
namespace signaturematcher
  {
  static bool report_check(REPORT& report, const string& name, const REPORT_VALUE& node)
    {
    if(name.empty())  return true;
    REPORT::const_iterator it = report.find(name);
    if(it == report.end())  return true;
    const REPORT_VALUE& crv = (*it).second;
    if(crv.t != node.t)
      {
      xserr << "特征点 " << name << " 类型不符，原值 : "
        << crv.t << " ，现值 : " << node.t;
      return false;
      }
    switch(crv.t)
      {
      case 'd':
        if(crv.d != node.d)
          {
          xserr << "特征点 " << name << " 类型不符，原值 : "
            << crv.d << " ，现值 : " << node.d;
          return false;
          }
        break;
      case 'w':
        if(crv.w != node.w)
          {
          xserr << "特征点 " << name << " 类型不符，原值 : "
            << crv.w << "，现值 : " << node.w;
          return false;
          }
        break;
      case 'b':
        if(crv.b != node.b)
          {
          xserr << "特征点 " << name << " 类型不符，原值 : "
            << crv.b << "，现值 : " << node.b;
          return false;
          }
        break;
      case 'p':
        if(crv.p != node.p)
          {
          xserr << "特征点 " << name << " 类型不符，原值 : "
            << crv.p << "，现值 : " << node.p;
          return false;
          }
        break;
      default:
        if(crv.q != node.q)
          {
          xserr << "特征点 " << name << " 类型不符，原值 : "
            << crv.q << "，现值 : " << node.q;
          return false;
          }
        break;
      }
    xsdbg << "特征点 " << name << " 重复，略过。";
    return true;
    }
  bool report_add(REPORT& report, string name, const REPORT_VALUE& node)
    {
    if(!name.empty())
      {
      if(!report_check(report, name, node)) return false;
      }
    else
      {
      xmsg tmpname;
      tmpname << "noname" << report.size();
      name = tmpname;
      }
    report[name] = node;
    xsdbg << "新添特征点：" << node.q << ':' << name << " 。";
    return true;
    }
  /// 提取特征点数据及相关信息。
  static vector<LexicalBase*>::const_iterator fix_report(void* start, const vector<LexicalBase*>& vec, REPORT& report)
    {
    xsdbg << "----------- 正在提取 -----------";
    for(vector<LexicalBase*>::const_iterator it = vec.begin(); it != vec.end(); ++it)
      {
      LexicalBase* lb = *it;
      if(!lb->record()) continue;
      LexicalRecord* lr = (LexicalRecord*)lb;
      
      if(!report_check(report, lr->name, lr->pick_value(start))) return it;;
      }
    for(vector<LexicalBase*>::const_iterator it = vec.begin(); it != vec.end(); ++it)
      {
      LexicalBase* lb = *it;
      if(!lb->record()) continue;
      LexicalRecord* lr = (LexicalRecord*)lb;

      report_add(report, lr->name, lr->pick_value(start));
      }
    xsdbg << "----------- 提取完成 -----------";
    return vec.end();
    }
  }

//////////////////////////// 特征码匹配内核 ////////////////////////////////
namespace signaturematcher
  {
  /// 校验与提取。
  static vector<LexicalBase*>::const_iterator fix_match(void* start,
                                                        const vector<LexicalBase*>& vec,
                                                        REPORT& report)
    {
    auto it = match_check(vec);
    if(it!= vec.end())
      {
      xsdbg << "----------- 校验失败 -----------";
      return it;
      }

    XLIB_TRY
      {
      it = fix_report(start, vec, report);
      if(it != vec.end())
        {
        xsdbg << "----------- 提取失败 -----------";
        return it;
        }
      }
    XLIB_CATCH
      {
      xserr << "----------- 提取异常 -----------";
      return vec.end() + 1;
      }
    return vec.end();
    }
  /// 匹配内核。
  static bool match(const xblk& blk,
                    const vector<LexicalBase*>& vec,
                    intptr_t lp,
                    REPORT& report)
    {
    XLIB_TRY
      {
      auto itt = vec.begin();
      while(itt != vec.end())
        {
        for(auto it = itt; it != vec.end();)
          {
          const auto lb = *it;

          if(lb->match(blk, lp))                  // 匹配成功，继续。
            {
            ++it;
            continue;
            }

          if(lp > (intptr_t)blk.size())  break;   // 匹配彻底失败，跳出。

          if(it == vec.begin())  break;           // 无法回退，跳出。

          --it;                                   // 正常回退。
          }
        if(lp > (intptr_t)blk.size())
          {
          xsdbg << "-------------------------------匹配失败-------------------------------";
          return false;
          }
        if((*(vec.rbegin()))->match_count < 0)  break;
        // 匹配成功后进行校验。
        itt = fix_match(blk.start(), vec, report);
        if(itt == vec.end())        break;      // 校验成功。
        if(itt == vec.end() + 1)    break;      // 校验异常。
        xsdbg << (*itt)->type_name() << " 在 " << (*itt)->match_mem << " 处校验失败。";
        // 回退到校验失败的元素处。
        for(auto it = vec.end() - 1; it != itt - 1; --it)
          {
          xsdbg << (*it)->type_name() << " 因校验失败无条件回退: " << (*it)->match_count;
          lp -= (*it)->match_count;
          (*it)->match_count = gk_error_rangetype;
          }
        --itt;
        // 检验之前的元素是否可以继续匹配，如果不能，则继续回退。
        for(; itt != vec.begin() - 1; --itt)
          {
          if((*itt)->match_count < (*itt)->range.Max) break;
          xsdbg << (*itt)->type_name() << " 因校验失败回退: " << (*itt)->match_count;
          lp -= (*itt)->match_count;
          (*itt)->match_count = gk_error_rangetype;
          }
        if(itt == vec.begin() - 1)  break;      // 退无可退。
        xsdbg << (*itt)->type_name() << " 在 " << (*itt)->match_mem << " 处重新开始。";
        }
      }
    XLIB_CATCH
      {
      xserr << "-------------------------------匹配异常-------------------------------";
      return false;
      }
    return true;
    }

  REPORT match(const xblk& blk, const vector<LexicalBase*>& vec)
    {
    if(vec.empty()) return REPORT();

    xsdbg << "-------------------------------开始匹配-------------------------------"
      << blk.start();
    xsdbg << "词法个数：" << vec.size();
    intptr_t lp = 0;
    REPORT report;
    for(; lp < (intptr_t)blk.size(); ++lp)
      {
      if(!match(blk, vec, lp, report))  return REPORT();

      const uint8* mem = (const uint8*)blk.start() + lp;

      // 检验匹配成功与否的标准是判定最后一个结点是否匹配完成。
      if((*(vec.rbegin()))->match_count >= 0)
        {
        xsdbg << "-------------------------------匹配成功-------------------------------"
          << (void*)mem;
        break;
        }
      xsdbg << "-------------------------------匹配递进-------------------------------"
        << (void*)(mem + 1);
      }

    return report;
    }
  }

//////////////////////////// 特征码匹配重载 ////////////////////////////////
namespace signaturematcher
  {
  REPORT match(void* start, void* end, SIGNATURE signature)
    {
    vector<LexicalBase*> vec(create_lexer(signature));
    if(vec.empty())   return REPORT();
    const xblk blk(start, end);

    REPORT report(match(blk, vec));

    for(auto& lb : vec)
      {
      delete lb;
      }

    return report;
    }

  REPORT match(void* start, void* end, vline& atom)
    {
    vector<LexicalBase*> vec(LexicalAtom(atom));
    if(vec.empty())   return REPORT();
    const xblk blk(start, end);

    REPORT report(match(blk, vec));

    for(auto& lb : vec)
      {
      delete lb;
      }

    return report;
    }

  REPORT match(const std::vector<xblk>& blks, SIGNATURE signature)
    {
    REPORT report;
    vector<LexicalBase*> vec(create_lexer(signature));
    if(vec.empty())   return report;
    for(auto& blk : blks)
      {
      report = match(blk, vec);
      if(!report.empty())  break;
      }

    for(auto& lb : vec)
      {
      delete lb;
      }
    return report;
    }

  REPORT match(const std::vector<xblk>& blks, vline& atom)
    {
    REPORT report;
    vector<LexicalBase*> vec(LexicalAtom(atom));
    if(vec.empty())   return report;
    for(auto& blk : blks)
      {
      report = match(blk, vec);
      if(!report.empty())  break;
      }

    for(auto& lb : vec)
      {
      delete lb;
      }
    return report;
    }

  REPORT_VALUE find(void* start, void* end, SIGNATURE signature)
    {
    REPORT report(match(start, end, signature));
    if(report.empty())
      {
      REPORT_VALUE rv;
      rv.t = 'n';
      rv.q = 0;
      return rv;
      }
    return (*report.begin()).second;
    }
  }

//////////////////////////// 特征码匹配脚本 ////////////////////////////////
namespace signaturematcher
  {
  /// 给定脚本，提取特征码串。
  static string pick_sign(string& script)
    {
    if(script.empty())
      {
      xsdbg << "给定脚本为空";
      return string();
      }

    string sign;

    // 消除起始空白。
    auto it = script.begin();
    for(; it != script.end(); ++it)
      {
      const uint8 ch = *it;
      if(ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n')
        {
        sign.push_back(*it);
        continue;
        }
      break;
      }
    script.erase(script.begin(), it);

    if(script.empty())
      {
      xsdbg << "给定脚本无有效数据。";
      return string();
      }

    // 提取起始，可以有起始标记 "/\r?\n" ，如果没有起始标记，当前即为起始。
    it = script.begin();
    if(*it == '/')
      {
      if(*(it + 1) == '\n')
        {
        script.erase(0, 2);   // / \n
        }
      else
        {
        if(*(it + 1) == '\r' && *(it + 2) == '\n')
          {
          script.erase(0, 3);   // / \r\n
          }
        }
      // 如果存在起始标记，清除之前保存的空白。
      sign.clear();
      }

    if(script.empty())
      {
      xserr << "给定脚本只有起始标记，无有效数据。";
      return string();
      }

    // 查找结束标记，提取特征码。
    it = script.begin();
    for(; it != script.end(); ++it)
      {
      sign.push_back(*it);

      if(*it == '/')
        {
        // '/'后继脚本结束，完成提取。
        if((it + 1) == script.end())
          {
          sign.pop_back();
          script.clear();
          xsdbg << "脚本提取到特征码:\r\n" << sign;
          return sign;
          }
        // '/'后继'\n'情况，结束标记，完成提取。
        if(*(it + 1) == '\n')
          {
          sign.pop_back();
          script.erase(script.begin(), it + 2);
          xsdbg << "脚本提取到一个特征码:\r\n" << sign;
          return sign;
          }
        if(*(it + 1) != '\r')  break;
        if((it + 2) == script.end())  break;
        // '/'后继"\r\n"情况，通过，定为起始。
        if(*(it + 2) != '\n') break;
        sign.pop_back();
        script.erase(script.begin(), it + 3);
        xsdbg << "脚本提取到一个特征码:\r\n" << sign;
        return sign;
        }
      }

    // 没有结束标记，全数作为特征码。
    for(; it != script.end(); ++it)
      {
      sign.push_back(*it);
      }
    script.clear();
    xsdbg << "脚本提取到特征码:\r\n" << sign;
    return sign;
    }

  /// 特征码中间原子文件头标记。
  static const string gk_atom_sign("SignAtom");

  /// 分离以使 ring0 下 SEH 正常。
  static bool analy_atom_script(string& script, analy_script_routine asr, void* lparam)
    {
    vline atoms;
    atoms.assign((const uint8*)script.c_str(), script.size());

    atoms.erase(0, gk_atom_sign.size());

    xsdbg << "给定的脚本是中间原子脚本。";

    while(!atoms.empty())
      {
      ScriptNode sn;
      atoms >> sn.type;
      // 中间原子的类型只有两种。
      size_t size = 0;
      atoms >> size;

      if(sn.type == ST_Blk)
        {
        sn.sig.assign((const char*)atoms.c_str(), size);
        atoms.erase(0, size);
        xsdbg << "中间原子脚本提取得到范围指示：" << sn.sig;
        if(!asr(sn, lparam))
          {
          script.erase(0, script.size() - atoms.size());
          return false;
          }
        continue;
        }

      if(sn.type != ST_Atom)
        {
        xserr << "中间原子未知类型数据:" << sn.type;
        script.erase(0, script.size() - atoms.size());
        return false;
        }

      sn.atom.assign(atoms.c_str(), size);
      atoms.erase(0, size);
      xsdbg << "中间原子脚本提取得到 ATOM ：\r\n" << showbin(sn.atom);
      if(!asr(sn, lparam))
        {
        script.erase(0, script.size() - atoms.size());
        return false;
        }
      }
    script.erase(0, script.size() - atoms.size());
    return true;
    }

  static bool analy_script_script(string& script, analy_script_routine asr, void* lparam)
    {
    xsdbg << "不是中间原子脚本，作为普通脚本处理。";
    while(!script.empty())
      {
      string sign(pick_sign(script));
      if(sign.empty())
        {
        return script.empty();
        }

      bool isblk = false;

      for(auto ch : sign)
        {
        if(ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') continue;
        if(ch == '@' || ch == '$')
          {
          isblk = true;
          }
        break;
        }

      if(isblk) xsdbg << "提取到的特征码是范围指示。";

      ScriptNode sn = { isblk ? ST_Blk : ST_Sign, sign };

      if(!asr(sn, lparam))  return false;
      }

    return true;
    }

  bool analy_script(string& script, analy_script_routine asr, void* lparam)
    {
    XLIB_TRY
      {
      if(script.empty())
        {
        xserr << "给定的脚本为空。";
        return false;
        }

      // 先判定是否中间原子。
      if(script.size() >= gk_atom_sign.size())
        {
        if(gk_atom_sign == script.substr(0, gk_atom_sign.size()))
          {
          return analy_atom_script(script, asr, lparam);
          }
        }

      // 作脚本处理。
      return analy_script_script(script, asr, lparam);
      }
    XLIB_CATCH
      {
      ;
      }
    xserr << "脚本解析未知异常。";
    return false;
    }

  static bool create_atom_routine(ScriptNode sn, void* lparam)
    {
    vline& atoms = *(vline*)lparam;

    if(sn.type == ST_Blk)
      {
      atoms << sn.type << sn.sig.size() << sn.sig;
      return true;
      }
    if(sn.type == ST_Sign)
      {
      sn.atom = create_atom(sn.sig.c_str());
      if(sn.atom.empty())
        {
        atoms.clear();
        return false;
        }
      atoms << ST_Atom << sn.atom;
      return true;
      }
    if(sn.type != ST_Atom)
      {
      xserr << "脚本建立中间原子未知类型: " << sn.type;
      }
    atoms.clear();
    return false;
    }

  vline create_atom_by_script(string& script)
    {
    vline atoms;
    atoms << gk_atom_sign;
    if(!analy_script(script, create_atom_routine, &atoms))
      {
      atoms.clear();
      }
    return atoms;
    }

  vector<xblk> analy_blk(string& blkstr)
    {
    vector<xblk> vec;
    // 消除前缀空白。
    for(string::iterator it = blkstr.begin(); it != blkstr.end(); ++it)
      {
      const char ch = *it;
      if(ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') continue;
      blkstr.erase(blkstr.begin(), it);
      break;
      }

    xblk tmpblk(nullptr, nullptr);

    while(!blkstr.empty())
      {
      const char flag = *blkstr.begin();
      if(flag != '@' && flag != '$')
        {
        xserr << "范围解析未知指示：" << flag;
        vec.clear();
        return vec;
        }

      blkstr.erase(blkstr.begin(), blkstr.begin() + 1);

      // 提取内容。
      string::iterator it = blkstr.begin();
      for(; it != blkstr.end(); ++it)
        {
        const char ch = *it;
        if(ch == '@' || ch == '$')
          {
          break;
          }
        }

      string blk(blkstr.begin(), it);
      blkstr.erase(blkstr.begin(), it);

      // 消除前缀空白。
      for(string::iterator itt = blk.begin(); itt != blk.end(); ++itt)
        {
        const char ch = *itt;
        if(ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') continue;
        blk.erase(blk.begin(), itt);
        break;
        }

      // 消除后缀空白。
      for(string::reverse_iterator itt = blk.rbegin(); itt != blk.rend();/* ++itt*/)
        {
        const char ch = *itt;
        if(ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n')
          {
          blk.pop_back();
          itt = blk.rbegin();
          continue;
          }
        break;
        }

      xsdbg << "提取到范围指示：" << blk;

      if(tmpblk.start() == nullptr)
        {
        // 尝试识别为模块。
        xsdbg << "尝试识别为模块...";
#ifdef _WIN32
#ifndef FOR_RING0
        HMODULE hmod = GetModuleHandleA(blk.c_str());
        if(hmod != nullptr)
          {
          pe thispe(hmod);
          if(thispe.IsPE())
            {
            auto i = thispe.GetImage();
            auto c = thispe.GetCode();
            tmpblk = (flag == '@') ? i : xblk(i.start(), c.end());
            }
          }
#else // FOR_RING0
        SysDriverSnap sds;
        for(const SYSTEM_MODULE& st : sds)
          {
          const char* name = (const char*)(st.Name + st.NameOffset);
          if(_stricmp(blk.c_str(), name) == 0)
            {
            if(flag == '@')
              {
              tmpblk = xblk(st.ImageBaseAddress, st.ImageSize);
              }
            else
              {
              pe thispe((HMODULE)st.ImageBaseAddress);
              if(thispe.IsPE())
                {
                auto i = thispe.GetImage();
                auto c = thispe.GetCode();
                tmpblk = xblk(i.start(), c.end());
                }
              }
            }
          }
#endif  // FOR_RING0
#endif  // _WIN32
        }
      if(tmpblk.size() != 0)
        {
        vec.push_back(tmpblk);
        xsdbg << "提取得到范围： { " << tmpblk.start() << ", " << tmpblk.end() << "}";
        tmpblk = xblk(nullptr, nullptr);
        continue;
        }

      // 如果识别模块失败，识别为范围。
      void* p = (void*)hex2value(blk);
      if(p == nullptr)
        {
        xserr << "无法识别的范围指示： " << blk;
        vec.clear();
        return vec;
        }

      if(tmpblk.start() == nullptr)
        {
        xsdbg << "提取到范围起始： " << p;
        tmpblk = xblk(p, p);
        continue;
        }

      tmpblk = xblk(tmpblk.start(), p);
      vec.push_back(tmpblk);
      xsdbg << "提取到范围结束，此时： {" << tmpblk.start() << " , "
        << tmpblk.end() << '}';
      tmpblk = xblk(nullptr, nullptr);
      }

    if(tmpblk.start() != nullptr && tmpblk.size() == 0)
      {
      xserr << "范围指示没有正确配对。";
      vec.clear();
      }

    return vec;
    }
  }

#ifdef _WIN32
#pragma warning(pop)
#endif

#ifdef _XLIB_TEST_

#include "pe.h"
static const char* const signaturematchertest = "4210421042104210421042104210";

ADD_XLIB_TEST(SIGNATUREMATCHER)
  {
  SHOW_TEST_INIT;

  auto done = false;
#ifdef _WIN32
  SHOW_TEST_HEAD("signaturematcher");
  pe my;
  const auto img(my.GetImage());
  //signaturematcher::log_level() = xlog::lvl_on;
  //auto rets = signaturematcher::find((void*)(signaturematchertest - 0x10), (void*)(signaturematchertest + 40), "<A>'4210421042104210421042104210'");
  auto rets = signaturematcher::find(img.start(), img.end(), "<A>'4210421042104210421042104210'");
  done = (signaturematchertest == rets.p);
  SHOW_TEST_RESULT(done);
#endif

  //signaturematcher::smlog::log_level = xlog::lvl_on;
  SHOW_TEST_HEAD("signaturematcher");
  uint8 signaturematchertestbuf[0x20];
  char* tests = "testbuf";
  size_t lp = 0;
  signaturematchertestbuf[lp] = 0x68;   ++lp;
  *(char**)&signaturematchertestbuf[lp] = tests + 1; lp += sizeof(char*);
  signaturematchertestbuf[lp] = 0x68;   ++lp;
  *(char**)&signaturematchertestbuf[lp] = tests + 2; lp += sizeof(char*);
  void* lpp = signaturematchertestbuf + lp;
  signaturematchertestbuf[lp] = 0x68;   ++lp;
  *(char**)&signaturematchertestbuf[lp] = tests; lp += sizeof(char*);
  signaturematchertestbuf[0x1F] = 0xCC;

  //signaturematcher::log_level() = xlog::lvl_on;
  rets = signaturematcher::find(
    signaturematchertestbuf,
    signaturematchertestbuf + sizeof(signaturematchertestbuf),
    "<A>68 \"testbuf\".{1, 20} CC");
  done = (lpp == rets.p);
  SHOW_TEST_RESULT(done);
  }

#endif  // _XLIB_TEST_

#endif  // _WIN32
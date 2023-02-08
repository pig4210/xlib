/**
  \file  xsig.h
  \brief 用于特征码定位。

  \version    0.0.1.230208

  \author     triones
  \date       2023-02-07

  \section history 版本记录

  - 2023-02-07 新建 xsig 。 0.1 。
*/
#ifndef _XLIB_XSIG_H_
#define _XLIB_XSIG_H_

#include <cstdint>
#include <memory>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#undef NOMINMAX
#undef WIN32_LEAN_AND_MEAN

#include "xblk.h"
#include "xswap.h"
#include "xlog.h"
#include "xbin.h"
#include "xhexbin.h"

#ifndef xslog
#define xslog xlog()
#endif
#define xsdbg if(xsig::dbglog) xslog
#define xserr xslog

class xsig {
 public:
  ~xsig() { for(auto o : lexs) delete o; }
 public:
//////////////////////////////////////////////////////////////// value 结构
  struct value {
    char        t;   //< qdwbp n（错误）。
    union {
      uint64_t  q;
      uint32_t  d;
      uint16_t  w;
      uint8_t   b;
      void*     p;
    };
  };
 private:
//////////////////////////////////////////////////////////////// Sign 类
  class Sign {
   public:
    Sign(const char* const sig) : s(sig), p(0) {}
    /// 返回当前处理的字符。
    inline char ch() const { return s[p]; }
    inline void step() { ++p; }
    /// 向后移动一个字符。
    inline operator char() const { return ch(); }
    /// 前缀自增。先向后移动一个字符，再返回当前字符。
    inline char operator++() { step(); return ch(); }
    /// 后缀自增。向后移动一个字符，返回之前位置的字符。
    inline char operator++(int) { auto c = ch(); step(); return c; }
   public:
    /// 计算当前位置行列信息，指示位置。
    inline xmsg pos() const {
      intptr_t row = 1;
      intptr_t col = 1;
      const intptr_t lp = p;
      for(intptr_t i = 0; i < lp; ++i) {
        switch(s[i]) {
          case '\n': col = 1;  ++row; break;
          case '\0': return xmsg() << "[" << row << "][" << col << "][overflow]";
          default:   ++col; break;
        }
      }
    return xmsg() << "[" << row << "][" << col << "]";
    }
   private:
    const char* s; //< 存放特征码字符串指针。
    intptr_t    p; //< 指示当前解析字符起始索引。
  };
 private:
//////////////////////////////////////////////////////////////// Range 类
  struct Range {
    using Type = intptr_t;  ///< 范围类型。
    Type  Min;  ///< 最小匹配次数。
    Type  Max;  ///< 最大匹配次数。

    /// 简单初始化。
    constexpr Range(const Type min, const Type max) : Min(min), Max(max) {}
    /// 指定固定范围。
    constexpr Range(const Type min) : Min(min), Max(min) {}
    /// 比较范围是否相同。
    bool operator==(const Range& r) const {
      return (Min == r.Min) && (Max == r.Max);
    }
    /// 范围融合。
    Range& operator+=(const Range& r) {
      const auto min = Min + r.Min;
      Min = (min < r.Min) ? INTPTR_MAX : min;

      const auto max = Max + r.Max;
      Max = (max < r.Max) ? INTPTR_MAX : max;
    
    return *this;
    }

    /// 用以标识最大范围指示值。
    inline static constexpr Type MaxType = INTPTR_MAX;
    /// 用以标识错误范围指示值。
    inline static constexpr Type ErrType = -1;
    /// 用以标识初始范围指示值。
    inline static constexpr Type InitType = -1;
  };
  /// 用以标识错误的范围。
  inline static const Range ErrRange = {-1, -1};
  /// 用以标识缺省的范围。
  inline static const Range MinRange = { 1, 1 };
  /// 用以标识空的范围。
  inline static const Range NilRange = { 0, 0 };
 private:
//////////////////////////////////////////////////////////////// 词法类
  class Lexical {
   public:
    enum Type : uint8_t {
      LT_Error,       //< 错误词法。
      LT_End,         //< end          -> \0
                      //< ws           -> [ \t\n\r]*           // 空白词法不生成类型。
                      //< note         -> #[^\n\0]*(\n|\0)     // 注释词法不生成类型。
                      //< 以下词法不单独成词。
                      //< hex          -> [0-9A-Fa-f]
                      //< range_value  -> {hex}{1,8}  |  {hex}{1,16}
                      //< range        -> ([\*\+\?])|(\{{ws}{range_value}?{ws},?{ws}{range_value}?{ws}\})
                      //< hexhex       -> ({hex}{2}) | ({hex}{4}) | ({hex}{8}) | ({hex}{16}) (\:[^\0])
      LT_String,      //< string       -> [L|l]?{ws}\'[^\0]+\'
      LT_Quote,       //< quote        -> [L|l]?{ws}\"[^\0]+\"
      LT_Dot,         //< dot          -> \.{range}?
      LT_MarkRef,     //< markreg      -> {hexhex}({ws}@{ws}L)?({ws}@{ws}R)?
                      //< refreg       -> {hexhex}({ws}${ws}[1-7]{ws}L)?({ws}${ws}[1-7]{ws}R)?
      LT_Ucbit,       //< ucbit        -> {hexhex}({ws}[\-\|\&]{ws}{hexhex})*{range}?
      LT_ConstHex,    //< consthex     -> {hexhex}{range}?
                      //< collection   -> \[\^?{hexhex}({ws}[\&\|\&]?{ws}{hexhex})*{ws}\]{range}?
      LT_Record_Addr,
      LT_Record_Offset,
      LT_Record_Qword,
      LT_Record_Dword,
      LT_Record_Word,
      LT_Record_Byte, //< record       -> \<\^?[AFQDWB]{ws}[^\n\r\0\>]*\>

    };
   public:
//////////////////////////////////////////////////////////////// 词法基类
    class Base {
     public:
      Base(Type t = LT_Error, const Range& r = {1, 1})
        : type(t), range(r), match_count(Range::InitType) {}
      virtual ~Base() {}
      virtual const char* type_name() const { return "Lexical end/noname/errtype"; }
      /// 用以指示是否有效词法。
      bool valid() const { return type != LT_Error; }
      /// 用以整合同类型，返回表示融合失败，融合成功后注意释放资源。默认不融合。
      virtual bool fusion(Base*) { return false; }
      /// 用以指示是否特征点，默认不是。
      virtual bool record() const { return false; }
      /// 指定内存范围和索引，进行匹配，匹配失败返回 false ，失败且 lp > blk.size() 时，彻底失败。
      bool match(const xblk& blk, Range::Type& lp) {
        void* pp = (uint8_t*)blk.start + lp;
        // 如果匹配达到最大，指针回退，允许继续。
        if(match_count >= range.Max) {
          xsdbg << pp << ' ' << type_name()
            << " max match " << (void*)range.Max << " back .";
          lp -= match_count;
          match_count = Range::InitType;
          return false;
        }

        if(match_count < range.Min) {
          match_count = Range::InitType;
          // 如果内存范围已经不足以进行最低匹配，则彻底失败。
          if((Range::Type)blk.size < (lp + range.Min)) {
            xsdbg << pp << ' ' << type_name()
              << " min match " << (void*)range.Min << " fail !";
            lp = Range::MaxType;
            return false;
          }
          xsdbg << pp << ' ' << type_name()
            << " min matching " << (void*)range.Min << " ...";
          // 最低匹配失败，允许回退继续。
          if(!test(xblk(pp, range.Min))) {
            xsdbg << pp << ' ' << type_name() << " min match fail, back .";
            return false;
          }
          match_mem = pp;
          match_count = range.Min;
          lp += match_count;
          return true;
        }

        // 如果内存范围已经不足以进行递进匹配，则彻底失败。
        if((Range::Type)blk.size < (lp + 1)) {
          xsdbg << pp << ' ' << type_name() << " stepping fail !";
          lp = Range::MaxType;
          return false;
        }
        xsdbg << pp << ' ' << type_name() << " stepping : " << match_count + 1;
        // 递进匹配失败，允许回退继续。
        if(!test(xblk(pp, 1))) {
          xsdbg << pp << ' ' << type_name() << " stepping fail, back .";
          lp -= match_count;
          match_count = Range::InitType;
          return false;
        }
        ++lp;
        ++match_count;
        return true; 
        }
      /// 设计防止内存不可读产生异常。
      virtual bool readable(const xblk& blk) const {
        return FALSE == IsBadReadPtr(blk.start, blk.size);
      }
      /// 用以给定内存段细节匹配，默认匹配。需要细节匹配则重载之。
      virtual bool test(const xblk& blk) const { return readable(blk); }
      /// 输出词法细节。
      virtual void to_bin(vbin&) const {}
      /// 用以输出词法。
      void create_bin(vbin& atom) const {
        atom << type << range.Min << range.Max;
        to_bin(atom);
      }
     public:
      Type        type;         //< 指示词法类型。
      Range       range;        //< 指示词法匹配内存大小。
      Range::Type match_count;  //< 指示词法在匹配过程中匹配的大小。
      void*       match_mem;    //< 用以记录匹配位置。
    };
//////////////////////////////////////////////////////////////// 词法 dot
    class Dot : public Base {
     public:
      Dot(const Range& r) : Base(LT_Dot, r) {}
      virtual const char* type_name() const { return "Lexical dot"; }
      virtual bool fusion(Base* lb) {
        if(lb->type != LT_Dot)  return false;
        range += ((Dot*)lb)->range;
        return true;
      }
    };
//////////////////////////////////////////////////////////////// 词法 record
    class RecordBase : public Base {
     public:
      RecordBase(const Type type, const Range& r, const std::string& n, const bool offset)
        : Base(type, r), name(n), isoff(offset) {}
      virtual bool record() const { return true; }
      virtual value pick_value(void* start) const = 0;
      virtual void to_bin(vbin& bin) const {
        bin << isoff << name.size() << name;
      }
     public:
      std::string name;
      bool        isoff;
    };
    class RecordAddr : public RecordBase {
     public:
      RecordAddr(const std::string& n, const bool offset)
        : RecordBase(LT_Record_Addr, NilRange, n, offset) {}
      virtual const char* type_name() const { return "Lexical record_addr"; }
      virtual value pick_value(void* start) const {
        value rv;
        rv.t = 'p';
        rv.q = 0;
        rv.p = match_mem;
        if(isoff) rv.p = (void*)((size_t)rv.p - (size_t)start);
        return rv;
      }
    };
    class RecordOffset : public RecordBase {
     public:
      RecordOffset(const std::string& n, const bool offset)
        : RecordBase(LT_Record_Offset, Range(sizeof(uint32_t)), n, offset) {}
      virtual const char* type_name() const { return "Lexical record_offset"; }
      virtual value pick_value(void* start) const {
        value rv;
        const int32_t off = *(int32_t*)match_mem;
        rv.t = 'p';
        rv.q = 0;
        rv.p = (void*)((uint8_t*)match_mem + off + sizeof(off));
        if(isoff) rv.p = (void*)((size_t)rv.p - (size_t)start);
        return rv;
      }
    };
    class RecordQword : public RecordBase {
     public:
      RecordQword(const std::string& n, const bool offset)
        : RecordBase(LT_Record_Qword, Range(sizeof(uint64_t)), n, offset) {}
      virtual const char* type_name() const { return "Lexical record_qword"; }
      virtual value pick_value(void* start) const {
        value rv;
        rv.t = 'q';
        rv.q = *(uint64_t*)match_mem;
        if(isoff) rv.q = rv.q - (uint64_t)start;
        return rv;
      }
    };
    class RecordDword : public RecordBase {
     public:
      RecordDword(const std::string& n, const bool offset)
        : RecordBase(LT_Record_Dword, Range(sizeof(uint32_t)), n, offset) {}
      virtual const char* type_name() const { return "Lexical record_dword"; }
      virtual value pick_value(void* start) const {
        value rv;
        rv.t = 'd';
        rv.q = 0;
        rv.d = *(uint32_t*)match_mem;
#pragma warning(push)
#pragma warning(disable:4302)
        if(isoff) rv.d = rv.d - (uint32_t)start;
#pragma warning(pop)
        return rv;
      }
    };
    class RecordWord : public RecordBase {
     public:
      RecordWord(const std::string& n, const bool offset)
        : RecordBase(LT_Record_Word, Range(sizeof(uint16_t)), n, offset) {}
      virtual const char* type_name() const { return "Lexical record_word"; }
      virtual value pick_value(void* start) const {
        value rv;
        rv.t = 'w';
        rv.q = 0;
        rv.w = *(uint16_t*)match_mem;
#pragma warning(push)
#pragma warning(disable:4302)
        if(isoff) rv.w = rv.w - (uint16_t)start;
#pragma warning(pop)
        return rv;
      }
    };
    class RecordByte : public RecordBase {
     public:
      RecordByte(const std::string& n, const bool offset)
        : RecordBase(LT_Record_Byte, Range(sizeof(uint8_t)), n, offset) {}
      virtual const char* type_name() const { return "Lexical record_byte"; }
      virtual value pick_value(void* start) const {
        value rv;
        rv.t = 'b';
        rv.q = 0;
        rv.b = *(uint8_t*)match_mem;
#pragma warning(push)
#pragma warning(disable:4302)
        if(isoff) rv.b = rv.b - (uint8_t)start;
#pragma warning(pop)
        return rv;
      }
    };
//////////////////////////////////////////////////////////////// 词法 string
    class String : public Base {
     public:
      String(const std::string& s) : Base(LT_String, Range(s.size())), str(s) {}
      virtual const char* type_name() const { return "Lexical string"; }
      virtual bool fusion(Base* lb) {
        if(lb->type != LT_String) return false;
        str.append(((String*)lb)->str);
        range = Range(str.size());    // 注意不是 += 。
        return true;
      }
      virtual bool test(const xblk& blk) const {
        if(!readable(blk)) return false;
        xsdbg << "    matching string :\r\n"
          << showbin(str.data(), str.size(), showcode, xmsg(), true)
          << showbin(blk.start, blk.size, showcode, xmsg() << "      ");
        if(blk.size != str.size())  return false;
        return memcmp(str.data(), blk.start, str.size()) == 0;
      }
      virtual void to_bin(vbin& bin) const { bin << str.size() << str; }
     public:
      std::string str;
    };
  };
  using Lexicals = std::vector<Lexical::Base*>;
 private:
//////////////////////////////////////////////////////////////// 词法 ws 、 hex 识别函数
  /// 匹配 ws 词法，跳过 ws 并返回 true。
  static bool match_ws(Sign& sig) {
    switch(sig.ch()) {
      case ' ': case '\t': case '\n': case '\r': sig.step(); return true;
      default: return false;
    }
  }
  /// 匹配 hex 词法，返回值 < 0 表示非此词法。
  static char match_hex(Sign& sig) {
    const auto ch = sig.ch();
    if(!std::isxdigit(ch)) return -1;
    sig.step();
    const char hex = ch & 0x0F;
    return (ch > '9') ? hex + 0x09 : hex;
  }
//////////////////////////////////////////////////////////////// 词法 range 提取函数
  /// 匹配词法 range_value ，返回值 < 0 表示非此词法。
  static Range::Type match_range_value(Sign& sig) {
    auto hex = match_hex(sig);
    if(hex < 0) return Range::ErrType;
    Range::Type r = hex;
    for(size_t i = 0; i < (2 * sizeof(Range::Type) - 1); ++i) {
      hex = match_hex(sig);
      if(hex < 0) return r;
      r = (r << 4) | hex;
    }
    return r;
  }
  /// 匹配词法 range，返回值 == ErrRange 时，匹配错误。
  static Range match_range(Sign& sig) {
    switch(sig.ch()) {
      case '*':
        xsdbg << "  match range * .";
        sig.step();
        return Range(0, Range::MaxType);
      case '+':
        xsdbg << "  match range + .";
        sig.step();
        return Range(1, Range::MaxType);
      case '?':
        xsdbg << "  match range ? .";
        sig.step();
        return Range(0, 1);
      case '{': sig.step(); break;
      default:
        xsdbg << "  default range { 1, 1 } .";
        return MinRange;
    }

    while(std::isblank(sig.ch())) sig.step();

    auto Min = match_range_value(sig);
    decltype(Min) Max = 0;

    while(std::isblank(sig.ch())) sig.step();

    constexpr char separator = ',';
    bool need_max = true;
    if(Min >= 0) {                        // 存在 N 值。
      xsdbg << "  match range N : " << (void*)Min;
      if(sig.ch() != separator) {         // 存在 N 值，但没有分隔符 {N} 。
        xsdbg << "    range mis ',' , skip M .";
        Max = Min; need_max = false;      // 没有分隔符的情况下，不能继续提取 M 值。
      }
      else {
        sig.step();                       // 存在 N 值且有分隔符的情况下，可能是 {N, } | {N, M} 。
      }
    }
    else {                                // 不存在 N 值
      xsdbg << "  range mis N .";
      if(sig.ch() != separator) {         // 不存在 N 值且无分隔符，非法 {} 。
        xserr << sig.pos() << "    range match fail !";
        return ErrRange;
      }
      sig.step();
      xsdbg << "     range has ',' , N = 0 .";
      Min = 0;                            // 不存在 N 值但有分隔符，可能是 {, } | {, M} 。
    }

    if(need_max) {
      while(std::isblank(sig.ch())) sig.step();
      Max = match_range_value(sig);
      if(Max >= 0) {                      // 存在 M 值 {N, M} | {, M} 。
        xsdbg << "  range M : " << (void*)Max;
      }
      else {                              // 不存在 M 值 {N, } | { , } 。
        xsdbg << "  range mis M, M = max .";
        Max = Range::MaxType;
      }
    }

    while(std::isblank(sig.ch())) sig.step();

    if(sig.ch() != '}') {
      xserr << sig.pos() << "  range mis '}' end/illegal char/out-max !";
      return ErrRange;
    }
    sig.step();

    if(Min == 0 && Max == 0) {
      xserr << sig.pos() << " illegal range = { 0, 0 } !";
      return ErrRange;
    }

    seqswap(Min, Max);
    xsdbg << "  make range = { " << (void*)Min << ", " << (void*)Max << " } .";
    return Range(Min, Max);
  }
//////////////////////////////////////////////////////////////// 一次词法识别
  bool make_lex(Sign& sig) {
    switch(sig.ch()) {
//////////////////////////////////////////////////////////////// 词法 end 识别逻辑
      // 一律返回 false 。
      case '\0' : {
        if(lexs.empty()) {
          xserr << sig.pos() << " : signature empty !" ;
          return false;;
        }
        lexs.push_back(new Lexical::Base(Lexical::LT_End));
        xsdbg << sig.pos() << " Matched " << (*lexs.rbegin())->type_name();
        sig.step();
        return false;
      }
//////////////////////////////////////////////////////////////// 词法 ws 识别逻辑
      // 只是跳过 ws ，一律返回 true 。
      case ' ': case '\t': case '\n': case '\r': sig.step(); return true;
//////////////////////////////////////////////////////////////// 词法 note 识别逻辑
      // 只是跳过 note ，一律返回 true 。
      case '#': {
        xsdbg << sig.pos() << " Matched Lexical note";
        sig.step();
        while(sig.ch() != '\n' && sig.ch() != '\0') sig.step();
        return true;
      }
//////////////////////////////////////////////////////////////// 词法 dot 识别逻辑
      case '.': {
        lexs.push_back(new Lexical::Dot(MinRange));
        xsdbg << sig.pos() << " Matched " << (*lexs.rbegin())->type_name();
        sig.step();
        const auto range = match_range(sig);
        if(ErrRange == range) return false;
        (*lexs.rbegin())->range = range;
        return true;
      }
//////////////////////////////////////////////////////////////// 词法 Record 识别逻辑
      case '<': {
        xsdbg << sig.pos() << " Matching record...";
        sig.step();
        const bool offset = sig.ch() == '^';
        if(offset) sig.step();
        Lexical::RecordBase* lr = nullptr;
        const std::string nul;
        switch(sig.ch()) {
          case 'A':       case 'a':
            lr = new Lexical::RecordAddr(nul, offset);  break;
          case 'F':       case 'f':
            lr = new Lexical::RecordOffset(nul, offset);  break;
          case 'Q':       case 'q':
            lr = new Lexical::RecordQword(nul, offset);  break;
          case 'D':       case 'd':
            lr = new Lexical::RecordDword(nul, offset);  break;
          case 'W':       case 'w':
            lr = new Lexical::RecordWord(nul, offset);  break;
          case 'B':       case 'b':
            lr = new Lexical::RecordByte(nul, offset);  break;
          default:
            xserr << sig.pos() << " record need [AFQDWB] !";
            return false;
        }
        sig.step();
        lexs.push_back(lr);

        while(std::isblank(sig.ch())) sig.step();

        std::string& name = lr->name;
        constexpr auto lc = '<';
        constexpr auto rc = '>';
        size_t needc = 1;
        do {
          const auto ch = sig.ch();
          switch(ch) {
            // 不允许分行 或 突然结束。
            case '\r': case '\n': case '\0':
              xserr << sig.pos() << " record need end by '>' .";
              return false;
            // 允许嵌套。
            case lc: ++needc; sig.step(); break;
            case rc: --needc; sig.step(); break;
              break;
            default: name.push_back(ch); sig.step(); break;
          }
        } while(0 != needc);

        xsdbg << "  Matched " << lr->type_name() << (offset ? '^' : ' ') << name;
        return true;
      }
//////////////////////////////////////////////////////////////// 词法 :# 识别逻辑
      case ':': {
        sig.step();
        const auto ch = sig.ch();
        if('\0' == ch) {
          xserr << sig.pos() << " :\\0 !";
          return false;
        }
        sig.step();
        lexs.push_back(new Lexical::String(std::string(1, ch)));
        return true;
      }
//////////////////////////////////////////////////////////////// 词法 'xxxx' 识别逻辑
      case '\'': {
        sig.step();
        std::string str;
        size_t needc = 1;
        do {
          const auto ch = sig.ch();
          switch(ch) {
            case '\'': --needc; sig.step(); break;
            case '\0':
              xserr << sig.pos() << " \\0 in '' !";
              return false;
            case '\\':
              str.push_back('\\');
              sig.step();
              if('\0' == sig.ch()) {
                xserr << sig.pos() << " \\\\0 in '' !";
                return false;
              }
              str.push_back(sig.ch());
              sig.step();
              break;
            default: str.push_back(ch); sig.step(); break;
          }
        } while(0 != needc);
      const auto ss = escape(str);
      if(ss.empty())  {
        xserr << sig.pos() << " '' empty !";
        return false;
      }
      xsdbg << " Match string :";
      xsdbg << showbin(ss.data(), ss.size(), showcode, xmsg() << "  ", true);
      lexs.push_back(new Lexical::String(ss));
      return true;
      }
      default:;
    }
    return false;
  }
 public:
  bool make_lexs(const char* const s) {
    Sign sig(s);
    lexs.clear();
    //size_t mlp = 0;
    xsdbg << "----------------------- lexical start -----------------------";
    while(make_lex(sig));
    return true;
  }
 private:
  Lexicals      lexs;
 public:
  inline static bool dbglog = false;  //< 指示是否输出 debug 信息。
  inline static ShowBinCode showcode = CheckBinCode<void>();
};
#undef xserr
#undef xsdbg
#undef xslog

#endif  // _XLIB_XSIG_H_
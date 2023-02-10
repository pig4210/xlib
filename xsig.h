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
#include <map>
#include <filesystem>
#include <fstream>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#undef NOMINMAX
#undef WIN32_LEAN_AND_MEAN
#endif

#include "xblk.h"
#include "xswap.h"
#include "xlog.h"
#include "xbin.h"
#include "xhexbin.h"

/*
  默认不编译 dbg 信息。
  如需要编译 dbg 信息，请添加 xsig_need_debug 宏。
  默认 dbg 信息不输出。如果需输出，请设置 xsig::dbglog = true;

  设置宏 xslog 以改变日志输出行为。
*/

#ifndef xslog
#define xslog xlog()
#endif

#ifdef xsig_need_debug
#define xsdbg if(xsig::dbglog) xslog
#else
#define xsdbg if constexpr (false) xslog
#endif
#define xserr xslog

#ifdef _WIN32
#ifdef _WIN64
#define xsig_is_x64
#endif
#else
// 非 windows 一律认为是 x64
#define xsig_is_x64
#endif

class xsig {
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
    /// 向后移动一个字符。
    inline void step() { ++p; }
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
    /// 用以标识最大范围指示值。
    inline static constexpr Type MaxType = INTPTR_MAX;
    /// 用以标识错误范围指示值。
    inline static constexpr Type ErrType = -1;
    /// 用以标识初始范围指示值。
    inline static constexpr Type InitType = -1;
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
      Min = (min < r.Min) ? MaxType : min;

      const auto max = Max + r.Max;
      Max = (max < r.Max) ? MaxType : max;

      if(Min > Max) std::swap(Min, Max);
      return *this;
    }
    /// 尝试重新组织并输出特征串。
    xmsg show() const {
      // 1 范围不输出。
      if(*this == Range(1, 1)) return xmsg();
      if(*this == Range(0, 1)) return xmsg() << '?';
      if(*this == Range(0, MaxType)) return xmsg() << '*';
      if(*this == Range(1, MaxType)) return xmsg() << '+';
      if(Min == Max) return xmsg().prt("{%tX}", Min);
      return xmsg().prt("{%tX,%tX}", Min, Max);
    }
  };
  /// 用以标识错误的范围。
  inline static const Range ErrRange = {Range::ErrType, Range::ErrType};
  /// 用以标识缺省的范围。
  inline static const Range MinRange = {1, 1};
  /// 用以标识空的范围。
  inline static const Range NilRange = {0, 0};
 private:
//////////////////////////////////////////////////////////////// 词法类
  class Lexical {
   public:
    enum Type : uint8_t {
      LT_End,         //< end          -> \0
                      //< bs           -> [ \t]
                      //< ws           -> [ \t\n\r]*           // 空白词法不生成类型。
                      //< note         -> #[^\n\0]*(\n|\0)     // 注释词法不生成类型。
                      //< 以下词法不单独成词。
                      //< hex          -> [0-9A-Fa-f]
                      //< range_value  -> {hex}{1,8}  |  {hex}{1,16}
                      //< range        -> ([\*\+\?])|(\{{ws}{range_value}?{ws},?{ws}{range_value}?{ws}\})
                      //< hexhex       -> {hex}{2}
      LT_Dot,         //< dot          -> \.{range}?
      LT_Record,      //< record       -> \<\^?[AFQDWB]{ws}[^\n\r\0\>]*\>
      LT_String,      //< string       -> [L|l]?{ws}\'[^\0]+\'
      LT_Const,       //< const        -> {hexhex}{range}?
                      //< collection   -> \[\^?{hexhex}({ws}[\&\|\&]?{ws}{hexhex})*{ws}\]{range}?
      LT_Quote,       //< quote        -> [L|l]?{ws}\"[^\0]+\"
      LT_MarkRef,     //< markreg      -> {hexhex}({ws}@{ws}L)?({ws}@{ws}R)?
                      //< refreg       -> {hexhex}({ws}${ws}[1-7]{ws}L)?({ws}${ws}[1-7]{ws}R)?
      LT_Ucbit,       //< ucbit        -> {hexhex}({ws}[\-\|\&]{ws}{hexhex})*{range}?
    };
   public:
//////////////////////////////////////////////////////////////// 词法基类
    class Base : std::enable_shared_from_this<Base> {
     public:
      Base(Type t, const Range& r = {1, 1})
        : type(t), range(r) { reset_match(); }
      /// 匹配重置。
      void reset_match() {
        match_count = Range::InitType;
        match_mem = nullptr;
      }
      virtual ~Base() {}
      /// 用以尝试 重新组织 并 输出 还原特征串。
      virtual xmsg show() const = 0;
      /// 输出词法 bin 细节。
      virtual void bin(vbin&) const = 0;
      /// 用以 整合 同类型词法。返回 false 表示融合失败。融合成功后注意释放资源。
      virtual bool fusion(const std::shared_ptr<Base>&) = 0;
      /// 用以给定内存段细节匹配。
      virtual bool test(const xblk&) const = 0;
      /**
        指定 内存范围 和 索引，进行匹配。
        匹配失败返回 false ，失败且 lp > blk.size() 时，彻底失败。

        \param blk  匹配范围。
        \param lp   当前指针。注意是引用，内部会修改其值。

        \return -2  失败，不可回退。内存范围不足继续匹配。
        \return -1  失败，但可回退。
        \return     其他返回表示 成功匹配，返回匹配字节数。（注意可能返回 0）
      */
      intptr_t match(const xblk& blk, const intptr_t& lp) {
        void* pp = (uint8_t*)blk.start + lp;
        // 如果匹配达到最大，指针回退，允许继续。
        if(match_count >= range.Max) {
          xsdbg << pp << " | `" << show() << "` max match, back.";
          return -1;
        }
        // 如果尚未匹配，则先进行最小匹配。
        if(match_count < range.Min) {
          // 如果内存范围已经不足以进行最低匹配，则彻底失败。
          if((Range::Type)blk.size < (lp + range.Min)) {
            xsdbg << pp << " | `" << show() << "` min match fail !";
            return -2;
          }
          xsdbg << pp << " | `" << show() << "` min matching...";
          // 记录匹配地址。注意应在 test 之前，因 test 可能会使用它。
          match_mem = pp; 
          // 最低匹配失败，允许回退继续。
          if(!test(xblk(pp, range.Min))) {
            xsdbg << pp << " | `" << show() << "` min match fail, back.";
            return -1;
          }
          match_count = range.Min;
          return range.Min;
        }

        // 如果内存范围已经不足以进行递进匹配，则彻底失败。
        if((Range::Type)blk.size < (lp + 1)) {
          xsdbg << pp << ' ' << show() << " stepping fail !";
          return -2;
        }
        xsdbg << pp << ' ' << show() << " stepping : " << match_count + 1;
        // 递进匹配失败，允许回退继续。
        if(!test(xblk(pp, 1))) {
          xsdbg << pp << ' ' << show() << " stepping fail, back.";
          return -1;
        }
        ++match_count;
        return 1;
        }
      /// 输出词法 bin 。
      void bins(vbin& bs) const {
        bs << type << range.Min << range.Max;
        bin(bs);
      }
     public:
      const Type  type;         //< 指示词法类型。
      Range       range;        //< 指示词法匹配内存大小。
      Range::Type match_count;  //< 指示词法在匹配过程中匹配的大小。
      void*       match_mem;    //< 用以记录匹配位置。
    };
//////////////////////////////////////////////////////////////// 词法 end
    class End : public Base {
     public:
      End() : Base(LT_End, {0, 0}) {};
      virtual xmsg show() const { return xmsg(); }
      virtual void bin(vbin&) const {}
      virtual bool fusion(const std::shared_ptr<Base>&) { return false; }
      virtual bool test(const xblk&) const { return true; }
    };
//////////////////////////////////////////////////////////////// 词法 dot
    class Dot : public Base {
     public:
      Dot(const Range& r) : Base(LT_Dot, r) {}
      virtual xmsg show() const {
#ifdef xsig_need_debug
        return xmsg() << '.' << range.show();
#else
        return xmsg();
#endif
      }
      virtual void bin(vbin&) const {}
      virtual bool fusion(const std::shared_ptr<Base>& lex) {
        if(lex->type != LT_Dot)  return false;
        range += lex->range;
        return true;
      }
      virtual bool test(const xblk&) const { return true; }
    };
//////////////////////////////////////////////////////////////// 词法 record
    class Record : public Base {
     public:
      Record(const char f, const Range& r, const std::string& n, const bool b)
        : Base(LT_Record, r), flag(f), name(n), isoff(b) {}
      virtual xmsg show() const {
#ifdef xsig_need_debug
        xmsg ss;
        ss << '<';
        if(isoff) ss << '^';
        ss << (char)std::toupper(flag);
        if(!name.empty()) ss << ' ' << name;
        ss << '>';
        return ss;
#else
        return xmsg();
#endif
      }
      virtual void bin(vbin& bs) const {
        bs << flag << isoff << name.size() << name;
      }
      virtual bool fusion(const std::shared_ptr<Base>&) { return false; }
      virtual bool test(const xblk&) const {
        // 没有需要校验的引用，直接返回 true 。
        if(!ref) return true;
        // 无视类型，直接比较。
        const auto v = pick_value(nullptr);
        const auto rv = ref->pick_value(nullptr);
        xsdbg << "check : " << name << " : " << v.q << " == " << rv.q;
        return v.q == rv.q;
        }
      value pick_value(const void* start) const {
        value rv;
        rv.q = 0;
        switch(flag) {
          case 'A': case 'a': {
            rv.t = 'p';
            rv.p = match_mem;
            if(isoff) rv.p = (void*)((size_t)rv.p - (size_t)start);
            return rv;
          }
          case 'F': case 'f': {
            rv.t = 'p';
            const int32_t off = *(int32_t*)match_mem;
            rv.p = (void*)((uint8_t*)match_mem + off + sizeof(off));
            if(isoff) rv.p = (void*)((size_t)rv.p - (size_t)start);
            return rv;
          }
          case 'Q': case 'q': {
            rv.t = 'q';
            rv.q = *(uint64_t*)match_mem;
            if(isoff) rv.q = rv.q - (uint64_t)start;
            return rv;
          }
          case 'D': case 'd': {
            rv.t = 'd';
            rv.d = *(uint32_t*)match_mem;
#ifndef xsig_is_x64
            if(isoff) rv.d = rv.d - (uint32_t)start;
#endif
            return rv;
          }
          case 'W': case 'w': {
            rv.t = 'w';
            rv.w = *(uint16_t*)match_mem;
            return rv;
          }
          case 'B': case 'b': {
            rv.t = 'b';
            rv.b = *(uint8_t*)match_mem;
            return rv;
          }
          default: rv.t = 'n'; return rv;
        }
      }
     public:
      const char              flag;
      const std::string       name;
      const bool              isoff;
      std::shared_ptr<Record> ref;
    };
//////////////////////////////////////////////////////////////// 词法 string
    class String : public Base {
     public:
      String(const std::string& s) : Base(LT_String, Range(s.size())), str(s) {}
      virtual xmsg show() const {
#ifdef xsig_need_debug
        return xmsg() << bin2hex(str, true);
#else
        return xmsg();
#endif
      }
      virtual void bin(vbin& bs) const { bs << str.size() << str; }
      virtual bool fusion(const std::shared_ptr<Base>& lex) {
        if(lex->type != LT_String) return false;
        str.append(((String*)lex.get())->str);
        range = Range(str.size());    // 注意不是 += 。
        return true;
      }
      virtual bool test(const xblk& blk) const {
        xsdbg << "    matching string : \r\n"
              << "                | " << bin2hex(str, true) << "\r\n"
              << "       " << blk.start << " | " << bin2hex(blk.start, blk.size, true);
        if(blk.size != str.size())  return false;
        return memcmp(str.data(), blk.start, str.size()) == 0;
      }
     private:
      std::string str;
    };
//////////////////////////////////////////////////////////////// 词法 const
    class Const : public Base {
     public:
      Const(const uint8_t c, const Range& range)
        : Base(LT_Const, range), hex(c) {}
      virtual xmsg show() const {
#ifdef xsig_need_debug
        return xmsg() << hex << range.show();
#else
        return xmsg();
#endif
      }
      virtual void bin(vbin& bs) const { bs << hex; }
      virtual bool fusion(const std::shared_ptr<Base>& lex) {
        if(lex->type != LT_Const) return false;
        if(hex != ((Const*)lex.get())->hex)  return false;
        range += lex->range;
        return true;
      }
      virtual bool test(const xblk& blk) const {
        xsdbg << "    matching const : \r\n"
              << "                | " << hex << "...\r\n"
              << "       " << blk.start << " | " << bin2hex(blk.start, blk.size, true);
        auto lp = (const uint8_t*)blk.start;  
        for(size_t i = 0; i < blk.size; ++i)
          if(hex != lp[i]) return false;
        return true;
      }
     public:
      const uint8_t hex;
    };
  };
  using Lexicals = std::vector<std::shared_ptr<Lexical::Base>>;
  using Blks = std::vector<xblk>;
  using Reports = std::map<std::string, value>;
 private:
//////////////////////////////////////////////////////////////// 词法 hex 识别函数
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
    for(size_t i = 1; i < (2 * sizeof(Range::Type)); ++i) {
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
        xsdbg << "  match range *";
        sig.step();
        return Range(0, Range::MaxType);
      case '+':
        xsdbg << "  match range +";
        sig.step();
        return Range(1, Range::MaxType);
      case '?':
        xsdbg << "  match range ?";
        sig.step();
        return Range(0, 1);
      case '{': sig.step(); break;
      default:
        xsdbg << "  default range { 1, 1 }";
        return {1, 1};
    }

    while(std::isblank(sig.ch())) sig.step();

    auto Min = match_range_value(sig);
    decltype(Min) Max = 0;

    while(std::isblank(sig.ch())) sig.step();

    constexpr char separator = ',';
    bool need_max = true;
    if(Min >= 0) {                        // 存在 N 值。
      xsdbg.prt("  match range N : %tX", Min);
      if(sig.ch() != separator) {         // 存在 N 值，但没有分隔符 {N} 。
        xsdbg << "    range mis ',' , skip M.";
        Max = Min; need_max = false;      // 没有分隔符的情况下，不能继续提取 M 值。
      } else {
        sig.step();                       // 存在 N 值且有分隔符的情况下，可能是 {N, } | {N, M} 。
      }
    } else {                                // 不存在 N 值
      xsdbg << "  range mis N.";
      if(sig.ch() != separator) {         // 不存在 N 值且无分隔符，非法 {} 。
        xserr << sig.pos() << "    range match fail !";
        return ErrRange;
      }
      sig.step();
      xsdbg << "     range has ',' , N = 0";
      Min = 0;                            // 不存在 N 值但有分隔符，可能是 {, } | {, M} 。
    }

    if(need_max) {
      while(std::isblank(sig.ch())) sig.step();
      Max = match_range_value(sig);
      if(Max >= 0) {                      // 存在 M 值 {N, M} | {, M} 。
        xsdbg.prt("  range M : %tX", Max);
      } else {                              // 不存在 M 值 {N, } | { , } 。
        xsdbg << "  range mis M, M = max.";
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
      xserr << sig.pos() << " illegal range = {0, 0} !";
      return ErrRange;
    }

    seqswap(Min, Max);
    xsdbg.prt("  make range = { %tX, %tX }", Min, Max);
    return Range(Min, Max);
  }
//////////////////////////////////////////////////////////////// 词法 hexhex 识别函数
  /// 返回 nullptr 表示识别失败。
  static std::shared_ptr<Lexical::Base> match_hexhex(Sign& sig) {
    const auto pos = sig.pos();

    const auto ch0 = match_hex(sig);
    if(ch0 < 0) return std::shared_ptr<Lexical::Base>();
    // hex 必须配对。
    const auto ch1 = match_hex(sig);
    if(ch1 < 0) {
      xsdbg << sig.pos() << " hexhex unpaired !";
      return std::shared_ptr<Lexical::Base>();
    }

    const uint8_t hex = ((uint8_t)ch0 << 4) | (uint8_t)ch1;
    
    xsdbg << pos << " Lexical const ";
    const auto range = match_range(sig);
    if(ErrRange == range) return nullptr;
    // TODO : 暂不支持 &|- 等操作。
    return std::make_shared<Lexical::Const>(hex, range);
  }
//////////////////////////////////////////////////////////////// 一次词法识别
  /// 一次词法识别。返回 false 表示 失败 或 结束。
  bool make_lex(Sign& sig) {
    switch(sig.ch()) {
//////////////////////////////////////////////////////////////// 词法 end 识别逻辑
      // 一律返回 false 。
      case '\0' : {
        xsdbg << sig.pos() << " Lexical end";
        sig.step();
        if(lexs.empty()) {
          xserr << sig.pos() << " : signature empty !" ;
          return false;;
        }
        lexs.push_back(std::make_shared<Lexical::End>());
        return false;
      }
//////////////////////////////////////////////////////////////// 词法 ws 识别逻辑
      // 只是跳过 ws ，一律返回 true 。
      case ' ': case '\t': case '\n': case '\r': sig.step(); return true;
//////////////////////////////////////////////////////////////// 词法 note 识别逻辑
      // 只是跳过 note ，一律返回 true 。
      case '#': {
        xsdbg << sig.pos() << " Lexical note";
        sig.step();
        while(sig.ch() != '\n' && sig.ch() != '\0') sig.step();
        return true;
      }
//////////////////////////////////////////////////////////////// 词法 dot 识别逻辑
      case '.': {
        xsdbg << sig.pos() << " Lexical dot";
        sig.step();
        auto lex = std::make_shared<Lexical::Dot>(MinRange);
        lexs.push_back(lex);
        const auto range = match_range(sig);
        if(ErrRange == range) return false;
        lex->range = range;
        return true;
      }
//////////////////////////////////////////////////////////////// 词法 Record 识别逻辑
      case '<': {
        xsdbg << sig.pos() << " Lexical record";
        sig.step();

        const bool offset = sig.ch() == '^';
        if(offset) sig.step();

        const auto type = sig.ch();
        Range range = ErrRange;
        switch(type) {
          case 'A': case 'a': range = Range(0); break;
          case 'F': case 'f': range = Range(sizeof(void*)); break;
          case 'Q': case 'q': range = Range(sizeof(uint64_t)); break;
          case 'D': case 'd': range = Range(sizeof(uint32_t));
#ifdef xsig_is_x64
            if(offset) {
              xserr << sig.pos() << " record ^D not allow !";
              return false;
            }
#endif
            break;
          case 'W': case 'w': range = Range(sizeof(uint16_t));
            if(offset) {
              xserr << sig.pos() << " record ^W not allow !";
              return false;
            }
            break;
          case 'B': case 'b': range = Range(sizeof(uint8_t));
            if(offset) {
              xserr << sig.pos() << " record ^B not allow !";
              return false;
            }
            break;
          default:
            xserr << sig.pos() << " record need [AFQDWB] !";
            return false;
        }
        sig.step();
        // 去除前缀空白符。
        while(std::isblank(sig.ch())) sig.step();

        std::string name;
        constexpr auto lc = '<';
        constexpr auto rc = '>';
        for(size_t needc = 1; 0 != needc;) {
          const auto ch = sig.ch();
          switch(ch) {
            // 不允许分行 或 突然结束。
            case '\r': case '\n': case '\0':
              xserr << sig.pos() << " record need end by '>'";
              return false;
            // 允许嵌套。
            case lc: ++needc; name.push_back(ch); sig.step(); break;
            case rc: --needc; name.push_back(ch); sig.step(); break;
            default:          name.push_back(ch); sig.step(); break;
          }
        }
        // 前面的简单逻辑令 > 总是被加入，这里删除之。
        name.pop_back();
        // 删除后缀空白。
        while(std::isblank(*name.rbegin())) name.pop_back();

        auto lex = std::make_shared<Lexical::Record>(type, range, name, offset);
        // 插入前先查询是否存在同名 record ，做引用。空名不做引用。
        if(!name.empty())
          for(const auto& x : lexs) {
            if(Lexical::LT_Record != x->type) continue;
            auto& xx = *(const Lexical::Record*)x.get();
            if(xx.name.empty()) continue;
            if(xx.name == name) lex->ref = *(std::shared_ptr<Lexical::Record>*)&x;
          }

        lexs.push_back(lex);
        xsdbg << "                " << lex->show() <<
          ((lex->ref) ? " Has ref*" : "");
        return true;
      }
//////////////////////////////////////////////////////////////// 词法 :x 识别逻辑
      case ':': {
        xsdbg << sig.pos() << " Lexical string";
        sig.step();
        const auto ch = sig.ch();
        if(!std::isgraph(ch)) {
          xserr << sig.pos() << " :# no unexpected char !";
          return false;
        }
        sig.step();
        lexs.push_back(std::make_shared<Lexical::String>(std::string(1, ch)));
        xsdbg << "                :" << ch;
        return true;
      }
//////////////////////////////////////////////////////////////// 词法 'xxxx' 识别逻辑
      case '\'': {
        xsdbg << sig.pos() << " Lexical string";
        sig.step();

        std::string str;
        for(size_t needc = 1; 0 != needc;) {
          const auto ch = sig.ch();
          switch(ch) {
            case '\'': --needc; sig.step(); break;
            case '\0': xserr << sig.pos() << " \\0 in '' !"; return false;
            case '\\':
              str.push_back('\\');
              sig.step();
              if('\0' == sig.ch()) {
                xserr << sig.pos() << " \\0 in '' !";
                return false;
              }
              str.push_back(sig.ch());
              sig.step();
              break;
            default: str.push_back(ch); sig.step(); break;
          }
        }
      const auto ss = escape(str);
      if(ss.empty()) { xserr << sig.pos() << " '' empty !"; return false; }

      xsdbg << "                " << bin2hex(str);
      lexs.push_back(std::make_shared<Lexical::String>(ss));
      return true;
      }
      default:;
    }
    // 尝试识别 hexhex 。
    auto lex = match_hexhex(sig);
    if(lex) {
      lexs.push_back(lex);
      xsdbg << "                " << lex->show();
      return true;
    }
    xserr << sig.pos() << " unknow lexcial !";
    return false;
  }
  /// 优化 lexs 。
  void optimization() {
    decltype(lexs) opts;
    // 先进行单个优化。
    while(!lexs.empty()) {
      auto it = lexs.begin();
      auto lex = *it;
      lexs.erase(it);
      switch(lex->type) {
        case Lexical::LT_Const: {
          // const 转换为 string 匹配效率更高。
          // 把最小匹配的 const 转为 string 。
          auto& range = lex->range;
          if(0 == range.Min) break;
          auto xlex = std::make_shared<Lexical::String>(
            std::string(range.Min, ((Lexical::Const*)lex.get())->hex));
          if(range.Min < range.Max) {
            opts.push_back(xlex);
            range.Max -= range.Min;
            range.Min = 0;
          } else {
            // 原 lex 被丢弃。
            lex = xlex;
          }
        }
        //TODO : 其他优化暂无。
        default:;
      }
      opts.push_back(lex);
    }

    if(opts.empty()) return;

    auto ait = opts.begin();
    auto alex = *ait;
    opts.erase(ait);
    while(!opts.empty()) {
      auto bit = opts.begin();
      auto blex = *bit;
      opts.erase(bit);

      // 如果整合成功，blex 将被丢弃。
      if(alex->fusion(blex)) continue;

      lexs.push_back(alex);
      alex = blex;
    }
    lexs.push_back(alex);
  }
 public:
  /// 返回对象的词法是否有效。
  bool valid() const {
    if(lexs.size() < 2) return false;
    return Lexical::LT_End == (*lexs.rbegin())->type;
  }
  /// 特征码串生成 特征码词法组。
  bool make_lexs(const char* const s) {
    Sign sig(s);
    lexs.clear();
    xsdbg << "---------------------------------------------------------------- lexical start";
    while(make_lex(sig));
    xsdbg << "---------------------------------------------------------------- lexical end";
    if(!valid()) return false;

    xsdbg << "---------------------------------------------------------------- optimization start";
    optimization();
    xsdbg << "---------------------------------------------------------------- optimization end";

    xsdbg << "```SIG";
    for(auto& lex : lexs) {
      xsdbg << lex->show();
    }
    xsdbg << "```";
    return valid();
  }
//////////////////////////////////////////////////////////////// match 内核
  /// 匹配内核。
  bool match_core(const xblk& blk, intptr_t& start) {
    try {
      xsdbg << "---------------------------------------------------------------- match start";
      Range fixRange(0);
      for(const auto& v : lexs) {
        fixRange += v->range;
        v->reset_match();
      }
      xsdbg << "match need : " << fixRange.Min << " - " << fixRange.Max;
      if(xblk::WholeIn != blk.check(xblk(
        (void*)((size_t)blk.start + start), fixRange.Min))) {
        xsdbg << "rest mem not enough";
        return false;
      }

      intptr_t lp = start;
      for(auto it = lexs.begin(); it != lexs.end();) {
        const auto lex = *it; // 注意非引用。
        const auto r = lex->match(blk, lp);
        // 匹配成功，继续。
        if(r >= 0) { lp += r;  ++it;  continue;  }
        // 匹配彻底失败，跳出。
        if(r != -1) {
          xsdbg << "---------------------------------------------------------------- match fail";
          return false;
        }
        // 逐步回退到未能最大匹配的特征。
        for(; it != (lexs.begin() - 1); --it) {
          const auto& x = *it;
          const auto c = x->match_count;
          if(c >= x->range.Min) {
            if(c < x->range.Max) break;
            xdbg << "back " << c;
            lp -= c;
          }
          x->reset_match();
        }

        if(it != (lexs.begin() - 1)) continue;
        // 前面所有特征都达到了最大匹配，无法回退。则 回到顶，递增继续。
        xsdbg << "reset and inc...";
        ++lp;
        start = lp;
        it = lexs.begin();
        if(xblk::WholeIn != blk.check(xblk(
          (void*)((size_t)blk.start + start), fixRange.Min))) {
          xsdbg << "rest mem not enough";
          return false;
        }
      }

      xsdbg << "---------------------------------------------------------------- match done";
      return true;
    } catch(...) {
      xserr << "---------------------------------------------------------------- match error !";
      return false;
    }
  }
  /// 指定块组，匹配特征。
  bool match(const Blks blks) {
    for(const auto& blk : blks) {
      intptr_t start = 0;
      if(match_core(blk, start)) return true;
    }
    return false;
  }
  /// 提取特征匹配结果。
  Reports report(const void* start) const {
    Reports reps;
    if(!valid()) {
      xserr << "xsig invalid, no report !";
      return reps;
    }
    if(nullptr == (*lexs.rbegin())->match_mem) {
      xserr << "xsig no matched, no report !";
      return reps;
    }
    int inoname = 0;
    for(const auto& lex : lexs) {
      if(Lexical::LT_Record != lex->type) continue;
      const auto& r = *(const Lexical::Record*)lex.get();
      auto name = r.name;
      if(name.empty()) name.assign(xmsg().prt("noname%d", inoname++).toas());
      reps.insert({name, r.pick_value(start)});
    }
    if(reps.empty()) {
      value v;
      v.t = 'p';
      v.p = (*lexs.begin())->match_mem;
      reps.insert({"noname", v});
    }
    return reps;
  }
  /// 转换为二进制。
  vbin to_bin() const {
    vbin bs;
    if(!valid()) {
      xserr << "xsig invalid, no bin !";
      return bs;
    }
    for(const auto& lex : lexs) {
      lex->bins(bs);
    }
    return bs;
  }
  /// 从二进制读取。
  bool from_bin(vbin& bs) {
    lexs.clear();
    try {
      while(!bs.empty()) {
        Lexical::Type t;
        Range range(0);

        bs >> t >> range.Min >> range.Max;

        switch(t) {
          case Lexical::LT_End: {
            lexs.push_back(std::make_shared<Lexical::End>());
            return true;
          }
          case Lexical::LT_Dot: {
            lexs.push_back(std::make_shared<Lexical::Dot>(range));
            break;
          }
          case Lexical::LT_Record: {
            char f;
            vbin n;
            bool b;
            bs >> f >> b >> n;
            lexs.push_back(std::make_shared<Lexical::Record>(
              f, range, std::string((const char*)n.data(), n.size()), b));
            break;
          }
          case Lexical::LT_String: {
            vbin s;
            bs >> s;
            lexs.push_back(std::make_shared<Lexical::String>(
              std::string((const char*)s.data(), s.size())));
            break;
          }
          case Lexical::LT_Const: {
            uint8_t hex;
            bs >> hex;
            lexs.push_back(std::make_shared<Lexical::Const>(hex, range));
            break;
          }
          default: {
            xserr << "Unknow type : " << (uint8_t)t;
            return false;
          }
        }
        xsdbg << (*lexs.rbegin())->show();
      }
      xserr << "No End !";
    } catch(...) {
      xserr << xfunexpt;
    }
    return false;
  }
 public:
  /// 指定块，检查内存可读。注意到：有些模块可读范围可能中断，导致匹配异常。
  static Blks check_blk(const xblk& blk) {
#ifndef _WIN32
    // 非 windows 暂不确定如何判断内存可读。
    return {blk};
#else
    // 内存可读直接返回。
    if(FALSE == IsBadReadPtr(blk.start, blk.size)) {
      //xsdbg << "kk : " << blk.start << " - " << blk.end;
      return {blk};
    }
    // 1 byte 都不可读，直接返回空。
    if(blk.size <= 1) return {};
    // 否则按 二分法 切片 递归 判断。
    Blks blks;

    const size_t asize = blk.size / 2;
    //xsdbg << "a>>" << blk.start << " : " << asize;
    for(const auto& v : check_blk(xblk(blk.start, asize)))
      blks.push_back(v);

    auto bsize = blk.size - asize;
    //xsdbg << "b>>" << (void*)((size_t)blk.start + asize) << " : " << bsize;
    for(const auto& v : check_blk(xblk((void*)((size_t)blk.start + asize), bsize)))
      blks.push_back(v);

    if(blks.empty()) return blks;

    // 优化，整合连续的块。
    auto opts(std::move(blks));

    auto it = opts.begin();
    auto as = (*it).start;
    auto ae = (*it).end;
    ++it;
    //xsdbg << "a " << as << " - " << ae;
    while(it != opts.end()) {
      const auto bs = (*it).start;
      const auto be = (*it).end;
      ++it;
      //xsdbg << "b " << bs << " - " << be;

      if(ae == bs) {
        ae = be;
        //xsdbg << "++ : " << as << " - " << ae;
      } else {
        //xsdbg << "c>> : " << as << " - " << ae;
        blks.push_back(xblk(as, ae));
        as = bs; ae = be;
      }
    }
    //xsdbg << "d>> : " << as << " - " << ae;
    blks.push_back(xblk(as, ae));
    return blks;
#endif
  }
  /// 读取特征码串。
  static std::vector<xsig> read_sig(const std::string& data) {
    std::vector<xsig> sigs;
    std::vector<std::string> ss;

    for(size_t p = 0; p != data.npos;) {
      const auto pp = data.find("\n/", p);
      if(data.npos == pp) {
        ss.push_back(std::string(data, p));
        break;
      }
      
      p = pp;
    }
    return sigs;
  }
  /// 读取特征码文件。
  static std::vector<xsig> read_sigs(const std::filesystem::path& path) {
    std::vector<xsig> sigs;
    std::ifstream file;
    file.open(path, std::ios_base::in | std::ios_base::binary);
    if(!file) {
      xserr << "open sig file fail !";
      return sigs;
    }
    
    file.seekg(0, std::ios_base::end);
    const size_t filelen = (size_t)file.tellg();
    if(0 == filelen) {
      xserr << "sig file empty !";
      return sigs;
    }
    file.seekg(0, std::ios_base::beg);
    std::string data;
    data.resize(filelen);
    file.read((char*)data.data(), filelen);
    file.close();

    return sigs;
  }
 private:
  Lexicals      lexs; //< 特征码词法组。
 public:
#ifdef xsig_need_debug
  inline static bool dbglog = false;  //< 指示是否输出 debug 信息。
#endif
  inline static ShowBinCode showcode = CheckBinCode<void>();
};
#undef xsig_is_x64
#undef xserr
#undef xsdbg
#undef xslog

#endif  // _XLIB_XSIG_H_
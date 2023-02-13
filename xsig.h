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
#include <regex>

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
    /// 输出 正则。
    std::string make_regex() const {
      // 1 范围不输出。
      if(*this == Range(1, 1)) return "";
      if(*this == Range(0, 1)) return "??";
      if(*this == Range(0, MaxType)) return "*?";
      if(*this == Range(1, MaxType)) return "+?";
      char buf[64];
      if(Min == Max)
        std::snprintf(buf, sizeof(buf), "{%td}?", Min);
      else
        std::snprintf(buf, sizeof(buf), "{%td,%td}?", Min, Max);
      return buf;
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
      Base(Type t) : type(t) {}
      virtual ~Base() {}
      /// 用以尝试 重新组织 并 输出 还原特征串。
      virtual xmsg show() const = 0;
      /// 输出词法 bin 细节。
      virtual void bin(vbin&) const = 0;
      /// 用以 整合 同类型词法。返回 false 表示融合失败。融合成功后注意释放资源。
      virtual bool fusion(const std::shared_ptr<Base>&) = 0;
      /// 输出转换后的正则表达式。
      virtual std::string make_regex() const = 0;
      /// 输出词法 bin 。
      void bins(vbin& bs) const { bs << type; bin(bs); }
     public:
      const Type  type;         //< 指示词法类型。
    };
//////////////////////////////////////////////////////////////// 词法 end
    class End : public Base {
     public:
      End() : Base(LT_End) {};
      End(vbin&) : Base(LT_End) {};
      virtual xmsg show() const { return xmsg(); }
      virtual void bin(vbin&) const {}
      virtual bool fusion(const std::shared_ptr<Base>&) { return false; }
      virtual std::string make_regex() const { return std::string(); }
    };
//////////////////////////////////////////////////////////////// 词法 dot
    class Dot : public Base {
     public:
      Dot(const Range& r) : Base(LT_Dot), range(r) {}
      Dot(vbin& bs) : Base(LT_Dot), range(ErrRange) {
        bs >> range.Min >> range.Max;
      }
      virtual xmsg show() const {
#ifdef xsig_need_debug
        return xmsg() << '.' << range.show();
#else
        return xmsg();
#endif
      }
      virtual void bin(vbin& bs) const {
        bs << range.Min << range.Max;
      }
      virtual bool fusion(const std::shared_ptr<Base>& lex) {
        if(lex->type != LT_Dot)  return false;
        range += ((Dot*)lex.get())->range;
        return true;
      }
      virtual std::string make_regex() const {
        return  "." + range.make_regex();
      }
     public:
      Range range;
    };
//////////////////////////////////////////////////////////////// 词法 record
    class Record : public Base {
     public:
      Record(const char f, const std::string& n, const bool b)
        : Base(LT_Record), flag(f), name(n), isoff(b) {}
      Record(vbin& bs) : Base(LT_Record) {
        vbin n;
        bs >> flag >> isoff >> n;
        name.assign((const char*)n.data(), n.size());
      }
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
      virtual std::string make_regex() const {
        switch(flag) {
          case 'A': case 'a': return "()";
          case 'F': case 'f': return "()....";
          case 'Q': case 'q': return "()........";
          case 'D': case 'd': return "()....";
          case 'W': case 'w': return "()..";
          case 'B': case 'b': return "().";
          default:            return "()";
        }
        }
      value pick_value(const void* start, const size_t pos) const {
        void* match_mem = (void*)((size_t)start + pos);
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
      char                    flag;
      std::string             name;
      bool                    isoff;
    };
//////////////////////////////////////////////////////////////// 词法 string
    class String : public Base {
     public:
      String(const std::string& s) : Base(LT_String), str(s) {}
      String(vbin& bs) : Base(LT_String) {
        vbin n;
        bs >> n;
        str.assign((const char*)n.data(), n.size());
      }
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
        return true;
      }
      virtual std::string make_regex() const {
        constexpr auto fmt = "0123456789ABCDEF";
        std::string ex;
        for(const auto& ch : str) {
          const uint8_t hex = ch;
          ex.push_back('\\');
          ex.push_back('x');
          ex.push_back(fmt[hex >> 4]);
          ex.push_back(fmt[hex & 0xF]);
        }
        return ex;
        }
     private:
      std::string str;
    };
//////////////////////////////////////////////////////////////// 词法 const
    class Const : public Base {
     public:
      Const(const uint8_t c, const Range& r)
        : Base(LT_Const), hex(c), range(r) {}
      Const(vbin& bs) : Base(LT_Const), range(ErrRange) {
        bs >> hex >> range.Min >> range.Max;
      }
      virtual xmsg show() const {
#ifdef xsig_need_debug
        return xmsg() << hex << range.show();
#else
        return xmsg();
#endif
      }
      virtual void bin(vbin& bs) const { bs << hex << range.show(); }
      virtual bool fusion(const std::shared_ptr<Base>& lex) {
        if(lex->type != LT_Const) return false;
        auto po = (Const*)lex.get();
        if(hex != po->hex)  return false;
        range += po->range;
        return true;
      }
      virtual std::string make_regex() const {
        constexpr auto fmt = "0123456789ABCDEF";
        std::string ex;
        ex.push_back('\\');
        ex.push_back('x');
        ex.push_back(fmt[hex >> 4]);
        ex.push_back(fmt[hex & 0xF]);
        return ex + range.make_regex();
      }
     public:
      uint8_t hex;
      Range   range;
    };
  };
  using Lexicals = std::vector<std::shared_ptr<Lexical::Base>>;
  using Blks = std::vector<xblk>;
  using Reports = std::map<std::string, value>;
 private:
  inline static const auto gk_separation_line =
    "---------------------------------------------------------------- ";
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
    // 其实还应该判断转换后是否超出 intprt_t ，但不好返回，也不好定位。
    // 考虑到这么写本来就离谱，这种情况就不处理了。
    return r;
  }
  /// 匹配词法 range，返回值 == ErrRange 时，匹配错误。
  static Range match_range(Sign& sig) {
    switch(sig.ch()) {
      case '*': sig.step(); return Range(0, Range::MaxType);
      case '+': sig.step(); return Range(1, Range::MaxType);
      case '?': sig.step(); return Range(0, 1);
      case '{': sig.step(); break;
      default:  return {1, 1};
    }
    // 有尝试使用正则语法匹配，但发现符合要求的正则不好实现。故这里放弃，为记。
    while(std::isblank(sig.ch())) sig.step();

    auto Min = match_range_value(sig);
    decltype(Min) Max = 0;

    while(std::isblank(sig.ch())) sig.step();

    constexpr char separator = ',';
    bool need_max = true;
    if(Min >= 0) {                        // 存在 N 值。
      //xsdbg.prt("  match range N : %tX", Min);
      if(sig.ch() != separator) {         // 存在 N 值，但没有分隔符 {N} 。
        //xsdbg << "    range mis ',' , skip M.";
        Max = Min; need_max = false;      // 没有分隔符的情况下，不能继续提取 M 值。
      } else {
        sig.step();                       // 存在 N 值且有分隔符的情况下，可能是 {N, } | {N, M} 。
      }
    } else {                                // 不存在 N 值
      //xsdbg << "  range mis N.";
      if(sig.ch() != separator) {         // 不存在 N 值且无分隔符，非法 {} 。
        xserr << sig.pos() << "    range match fail !";
        return ErrRange;
      }
      sig.step();
      //xsdbg << "     range has ',' , N = 0";
      Min = 0;                            // 不存在 N 值但有分隔符，可能是 {, } | {, M} 。
    }

    if(need_max) {
      while(std::isblank(sig.ch())) sig.step();
      Max = match_range_value(sig);
      if(Max >= 0) {                      // 存在 M 值 {N, M} | {, M} 。
        //xsdbg.prt("  range M : %tX", Max);
      } else {                              // 不存在 M 值 {N, } | { , } 。
        //xsdbg << "  range mis M, M = max.";
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
    //xsdbg.prt("  make range = { %tX, %tX }", Min, Max);
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
    
    const auto range = match_range(sig);
    if(ErrRange == range) {
      xsdbg << pos << " Lexical const ";
      return nullptr;
    }
    xsdbg << pos << " Lexical const   " << hex;
    // TODO : 暂不支持 &|- 等操作。
    return std::make_shared<Lexical::Const>(hex, range);
  }
//////////////////////////////////////////////////////////////// 一次词法识别
  /// 一次词法识别。返回 false 表示 失败 或 结束。
  bool make_lex(Sign& sig) {
    const auto pos = sig.pos();
    switch(sig.ch()) {
//////////////////////////////////////////////////////////////// 词法 end 识别逻辑
      // 一律返回 false 。
      case '\0' : {
        sig.step();
        if(lexs.empty()) {
          xserr << pos << " : signature empty !" ;
          return false;;
        }
        xsdbg << pos << " Lexical end";
        lexs.emplace_back(std::make_shared<Lexical::End>());
        return false;
      }
//////////////////////////////////////////////////////////////// 词法 ws 识别逻辑
      // 只是跳过 ws ，一律返回 true 。
      case ' ': case '\t': case '\n': case '\r': sig.step(); return true;
//////////////////////////////////////////////////////////////// 词法 note 识别逻辑
      // 只是跳过 note ，一律返回 true 。
      case '#': {
        sig.step();
        while(sig.ch() != '\n' && sig.ch() != '\0') sig.step();
        xsdbg << pos << " Lexical note";
        return true;
      }
//////////////////////////////////////////////////////////////// 词法 dot 识别逻辑
      case '.': {
        sig.step();
        const auto range = match_range(sig);
        if(ErrRange == range) return false;

        xsdbg << pos << " Lexical dot     ." << range.show();
        lexs.emplace_back(std::make_shared<Lexical::Dot>(range));
        return true;
      }
//////////////////////////////////////////////////////////////// 词法 Record 识别逻辑
      case '<': {
        sig.step();

        const bool offset = sig.ch() == '^';
        if(offset) sig.step();

        const auto t = sig.ch();
        switch(t) {
          case 'A': case 'a': break;
          case 'F': case 'f': break;
          case 'Q': case 'q': break;
          case 'D': case 'd': 
#ifndef xsig_is_x64
            break;
#endif
          case 'W': case 'w':
          case 'B': case 'b':
            if(offset) {
              xserr << pos << " record ^" << t << " not allow !";
              return false;
            }
            break;
          default:
            xserr << pos << " record need [AFQDWB] !";
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
              xserr << pos << " record need end by '>'";
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

        auto lex = std::make_shared<Lexical::Record>(t, name, offset);

        xsdbg << pos << " Lexical record  " << lex->show();
        lexs.emplace_back(lex);
        return true;
      }
//////////////////////////////////////////////////////////////// 词法 :x 识别逻辑
      case ':': {
        sig.step();
        const auto ch = sig.ch();
        if(!std::isgraph(ch)) {
          xserr << pos << " :# no unexpected char !";
          return false;
        }
        sig.step();
        xsdbg << pos << " Lexical string " << (uint8_t)ch;
        lexs.push_back(std::make_shared<Lexical::String>(std::string(1, ch)));
        return true;
      }
//////////////////////////////////////////////////////////////// 词法 'xxxx' 识别逻辑
      case '\'': {
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
      if(ss.empty()) { xserr << pos << " '' empty !"; return false; }

      xsdbg << pos << " Lexical string " << bin2hex(str);
      lexs.emplace_back(std::make_shared<Lexical::String>(ss));
      return true;
      }
      default:;
    }
    // 尝试识别 hexhex 。
    auto lex = match_hexhex(sig);
    if(lex) { lexs.emplace_back(lex); return true; }
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
          auto po = (Lexical::Const*)lex.get();
          auto& range = po->range;
          if(0 == range.Min) break;
          auto xlex = std::make_shared<Lexical::String>(
            std::string(range.Min, po->hex));
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
      opts.emplace_back(lex);
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

      lexs.emplace_back(alex);
      alex = blex;
    }
    lexs.emplace_back(alex);
  }
 public:
  /// 返回对象的词法是否有效。
  bool valid() const { return !lexs.empty(); }
  /// 特征码串生成 特征码词法组。
  bool make_lexs(const char* const s) {
    lexs.clear();
    regexs = std::regex();

    Sign sig(s);
    xsdbg << gk_separation_line << "lexical...";
    while(make_lex(sig));
    xsdbg << gk_separation_line << "lexical done.";
    if(!valid()) return false;

    xsdbg << gk_separation_line << "optimization...";
    optimization();
    xsdbg << gk_separation_line << "optimization done.";
    if(!valid()) return false;

    std::string ex;
    xsdbg << "```SIG";
    for(const auto& lex : lexs) {
      xsdbg << lex->show();
      ex.append(lex->make_regex());
    }
    xsdbg << "```";
    xsdbg << gk_separation_line << "regex ...";
    xsdbg << ex;
    regexs = std::regex(ex);
    xsdbg << gk_separation_line << "regex done.";

    return true;
  }
//////////////////////////////////////////////////////////////// match 内核
  /// 匹配内核。
  Reports match_core(const xblk& blk) {
    try {
      xsdbg << gk_separation_line << "match... " <<
        blk.start << " - " << blk.end;
      const auto& ex = regexs;
      const auto start = (const char*)blk.start;
      const auto end   = (const char*)blk.end;

      Reports reps;
      static const std::cregex_iterator itend;
      for(auto it = std::cregex_iterator(start, end, ex); it != itend; ++it) {
        xsdbg << "match @ " << (void*)((size_t)blk.start + it->position(0));

        reps.clear();
        int inoname = 0;
        size_t irecord = 0;
        bool err = false;

        for(const auto& lex : lexs) {
          if(Lexical::LT_Record != lex->type) continue;
          ++irecord;
          const auto& r = *(const Lexical::Record*)lex.get();
          const auto rv = r.pick_value(blk.start, it->position(irecord));
          auto name = r.name;
          if(name.empty())
            name.assign(xmsg().prt("noname%d", inoname++).toas());
          auto rit = reps.find(name);
          if(reps.end() == rit) {
            reps.insert({name, rv});
          } else if(rit->second.q != rv.q) {
            xserr << "  record [ " << name << " ] : "
              << rit->second.q << " != " << rv.q;
            err = true;
            break;
          }
        }
        if(!err) {
          if(reps.empty()) {
            value v;
            v.t = 'p';
            v.q = 0;
            v.p = (void*)((size_t)blk.start + it->position(0));
            reps.insert({"noname", v});
          }
          xsdbg << gk_separation_line << "match done.";
          return reps;
        }
      }

      xsdbg << gk_separation_line << "match over.";
      return Reports();
    } catch(...) {
      xserr << gk_separation_line << " match error !";
      return Reports();
    }
  }
  /// 指定块组，匹配特征。
  Reports match(const Blks blks) {
    for(const auto& blk : blks) {
      const auto reps = match_core(blk);
      if(!reps.empty()) return reps;
    }
    return Reports();
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
              f, std::string((const char*)n.data(), n.size()), b));
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
  /// 读取特征码串。要求多段特征码串，以 单行 / 分隔。
  static std::vector<std::string> read_sig(const std::string& _data) {
    std::vector<std::string> sigs;
    /*
      注意到，这里不适合用 正则表达式分割文本。

      - 加前缀 \n 是应对起始 / 的情况，可以组成 \n/ 。
        - 起始为 / 时，可使起始简单形成一个 空串。空串被忽略。
        - 起始非 / 时，\n 被加入 文本串，无影响。
      - 加后缀 \n/\n 是应对无 / 结尾的情况。
        - 结尾为 \n/ 时，使结尾简单形成一个 空串。空串被忽略。
        - 结尾非 /n/ 时，用于标记结尾，无影响。
    */
    const std::string data = "\n" + _data + "\n/\n";
    auto ds = data.begin();
    for(size_t s = 0, e = 0, p = 0; e != data.size(); p = e + 2) {
      e = data.find("\n/", p);
      if(data.npos == e) e = data.size();

      auto its = ds + s;
      auto ite = ds + e;

      // 如果不是单行 / ，则视为 sig 内容，继续。
      // 注意，因为加了后缀，itn 不可能为 end() 。
      auto itn = ite + 2; 
      if('\r' == *itn) ++itn;
      if('\n' != *itn) continue;

      s = e + 2;  // 注意设定下个起始位。

      // 前缀空白丢弃。
      for(; its != ite; ++its) {
        if(!std::isspace(*its)) break;
      }
      // 后缀空白丢弃。
      for(; ite != its; --ite) {
        if(!std::isspace(*its)) break;
      }
      // 空串丢弃。
      if(its == ite) continue;

      sigs.emplace_back(std::string(its, ite));
    }

    return sigs;
  }

  /// 读取特征码文件。
  static std::vector<xsig> read_sig_file(const std::filesystem::path& path) {
    std::vector<xsig> xsigs;
    std::ifstream file;
    file.open(path, std::ios_base::in | std::ios_base::binary);
    if(!file) {
      xserr << "open sig file fail !";
      return xsigs;
    }
    
    file.seekg(0, std::ios_base::end);
    const size_t filelen = (size_t)file.tellg();
    if(0 == filelen) {
      xserr << "sig file empty !";
      return xsigs;
    }
    file.seekg(0, std::ios_base::beg);
    std::string data;
    data.resize(filelen);
    file.read((char*)data.data(), filelen);
    file.close();
    // TODO ： 暂未处理 bin 的情况。

    if(data.size() >= 3 && "\xEF\xBB\xBF" == data.substr(0, 3)) {
      data.erase(data.begin(), data.begin() + 3);
    }

    const auto sigs = read_sig(data);
    for(const auto& sig : sigs) {
      xsig o;
      if(false == o.make_lexs(sig.data())) {
        xserr << "make_lexs error !";
        xserr << sig;
        return std::vector<xsig>();
      }
      xsigs.emplace_back(o);
    }

    return xsigs;
  }
 private:
  Lexicals      lexs;   //< 特征码词法组。
  std::regex    regexs; //< 特征码转义为正则表达式。
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
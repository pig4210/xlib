/**
  \file  xsig.h
  \brief 用于特征码定位。

  \version    0.0.1.230208

  \author     triones
  \date       2023-02-07

  \details

  - regex 中的 '.' 在 VS 中，不匹配 换行符。
    - 使用 (?:.|\r|\n) 用于匹配任意字符。
      - 注意不能使用 [.|\r|\n] ，[] 里的 . 是 '.' 符号自身，而不是正则意义的 '.' 。
      - (?:) 用于放弃捕获。
    - 建议使用 [\s\S] 来捕获任意字符，因为上面的方法在后面存在 () 空捕获时，会出现 栈溢出。
  - 大概看了 regex 的匹配细节，貌似没有更优，定长字符串也是逐字符判断。
  - regex 也不符合复杂特征匹配语法的实现。比如 & 操作，同名值判定。

  \section history 版本记录

  - 2023-02-07 新建 xsig 。 0.1 。
*/
#ifndef _XLIB_XSIG_H_
#define _XLIB_XSIG_H_

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#undef NOMINMAX
#undef WIN32_LEAN_AND_MEAN
#endif

#include "xbin.h"
#include "xblk.h"
#include "xcodecvt.h"
#include "xcompilerspecial.h"
#include "xhexbin.h"
#include "xlog.h"
#include "xmsg.h"
#include "xswap.h"
#include "xvarint.h"

/*
  默认不编译 dbg 信息。
  如需要编译 dbg 信息，请添加 xsig_need_debug 宏。
  默认 dbg 信息不输出。如果需输出，请设置 xsig::dbglog = true;

  设置宏 xslog 以改变日志输出行为。
*/

#ifndef xslog
#define xslog xlib::xlog()
#endif

#ifdef xsig_need_debug
#define xsdbg \
  if (xlib::xsig::dbglog) xslog
#else
#define xsdbg \
  if constexpr (false) xslog
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

namespace xlib {

class xsig {
 public:
  xsig() = default;
  xsig(const std::string& sig) { make_lexs(sig.data()); }
  //////////////////////////////////////////////////////////////// value 结构
  struct value {
    char        t;  //< qdwbp n（错误）。
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
    inline char operator()() const { return s[p]; }
    /// 向后移动一个字符。
    inline void operator++() { ++p; }

   public:
    /// 计算当前位置行列信息，指示位置。
    inline xmsg operator*() const {
      intptr_t row = 1;
      intptr_t col = 1;
      const intptr_t lp = p;
      for (intptr_t i = 0; i < lp; ++i) {
        switch (s[i]) {
          case '\n': col = 1;  ++row; break;
          case '\0': return xmsg() << "[" << row << "][" << col << "][overflow]";
          default  : ++col; break;
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
    using Type = intptr_t;  //< 范围类型。
    Type Min;               //< 最小匹配次数。
    Type Max;               //< 最大匹配次数。
    /// 用以标识最大范围指示值。
    static inline constexpr Type MaxType = INTPTR_MAX;
    /// 用以标识错误范围指示值。
    static inline constexpr Type ErrType = -1;
    /// 用以标识初始范围指示值。
    static inline constexpr Type InitType = -1;
    /// 故意删除默认初始化，请用 其他构造 或 {} 构造。
    Range() = delete;
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

      if (Min > Max) std::swap(Min, Max);
      return *this;
    }
    /// 尝试重新组织并输出特征串。
    xmsg sig() const {
      // 1 范围不输出。
      if (*this == Range(1, 1)) return xmsg();
      if (*this == Range(0, 1)) return xmsg() << '?';
      if (*this == Range(0, MaxType)) return xmsg() << '*';
      if (*this == Range(1, MaxType)) return xmsg() << '+';
      if (Min == Max) return xmsg().prt("{%tX}", Min);
      return xmsg().prt("{%tX,%tX}", Min, Max);
    }
  };
  /// 用以标识错误的范围。
  static inline const Range ErrRange = {Range::ErrType, Range::ErrType};

 public:
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
                      //< range        -> ([\*\+\?])|(\{{bs}{range_value}?{bs},?{bs}{range_value}?{bs}\})
                      //< hexhex       -> {hex}{2}
      LT_Dot,         //< dot          -> \.{range}?
      LT_Record,      //< record       -> \<\^?[AFQDWB]{bs}[^\n\r\0\>]*\>
                      //< const        -> {hexhex}
      LT_Hexs,        //< hexs         -> {const}+
      LT_Sets,
    };

   public:
    //////////////////////////////////////////////////////////////// 词法基类
    class Base : public std::enable_shared_from_this<Base> {
     public:
      Base(Type t, const Range& r = {1, 1}) : type(t), range(r) {
        reset_match();
      }
      virtual ~Base() {}
      /// 用以尝试 重新组织 并 输出 还原特征串。
      virtual xmsg sig() const = 0;
      /// 输出词法 bin 细节。
      virtual void bin(vbin&) const = 0;
      /// 输出词法 bin 。注意：默认不输出 range ，需在 bin 中自决。
      void bins(vbin& bs) const { bs << type; bin(bs); }
      /// 词法序列优化。
      std::shared_ptr<Base> optimizes() {
        if (!child) return optimize();
        // 如果不是最后一条词法，则需要先优化下一条词法。
        auto newo = child->optimizes();
        if (newo) child = newo;

        return optimize();
      }
      /**
        进行词法优化。
        通常是 通过分析下一条词法，整合 同类型词法。

        返回空时，无需操作。
        返回非空时，当前结点应替换成新的返回结果。
      */
      virtual std::shared_ptr<Base> optimize() {
        return std::shared_ptr<Base>();
      }
      /// 匹配重置。
      void reset_match() {
        match_count = Range::InitType;
        match_mem = nullptr;
      }
      /// 给定内存段细节匹配。
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
        void* pp = (uint8_t*)blk.begin() + lp;
        // 如果匹配达到最大，指针回退，允许继续。
        if (match_count >= range.Max) {
          xsdbg << pp << " | `" << sig() << "` max match, back.";
          return -1;
        }
        // 如果尚未匹配，则先进行最小匹配。
        if (match_count < range.Min) {
          // 如果内存范围已经不足以进行最低匹配，则彻底失败。
          if ((Range::Type)blk.size() < (lp + range.Min)) {
            xsdbg << pp << " | `" << sig() << "` min match fail !";
            return -2;
          }
          xsdbg << pp << " | `" << sig() << "` min matching...";
          // 记录匹配地址。注意应在 test 之前，因 test 可能会使用它。
          match_mem = pp;
          // 最低匹配失败，允许回退继续。
          if (!test(xblk(pp, range.Min))) {
            xsdbg << pp << " | `" << sig() << "` min match fail, back.";
            return -1;
          }
          match_count = range.Min;
          return range.Min;
        }

        // 如果内存范围已经不足以进行递进匹配，则彻底失败。
        if ((Range::Type)blk.size() < (lp + 1)) {
          xsdbg << pp << ' ' << sig() << " stepping fail !";
          return -2;
        }
        xsdbg << pp << ' ' << sig() << " stepping : " << match_count + 1;
        // 递进匹配失败，允许回退继续。
        if (!test(xblk(pp, 1))) {
          xsdbg << pp << ' ' << sig() << " stepping fail, back.";
          return -1;
        }
        ++match_count;
        return 1;
      }
      /// 添加子结点。
      void push_back(std::shared_ptr<Base>& o) {
        // 如果没有子结点，则直接添加。否则让子结点添加。
        if (child) return child->push_back(o);
        child = o;
        o->parent = shared_from_this();
      }

     public:
      const Type            type;        //< 指示词法类型。
      Range                 range;       //< 指示匹配内存大小。
      intptr_t              match_count; //< 指示在匹配过程中匹配的大小。
      void*                 match_mem;   //< 记录匹配位置。
      // 注意使用 weak_ptr 避免循环引用。
      std::weak_ptr<Base>   parent;      //< 上一条词法。
      std::shared_ptr<Base> child;       //< 下一条词法。
    };
    //////////////////////////////////////////////////////////////// 词法 end
    class End : public Base {
     public:
      End() : Base(LT_End, {0, 0}) {};
      End(vbin&) : End() {};
      virtual xmsg sig() const { return xmsg(); }
      virtual void bin(vbin&) const {}
      virtual bool test(const xblk&) const { return true; }
    };
    //////////////////////////////////////////////////////////////// 词法 dot
    class Dot : public Base {
     public:
      Dot(const Range& r) : Base(LT_Dot, r) {}
      Dot(vbin& bs) : Dot(ErrRange) { bs >> range.Min >> range.Max; }
      virtual xmsg sig() const {
#ifdef xsig_need_debug
        return xmsg() << '.' << range.sig();
#else
        return xmsg();
#endif
      }
      virtual void bin(vbin& bs) const { bs << range.Min << range.Max; }
      virtual std::shared_ptr<Base> optimize() {
        if (!child) return std::shared_ptr<Base>();
        if (child->type != LT_Dot) return std::shared_ptr<Base>();

        // 同是 . ，直接融合，无需构造新对象。
        range += child->range;
        // 注意断链。
        child = child->child;
        child->parent = shared_from_this();

        return std::shared_ptr<Base>();
      }
      virtual bool test(const xblk&) const { return true; }
    };
    //////////////////////////////////////////////////////////////// 词法 record
    class Record : public Base {
     public:
      Record(const char f, const std::string& n, const bool b)
          : Base(LT_Record, {0, 0}), flag(f), name(n), isoff(b) {
        switch (f) {
          case 'A': case 'a': range = Range(0); break;
          case 'F': case 'f': range = Range(sizeof(uint32_t)); break;
          case 'Q': case 'q': range = Range(sizeof(uint64_t)); break;
          case 'D': case 'd': range = Range(sizeof(uint32_t)); break;
          case 'W': case 'w': range = Range(sizeof(uint16_t)); break;
          case 'B': case 'b': range = Range(sizeof(uint8_t)); break;
          default:            range = Range(0); break;
        }
      }
      Record(vbin& bs) : Base(LT_Record, {0, 0}) {
        vbin n;
        bs >> flag >> isoff >> n;
        name.assign((const char*)n.data(), n.size());
        switch (flag) {
          case 'A': case 'a': range = Range(0); break;
          case 'F': case 'f': range = Range(sizeof(uint32_t)); break;
          case 'Q': case 'q': range = Range(sizeof(uint64_t)); break;
          case 'D': case 'd': range = Range(sizeof(uint32_t)); break;
          case 'W': case 'w': range = Range(sizeof(uint16_t)); break;
          case 'B': case 'b': range = Range(sizeof(uint8_t)); break;
          default:            range = Range(0); break;
        }
      }
      virtual xmsg sig() const {
#ifdef xsig_need_debug
        xmsg ss;
        ss << '<';
        if (isoff) ss << '^';
        ss << (char)std::toupper(flag);
        if (!name.empty()) ss << ' ' << name;
        ss << '>';
        return ss;
#else
        return xmsg();
#endif
      }
      virtual void bin(vbin& bs) const {
        bs << flag << isoff << name.size() << name;
      }
      virtual bool test(const xblk&) const {
        // 没有需要校验的引用，直接返回 true 。
        auto lock = ref.lock();
        if (!lock) return true;
        // 无视类型，直接比较。
        const auto& r = *(const Record*)lock.get();
        const auto v = pick_value(nullptr);
        const auto rv = r.pick_value(nullptr);
        xsdbg << "        check : " << name << " : " << v.q << " == " << rv.q;
        return v.q == rv.q;
      }
      value pick_value(const void* start) const {
        value rv;
        rv.q = 0;
        switch (flag) {
          case 'A': case 'a': {
            rv.t = 'p';
            rv.p = match_mem;
            if (isoff) rv.p = (void*)((size_t)rv.p - (size_t)start);
            return rv;
          }
          case 'F': case 'f': {
            rv.t = 'p';
            const auto off = *(int32_t*)match_mem;
            rv.p = (void*)((uint8_t*)match_mem + off + sizeof(off));
            if (isoff) rv.p = (void*)((size_t)rv.p - (size_t)start);
            return rv;
          }
          case 'Q': case 'q': {
            rv.t = 'q';
            rv.q = *(uint64_t*)match_mem;
            if (isoff) rv.q = rv.q - (uint64_t)start;
            return rv;
          }
          case 'D': case 'd': {
            rv.t = 'd';
            rv.d = *(uint32_t*)match_mem;
#ifndef xsig_is_x64
            if (isoff) rv.d = rv.d - (uint32_t)start;
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
      std::weak_ptr<Base>     ref;
    };
    //////////////////////////////////////////////////////////////// 词法 hexs
    class Hexs : public Base {
     public:
      Hexs(const std::string& s) : Base(LT_Hexs, Range(s.size())), str(s) {}
      Hexs(vbin& bs) : Base(LT_Hexs, {0, 0}) {
        vbin n;
        bs >> n;
        str.assign((const char*)n.data(), n.size());
        range = Range(str.size());
      }
      virtual xmsg sig() const {
#ifdef xsig_need_debug
        return xmsg() << bin2hex(str, true);
#else
        return xmsg();
#endif
      }
      virtual void bin(vbin& bs) const { bs << str.size() << str; }
      virtual std::shared_ptr<Base> optimize() {
        if (!child) return std::shared_ptr<Base>();
        if (child->type != LT_Hexs) return std::shared_ptr<Base>();

        const auto& o = *(const Hexs*)child.get();
        str.append(o.str);
        range = Range(str.size());

        child = child->child;
        child->parent = shared_from_this();

        return std::shared_ptr<Base>();
      }
      virtual bool test(const xblk& blk) const {
        xsdbg << "    matching string : \r\n"
              << "                | " << bin2hex(str, true) << "\r\n"
              << "       " << blk.begin() << " | "
              << bin2hex(blk.begin(), blk.size(), true);
        if (blk.size() != str.size()) return false;
        return memcmp(str.data(), blk.begin(), str.size()) == 0;
      }

     public:
      std::string str;
    };
    //////////////////////////////////////////////////////////////// 非词法 sets
    class Sets : public Base {
     public:
      Sets() : Base(LT_Sets, {0, 0}) {}
      Sets(const std::vector<std::string>&  mods,
           const std::vector<xblk>&         blks,
           const std::vector<std::string>&  cfgs)
          : Base(LT_Sets, {0, 0}), _mods(mods), _blks(blks), _cfgs(cfgs) {}
      Sets(vbin& bs) : Base(LT_Sets, {0, 0}) {
        size_t c = 0;
        vbin s;
        bs >> c;
        for (size_t i = 0; i < c; ++i ) {
          bs >> s;
          _mods.push_back(std::string((const char*)s.data(), s.size()));
        }
        bs >> c;
        for (size_t i = 0; i < c; ++i) {
          void* start = nullptr;
          void* end = nullptr;
          bs >> start >> end;
          _blks.push_back(xblk(start, end));
        }
        bs >> c;
        for (size_t i = 0; i < c; ++i) {
          bs >> s;
          _cfgs.push_back(std::string((const char*)s.data(), s.size()));
        }
      }
      virtual xmsg sig() const {
        xmsg msg;
        msg << '@';
        for (const auto& v : _mods) {
          msg << v << ',';
        }
        for (const auto& v : _blks) {
          msg << v.begin() << ',' << v.end() << ',';
        }
        for (const auto& v : _cfgs) {
          msg << v << ',';
        }
        return msg;
      }
      virtual void bin(vbin& bs) const {
        bs << _mods.size();
        for (const auto& v : _mods) {
          bs << v.size() << v;
        }
        bs << _blks.size();
        for (const auto& v : _blks) {
          bs << v.begin() << v.end();
        }
        bs << _cfgs.size();
        for (const auto& v : _cfgs) {
          bs << v.size() << v;
        }
      }
      virtual bool test(const xblk&) const { return true; }

     public:
      std::vector<std::string>  _mods;
      std::vector<xblk>         _blks;
      std::vector<std::string>  _cfgs;
    };
  };

 public:
  using Blks = std::vector<xblk>;
  using Reports = std::map<std::string, value>;

 private:
  static inline const auto gk_separation_line =
      "---------------------------------------------------------------- ";
  /// 添加一个词法。
  void add_lex(std::shared_ptr<Lexical::Base> o) {
    if (_lex) return _lex->push_back(o);
    _lex = o;
    if (o->parent.lock()) xserr << "add_lex has parent !";
  }
  //////////////////////////////////////////////////////////////// 词法 hex
  ///识别函数
  /// 匹配 hex 词法，返回值 < 0 表示非此词法。
  static inline char match_hex(Sign& sig) {
    const auto ch = sig();
    if (!std::isxdigit(ch)) return -1;
    ++sig;
    const char hex = ch & 0x0F;
    return (ch > '9') ? hex + 0x09 : hex;
  }
  //////////////////////////////////////////////////////////////// 词法 range
  ///提取函数
  /// 匹配词法 range_value ，返回值 < 0 表示非此词法。
  static inline Range::Type match_range_value(Sign& sig) {
    auto hex = match_hex(sig);
    if (hex < 0) return Range::ErrType;
    Range::Type r = hex;
    for (size_t i = 1; i < (2 * sizeof(Range::Type)); ++i) {
      hex = match_hex(sig);
      if (hex < 0) return r;
      r = (r << 4) | hex;
    }
    // 其实还应该判断转换后是否超出 intprt_t ，但不好返回，也不好定位。
    // 考虑到特征码这么写本来就离谱，这种情况就不处理了。
    return r;
  }
  /// 匹配词法 range ，返回值 == ErrRange 时，匹配错误。
  static inline Range match_range(Sign& sig) {
    switch (sig()) {
      case '*': ++sig; return {0, Range::MaxType};
      case '+': ++sig; return {1, Range::MaxType};
      case '?': ++sig; return {0, 1};
      case '{': ++sig; break;
      default:  return {1, 1};
    }
    // 有尝试使用正则语法匹配，但发现符合要求的正则不好实现。故这里放弃，为记。
    while (std::isblank(sig())) ++sig;

    auto Min = match_range_value(sig);
    if (Min < 0) {
      xserr << *sig << "    range.min lost !";
      return ErrRange;
    }
    while (std::isblank(sig())) ++sig;
    if ('}' == sig()) {
      ++sig;
      return Range(Min);
    }
    if (',' != sig()) {  // 不存在 N 值且无分隔符，非法 {} 。
      xserr << *sig << "    range need ',' !";
      return ErrRange;
    }
    ++sig;

    while (std::isblank(sig())) ++sig;

    auto Max = match_range_value(sig);
    if (Min < 0) {
      xserr << *sig << "    range.max lost !";
      return ErrRange;
    }

    while (std::isblank(sig())) ++sig;

    if (sig() != '}') {
      xserr << *sig << "  range mis '}' end/illegal char/out-max !";
      return ErrRange;
    }
    ++sig;

    if (Min == 0 && Max == 0) {
      xserr << *sig << " illegal range = {0, 0} !";
      return ErrRange;
    }

    seqswap(Min, Max);
    // xsdbg.prt("  make range = { %tX, %tX }", Min, Max);
    return Range(Min, Max);
  }
  /// 匹配设置 sets 。
  static inline std::shared_ptr<Lexical::Sets> match_sets(Sign& sig) {
    std::string data;
    while ('\0' != sig()) {
      data.push_back(sig());
      ++sig;
    }
    data.push_back(',');

    std::vector<std::string> vec;
    auto ds = data.begin();
    for (size_t s = 0, e = 0; e != data.size();) {
      e = data.find_first_of(',', s);
      if (data.npos == e) break;

      auto its = ds + s;
      auto ite = ds + e;

      ++e;
      s = e;  // 注意设定下个起始位。

      // 前缀空白丢弃。
      for (; its != ite; ++its) {
        if (!std::isspace(*its)) break;
      }
      // 后缀空白丢弃。
      for (; ite != its; --ite) {
        if (!std::isspace(*(ite - 1))) break;
      }
      // 空串丢弃。
      if (its == ite) continue;

      vec.emplace_back(std::string(its, ite));
    }

    data.clear();
    auto sets = std::make_shared<Lexical::Sets>();

    std::vector<size_t> bs;
    for (const auto& v : vec) {
      if (':' == *v.begin()) {
        sets->_cfgs.push_back(v);
        continue;
      }
      bool bhex = true;
      for (const auto ch : v) {
        if (!std::isxdigit(ch)) {
          bhex = false;
          break;
        }
      }
      if (!bhex) {
        sets->_mods.push_back(v);
        continue;
      }
      try {
#ifdef xsig_is_x64
        const auto u = std::stoull(v, 0, 16);
#else
        const auto u = std::stoul(v, 0, 16);
#endif
        bs.push_back(u);
        if (0 == (bs.size() % 2)) {
          sets->_blks.push_back(xblk((void*)*(bs.rbegin() + 1), u));
        }
      } catch (...) {
        xserr << *sig << " : " << v << " stoull error !";
        return std::shared_ptr<Lexical::Sets>();
      }
    }
    if (0 != (bs.size() % 2)) {
      xserr << *sig << " : blks no pair !";
      return std::shared_ptr<Lexical::Sets>();
    }
    return sets;
  }
  //////////////////////////////////////////////////////////////// 一次词法识别
  /// 一次词法识别。返回 false 表示 失败 或 结束。
  bool make_lex(Sign& sig) {
    const auto pos = *sig;
    uint8_t hex = sig() & 0xF;
    switch (sig()) {
      //////////////////////////////////////////////////////////////// 词法 end 识别逻辑
      // 一律返回 false 。
      case '\0': {
        ++sig;
        xsdbg << pos << " Lexical end";
        add_lex(std::make_shared<Lexical::End>());
        return false;
      }
      //////////////////////////////////////////////////////////////// 词法 ws 识别逻辑。
      // 只是跳过 ws ，一律返回 true 。
      case ' ': case '\t': case '\n': case '\r': ++sig; return true;
      case '@': {
        if (_lex) {
          xserr << pos << "@ must first character !";
          return false;
        }
        ++sig;
        const auto lex = match_sets(sig);
        if (!lex) return false;
        add_lex(lex);
        return true;
      }
      //////////////////////////////////////////////////////////////// 词法 note 识别逻辑
      // 只是跳过 note ，一律返回 true 。
      case '#': {
        ++sig;
        while (sig() != '\n' && sig() != '\0') ++sig;
        xsdbg << pos << " Lexical note";
        return true;
      }
      //////////////////////////////////////////////////////////////// 词法 dot 识别逻辑
      case '.': {
        ++sig;
        const auto range = match_range(sig);
        if (ErrRange == range) return false;

        xsdbg << pos << " Lexical dot     ." << range.sig();
        add_lex(std::make_shared<Lexical::Dot>(range));
        return true;
      }
      //////////////////////////////////////////////////////////////// 词法 Record 识别逻辑
      case '<': {
        ++sig;

        const bool offset = sig() == '^';
        if (offset) ++sig;

        const auto t = sig();
        switch (t) {
          case 'A': case 'a': break;
          case 'F': case 'f': break;
          case 'Q': case 'q': break;
          case 'D': case 'd': 
#ifndef xsig_is_x64
            break;
#endif
          case 'W': case 'w':
          case 'B': case 'b':
            if (offset) {
              xserr << pos << " record ^" << t << " not allow !";
              return false;
            }
            break;
          default:
            xserr << pos << " record need [AFQDWB] !";
            return false;
        }
        ++sig;
        // 去除前缀空白符。
        while (std::isblank(sig())) ++sig;

        std::string name;
        constexpr auto lc = '<';
        constexpr auto rc = '>';
        for (size_t needc = 1; 0 != needc;) {
          const auto c = sig();
          switch (c) {
            // 不允许分行 或 突然结束。
            case '\r': case '\n': case '\0':
              xserr << pos << " record need end by '>'";
              return false;
            // 允许嵌套。
            case lc: ++needc; name.push_back(c); ++sig; break;
            case rc: --needc; name.push_back(c); ++sig; break;
            default:          name.push_back(c); ++sig; break;
          }
        }
        // 前面的简单逻辑令 > 总是被加入，这里删除之。
        name.pop_back();
        // 删除后缀空白。
        while (std::isblank(*name.rbegin())) name.pop_back();

        auto lex = std::make_shared<Lexical::Record>(t, name, offset);

        // 插入前先查询是否存在同名 record ，做引用。空名不做引用。
        if (!name.empty())
          for (auto x = _lex; x; x = x->child) {
            if (Lexical::LT_Record != x->type) continue;
            auto& xx = *(const Lexical::Record*)x.get();
            if (xx.name.empty()) continue;
            if (xx.name == name) {
              lex->ref = x;
              break;
            }
          }

        xsdbg << pos << " Lexical record  " << lex->sig()
              << ((lex->ref.lock()) ? " Has ref*" : "");
        add_lex(lex);
        return true;
      }
      case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
      case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
        hex += 9;  // 注意这里没有 break 。
      case '0': case '1': case '2': case '3': case '4': case '5':
      case '6': case '7': case '8': case '9': {
        hex <<= 4;
        ++sig;
        const auto c = match_hex(sig);
        if (c < 0) {
          xsdbg << pos << "hexhex unpaired !";
          return false;
        }
        hex |= c;
        // TODO: 暂不支持 &|- ，也不支持范围。
        xsdbg << pos << " Lexical string  " << hex;
        add_lex(std::make_shared<Lexical::Hexs>(std::string(1, hex)));
        return true;
      }
      default:
        xserr << *sig << " unknow lexcial !";
        return false;
    }
  }

 public:
  /// 返回对象的词法是否有效。非空，且以 End 结尾。
  bool valid() const {
    if (!_lex) return false;
    // 故意直接取子结点， 而放弃 _lex 的检查。
    for (auto x = _lex->child; x; x = x->child) {
      if (Lexical::LT_End == x->type && !x->child) return true;
    }
    return false;
  }
  /// 特征码串生成 特征码词法组。
  bool make_lexs(const char* const s) {
    _lex.reset();

    Sign sig(s);
    xsdbg << gk_separation_line << "lexical...";
    while (make_lex(sig))
      ;
    xsdbg << gk_separation_line << "lexical done.";
    if (!valid()) return false;

    xsdbg << gk_separation_line << "optimization...";
    auto newo = _lex->optimizes();
    if (newo) _lex = newo;
    xsdbg << gk_separation_line << "optimization done.";
    if (!valid()) return false;

    xsdbg << "```SIG";
    for (auto lex = _lex; lex; lex = lex->child) {
      xsdbg << lex->sig();
    }
    xsdbg << "```";

    return true;
  }

 public:
  //////////////////////////////////////////////////////////////// BM 算法
  /*
    网上有不少算法经实验有 BUG 。

    只有 一个 GS 好后缀表 的那种算法。
    模式串 0000C745FC00000000E8 在匹配 FF50C745E800000000E80000C745FC00000000E8 时，将陷入死循环。
  */
  class BM {
   public:
    /// 不允许默认构造。
    BM() = delete;

   public:
    /// 用 模式串初始化对象。
    BM(const std::string& pat) : _pattern(pat) {
      const auto pattern = (const uint8_t*)_pattern.data();
      const intptr_t pattern_len = _pattern.size();

      // 计算坏字符表，坏字符表最大是 256 。
      _bad_char.reset(new intptr_t[0x100]());
      const auto bad_char = _bad_char.get();

      memset(bad_char, -1, 0x100 * sizeof(intptr_t));

      for (intptr_t i = 0; i < pattern_len; ++i) bad_char[pattern[i]] = i;

      // 计算好后缀表。好后缀表最大不超过模式串大小。
      _suffix.reset(new intptr_t[pattern_len]());
      const auto suffix = _suffix.get();
      _prefix.reset(new bool[pattern_len]());
      const auto prefix = _prefix.get();

      memset(suffix, -1, pattern_len * sizeof(intptr_t));
      memset(prefix, false, pattern_len * sizeof(bool));

      for (intptr_t i = 0; i < pattern_len - 1; ++i) {
        auto j = i;
        intptr_t k = 0;
        while (j >= 0 && pattern[j] == pattern[pattern_len - 1 - k]) {
          --j;
          ++k;
          suffix[k] = j + 1;
        }
        if (j == -1) prefix[k] = true;
      }
    }

   public:
    intptr_t operator()(const uint8_t* mem, const intptr_t size) {
      const auto pattern = (const uint8_t*)_pattern.data();
      const intptr_t pattern_len = _pattern.size();

      const auto bad_char = _bad_char.get();
      const auto suffix = _suffix.get();
      const auto prefix = _prefix.get();

      const auto match_suffix = [&](const intptr_t j) {
        const intptr_t k = pattern_len - 1 - j;
        if (suffix[k] != -1) {
          return j - suffix[k] + 1;
        }
        for (intptr_t r = j + 2; r <= pattern_len - 1; ++r) {
          if (prefix[pattern_len - r]) return r;
        }
        return pattern_len;
      };

      intptr_t mem_pos = 0;
      while (mem_pos <= size - pattern_len) {
        intptr_t pat_pos = pattern_len - 1;

        while (pat_pos >= 0 && mem[mem_pos + pat_pos] == pattern[pat_pos])
          --pat_pos;

        if (pat_pos < 0) return mem_pos;

        intptr_t x = pat_pos - bad_char[mem[mem_pos + pat_pos]];
        intptr_t y = 0;
        if (pat_pos < pattern_len - 1) y = match_suffix(pat_pos);
        if (x < 0 || y < 0)
          xsdbg << "=== " << (uint64_t)pat_pos << " " << (uint64_t)mem_pos
                << " " << x << " " << y;
        mem_pos += std::max(x, y);
      }
      return (intptr_t)-1;
    }

   public:
    const std::string _pattern;

   private:
    std::shared_ptr<intptr_t> _bad_char;
    std::shared_ptr<intptr_t> _suffix;
    std::shared_ptr<bool>     _prefix;
  };

 public:
  //////////////////////////////////////////////////////////////// match with preprocess
  /**
    加入 预处理 的匹配。

    1. 找到最长的 hexs 串。 SS
    1. 确定 SS 前 词法匹配范围最大值的总和。 LA
    1. 确定 SS 后 词法匹配范围最大值的总和。 LB
    1. BM 算法扫描 全块，匹配 SS 。 得到匹配位置。 MM
    1. 重新给出块 {MM - LA, MM + SS.size() + LB}
  */
  bool match_with_preprocess(const xblk& blk) {
    std::shared_ptr<Lexical::Base> lex;
    std::string ss;
    for (auto x = _lex; x; x = x->child) {
      if (x->type != Lexical::LT_Hexs) continue;
      auto& o = *(Lexical::Hexs*)x.get();
      if (o.str.size() <= ss.size()) continue;
      lex = x;
      ss = o.str;
    }
    xsdbg << "max string is " << bin2hex(ss, true);
    // 找不到最长 hexs 串的情况，虽然很离谱，但也处理一下。
    if (!lex) return match_core(blk);

    intptr_t LA = 0;
    for (auto x = lex->parent.lock(); x; x = x->parent.lock()) {
      const auto k = LA + x->range.Max;
      LA = (k >= LA) ? k : Range::MaxType;
    }
    xsdbg << "LA = " << (uint64_t)LA;

    intptr_t LB = 0;
    for (auto x = lex; x; x = x->child) {
      const auto k = LB + x->range.Max;
      LB = (k >= LB) ? k : Range::MaxType;
    }
    xsdbg << "LB = " << (uint64_t)LB;

    BM bm(ss);
    intptr_t lp = 0;
    while (lp < (intptr_t)blk.size()) {
      xsdbg << "start lp " << (uint64_t)lp;
      const auto MM = bm((const uint8_t*)blk.begin() + lp, blk.size() - lp);
      xsdbg << "    MM " << (uint64_t)MM;
      if (MM < 0) return false;
      auto a = (size_t)blk.begin() + lp + MM - LA;
      auto b = (size_t)blk.begin() + lp + MM + LB;
      if (match_core(xblk((const void*)a, (const void*)b))) return true;
      lp += MM + 1;
    }

    return false;
  }
  //////////////////////////////////////////////////////////////// match 内核
  /// 匹配内核。朴素匹配。
  bool match_core(const xblk& blk) {
    try {
      xsdbg << gk_separation_line << "match... " << blk.begin() << " - "
            << blk.end();

      intptr_t lp = 0;
      Range fixRange(0);
      for (auto lex = _lex; lex; lex = lex->child) {
        fixRange += lex->range;
        lex->reset_match();
      }
      xsdbg << "match need : " << fixRange.Min << " - " << fixRange.Max;
      if (xblk::WholeIn !=
          blk.check(xblk((void*)((size_t)blk.begin() + lp), fixRange.Min))) {
        xsdbg << "rest mem not enough";
        return false;
      }

      for (auto lex = _lex; lex;) {
        const auto r = lex->match(blk, lp);
        // 匹配成功，继续。
        if (r >= 0) {
          lp += r;
          lex = lex->child;
          continue;
        }
        // 匹配彻底失败，跳出。
        if (r != -1) {
          xsdbg << gk_separation_line << "match fail";
          return false;
        }
        // 逐步回退到未能最大匹配的特征。
        for (; lex; lex = lex->parent.lock()) {
          const auto c = lex->match_count;
          if (c >= lex->range.Min) {
            if (c < lex->range.Max) break;
            xsdbg << "back " << c;
            lp -= c;
          }
          lex->reset_match();
        }

        if (lex) continue;
        // 前面所有特征都达到了最大匹配，无法回退。则 回到顶，递增继续。
        xsdbg << "reset and inc...";
        ++lp;
        lex = _lex;
      }

      xsdbg << gk_separation_line << "match done";
      return true;
    } catch (...) {
      xserr << gk_separation_line << "match error !";
      return false;
    }
  }
  /// 指定块组，匹配特征。
  bool match(const Blks blks) {
    auto match_func =
        exmatch ? &xsig::match_with_preprocess : &xsig::match_core;
    for (const auto& blk : blks) {
      if ((this->*match_func)(blk)) return true;
    }
    return false;
  }
  /// 提取特征匹配结果。
  Reports report(const void* start) const {
    Reports reps;
    if (!valid()) {
      xserr << "xsig invalid, no report !";
      return reps;
    }
    int inoname = 0;
    for (auto lex = _lex; lex; lex = lex->child) {
      if (Lexical::LT_Record != lex->type) continue;
      const auto& r = *(const Lexical::Record*)lex.get();
      auto name = r.name;
      if (name.empty()) name.assign(xmsg().prt("noname%d", inoname++).toas());
      reps.insert({name, r.pick_value(start)});
    }
    if (reps.empty()) {
      value v;
      v.t = 'p';
      v.p = _lex->match_mem;
      reps.insert({"noname", v});
    }
    return reps;
  }
  /// 转换为二进制。
  vbin to_bin() const {
    vbin bs;
    if (!valid()) {
      xserr << "xsig invalid, no bin !";
      return bs;
    }
    for (auto lex = _lex; lex; lex = lex->child) {
      lex->bins(bs);
    }
    return bs;
  }
  /// 从二进制读取。
  bool from_bin(vbin& bs) {
    _lex.reset();
    std::shared_ptr<Lexical::Base> lex;
    try {
      while (!bs.empty()) {
        lex.reset();
        Lexical::Type t;
        bs >> t;

        Range range(0);
        vbin s;
        switch (t) {
          case Lexical::LT_End:
            lex = std::make_shared<Lexical::End>();
            break;
          case Lexical::LT_Dot:
            bs >> range.Min >> range.Max;
            lex = std::make_shared<Lexical::Dot>(range);
            break;
          case Lexical::LT_Record: {
            char f;
            bool b;
            bs >> f >> b >> s;
            lex = std::make_shared<Lexical::Record>(
                f, std::string((const char*)s.data(), s.size()), b);
            break;
          }
          case Lexical::LT_Hexs: {
            bs >> s;
            lex = std::make_shared<Lexical::Hexs>(
                std::string((const char*)s.data(), s.size()));
            break;
          }
          case Lexical::LT_Sets: {
            size_t c = 0;
            std::vector<std::string> mods;
            bs >> c;
            for (size_t i = 0; i < c; ++i) {
              bs >> s;
              mods.push_back(std::string((const char*)s.data(), s.size()));
            }
            std::vector<xblk> blks;
            bs >> c;
            for (size_t i = 0; i < c; ++i) {
              void* start = nullptr;
              void* end = nullptr;
              bs >> start >> end;
              blks.push_back(xblk(start, end));
            }
            std::vector<std::string> cfgs;
            bs >> c;
            for (size_t i = 0; i < c; ++i) {
              bs >> s;
              cfgs.push_back(std::string((const char*)s.data(), s.size()));
            }
            lex = std::make_shared<Lexical::Sets>(mods, blks, cfgs);
            break;
          }
          default: {
            xserr << "Unknow type : " << (uint8_t)t;
            return false;
          }
        }
        add_lex(lex);
        xsdbg << lex->sig();
        if (t == Lexical::LT_End) {
          if (lex->parent.lock()) return true;
          xserr << "bins empty !";
          return false;
        }
      }
      xserr << "No End !";
    } catch (...) {
      xserr << XTEXT("xsig::from_bin exception !");
    }
    return false;
  }
  std::shared_ptr<Lexical::Sets> get_sets() const {
    if (!_lex) return std::shared_ptr<Lexical::Sets>();
    if (Lexical::LT_Sets != _lex->type) return std::shared_ptr<Lexical::Sets>();

    const auto lex = (const Lexical::Sets*)_lex.get();

    auto ret = std::make_shared<Lexical::Sets>();
    ret->_mods = lex->_mods;
    ret->_blks = lex->_blks;
    ret->_cfgs = lex->_cfgs;

    return ret;
  }

 public:
  /// 指定块，检查内存可读。注意到：有些模块可读范围可能中断，导致匹配异常。
  static inline Blks check_blk(const xblk& blk) {
#ifndef _WIN32
    // 非 windows 暂不确定如何判断内存可读。
    return {blk};
#else
    // 内存可读直接返回。
    if (FALSE == IsBadReadPtr(blk.begin(), blk.size())) {
      // xsdbg << "kk : " << blk.begin() << " - " << blk.end();
      return {blk};
    }
    // 1 byte 都不可读，直接返回空。
    if (blk.size() <= 1) return {};
    // 否则按 二分法 切片 递归 判断。
    Blks blks;

    const size_t asize = blk.size() / 2;
    // xsdbg << "a>>" << blk.begin() << " : " << asize;
    for (const auto& v : check_blk(xblk(blk.begin(), asize))) blks.push_back(v);

    auto bsize = blk.size() - asize;
    // xsdbg << "b>>" << (void*)((size_t)blk.begin() + asize) << " : " << bsize;
    for (const auto& v :
         check_blk(xblk((void*)((size_t)blk.begin() + asize), bsize)))
      blks.push_back(v);

    if (blks.empty()) return blks;

    // 优化，整合连续的块。
    auto opts(std::move(blks));

    auto it = opts.begin();
    auto as = (*it).begin();
    auto ae = (*it).end();
    ++it;
    // xsdbg << "a " << as << " - " << ae;
    while (it != opts.end()) {
      const auto bs = (*it).begin();
      const auto be = (*it).end();
      ++it;
      // xsdbg << "b " << bs << " - " << be;

      if (ae == bs) {
        ae = be;
        // xsdbg << "++ : " << as << " - " << ae;
      } else {
        // xsdbg << "c>> : " << as << " - " << ae;
        blks.push_back(xblk(as, ae));
        as = bs;
        ae = be;
      }
    }
    // xsdbg << "d>> : " << as << " - " << ae;
    blks.push_back(xblk(as, ae));
    return blks;
#endif
  }
  /// 读取特征码串。要求多段特征码串，以 单行 / 分隔。
  static inline std::vector<std::string> read_sig(const std::string& _data) {
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
    for (size_t s = 0, e = 0, p = 0; e != data.size(); p = e + 2) {
      e = data.find("\n/", p);
      if (data.npos == e) break;

      auto its = ds + s;
      auto ite = ds + e;

      // 如果不是单行 / ，则视为 sig 内容，继续。
      // 注意，因为加了后缀，itn 不可能为 end() 。
      auto itn = ite + 2;
      if ('\r' == *itn) ++itn;
      if ('\n' != *itn) continue;

      s = e + 2;  // 注意设定下个起始位。

      // 前缀空白丢弃。
      for (; its != ite; ++its) {
        if (!std::isspace(*its)) break;
      }
      // 后缀空白丢弃。
      for (; ite != its; --ite) {
        if (!std::isspace(*(ite - 1))) break;
      }
      // 空串丢弃。
      if (its == ite) continue;

      sigs.emplace_back(std::string(its, ite));
    }

    return sigs;
  }
  /// 读取特征码文件。
  static inline std::vector<std::string> read_sig_file(
      const std::filesystem::path& path) {
    std::vector<std::string> sigs;
    std::ifstream file;
    file.open(path, std::ios_base::in | std::ios_base::binary);
    if (!file) {
      xserr << "open sig file fail !";
      return sigs;
    }

    file.seekg(0, std::ios_base::end);
    const size_t filelen = (size_t)file.tellg();
    if (0 == filelen) {
      xserr << "sig file empty !";
      return sigs;
    }
    file.seekg(0, std::ios_base::beg);
    std::string data;
    data.resize(filelen);
    file.read((char*)data.data(), filelen);
    file.close();

    if (data.size() >= 3 && "\xEF\xBB\xBF" == data.substr(0, 3)) {
      data.erase(data.begin(), data.begin() + 3);
    }

    return read_sig(data);
  }

 private:
  std::shared_ptr<Lexical::Base> _lex;  //< 特征码起始词法。是一个双向链表。
 public:
#ifdef xsig_need_debug
  static inline bool dbglog = false;  //< 指示是否输出 debug 信息。
#endif
  static inline bool exmatch = true;  //< match 函数使用 预处理。
};
#undef xsig_is_x64
#undef xserr
#undef xsdbg
#undef xslog

}  // namespace xlib

#endif  // _XLIB_XSIG_H_
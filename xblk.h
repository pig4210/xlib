/**
  \file  xblk.h
  \brief 定义了内存块比对操作的类。

  \version    2.0.1.230224

  \author     triones
  \date       2012-09-12

  \section history 版本记录

  - 2012-09-12 有封装需要，决定封装此操作。 1.0 。
  - 2016-11-15 适配 Linux g++ 。 1.1 。
  - 2019-08-01 PosDcrpt 删除前缀。 1.1.1 。
  - 2019-09-20 重构 xblk 。 2.0 。
*/
#ifndef _XLIB_XBLK_H_
#define _XLIB_XBLK_H_

#include <cstddef>
#include <cstdint>

namespace xlib {

class xblk {
 public:
  enum PosDcrpt {
    NoIn,     //< 不在块内。
    WholeIn,  //< 全部在块内。
    HeadIn,   //< 前部在块内，尾部在块外。
    TailIn,   //< 前部在块外，尾部在块内。
    SubIn,    //< 中间部分在块内。
  };

 public:
  /// 默认构造表示非法块。
  constexpr xblk() : _beg(nullptr), _end(nullptr), _size(0) {}
  /// 允许设置起始与结束位置初始化，自动识别起始与结束。
  template <typename T>
  constexpr xblk(const T* const a, const T* const b)
      : _beg (((size_t)a < (size_t)b) ? a : b),
        _end (((size_t)a > (size_t)b) ? a : b),
        _size ((size_t)_end - (size_t)_beg) {}
  /// 允许设置起始位置与大小初始化，允许 diff 为负值。
  template <typename T>
  constexpr xblk(const T* const a, const intptr_t diff = 1)
      : xblk(a, a + diff) {}
  constexpr xblk(const void* const a, const intptr_t diff = 1)
      : xblk((const char*)a, (const char*)a + diff) {}
  constexpr xblk(const void* const a, const void* const b)
      : xblk((const char*)a, (const char*)b) {}
  constexpr auto begin() const { return _beg; }
  constexpr auto end()   const { return _end; }
  constexpr auto size()  const { return _size; }
  constexpr auto data()  const { return _beg; }
  /// 判定目标块与本块的关系。
  constexpr PosDcrpt check(const xblk& blk) const {
    const char* const s  = (const char*)_beg;
    const char* const e  = (const char*)_end;
    const char* const ss = (const char*)blk.begin();
    const char* const ee = (const char*)blk.end();
    if (ss < s) {
      if (ee < s) return NoIn;
      if (ee > e) return SubIn;
      return TailIn;
    }
    if (ss > e) return NoIn;
    if (ee > e) return HeadIn;
    return WholeIn;
  }
  template <typename T>
  constexpr PosDcrpt check(const T* const a, const T* const b) const {
    return check(xblk(a, b));
  }
  template <typename T>
  constexpr PosDcrpt check(const T* const a, const intptr_t diff = 1) const {
    return check(xblk(a, diff));
  }
  constexpr PosDcrpt check(const void* const a, const intptr_t diff = 1) const {
    return check(xblk(a, diff));
  }
  constexpr PosDcrpt check(const void* const a, const void* b) const {
    return check(xblk(a, b));
  }
  /// 比较操作。
  constexpr bool operator==(const xblk& blk) const {
    return (_beg == blk.begin()) && (_end == blk.end());
  }
  constexpr bool operator!=(const xblk& blk) const { return !operator==(blk); }

 private:
  const void*   _beg;   //< 块首。
  const void*   _end;   //< 块尾。
  size_t        _size;  //< 块大小。
};

}  // namespace xlib

#endif  // _XLIB_XBLK_H_
/**
  \file  xblk.h
  \brief 定义了内存块比对操作的类。

  \version    2.0.0.190920
  \note       For All

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

#include <cstdint>
#include <cstddef>

class xblk
  {
  public:
    enum PosDcrpt
      {
      NoIn,         ///< 不在块内。
      WholeIn,      ///< 全部在块内。
      HeadIn,       ///< 前部在块内，尾部在块外。
      TailIn,       ///< 前部在块外，尾部在块内。
      SubIn,        ///< 中间部分在块内。
      };
  public:
    const void*   start;    ///< 块首。
    const void*   end;      ///< 块尾。
    const size_t  size;     ///< 块大小。
  public:
    /// 默认构造表示非法块。
    xblk():start(nullptr), end(nullptr), size(0)
      {
      }
    /// 允许设置起始与结束位置初始化，自动识别起始与结束。
    xblk(void* a, void* b):
      start(((size_t)a < (size_t)b) ? a : b),
      end(((size_t)a > (size_t)b) ? a : b),
      size((size_t)end - (size_t)start)
      {
      }
    /// 允许设置起始位置与大小初始化，允许 diff 为负值。
    xblk(void* a, const intptr_t diff):xblk(a, (void*)((intptr_t)a + diff))
      {
      }
    /// 判定目标块与本块的关系。
    PosDcrpt checkin(const xblk& blk) const
      {
      const char* const s = (const char*)start;
      const char* const e = (const char*)end;
      const char* const ss = (const char*)blk.start;
      const char* const ee = (const char*)blk.end;
      if(ss < s)
        {
        if(ee < s)  return NoIn;
        if(ee > e)  return SubIn;
        return TailIn;
        }
      if(ss > e)  return NoIn;
      if(ee > e)  return HeadIn;
      return WholeIn;
      }
    PosDcrpt checkin(void* a, void* b) const
      {
      return checkin(xblk(a, b));
      }
    PosDcrpt checkin(void* a, const intptr_t diff = 1) const
      {
      return checkin(xblk(a, diff));
      }
    /// 比较操作。
    bool operator==(const xblk& blk) const
      {
      if(start != blk.start) return false;
      if(end != blk.end)  return false;
      return true;
      }
    bool operator!=(const xblk& blk) const
      {
      return !this->operator==(blk);
      }
  };

#endif  // _XLIB_XBLK_H_
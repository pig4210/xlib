/*!
  \file  xblk.h
  \brief xblk.h定义了内存块描述比对操作的类。

  \version    2.0.1611.1515
  \note       For All

  \author     triones
  \date       2012-09-12
*/
#ifndef _XLIB_XBLK_H_
#define _XLIB_XBLK_H_

#ifndef _WIN32
#include <stdint.h>
#endif

#include "xlib_base.h"

class xblk
  {
  public:
    enum PosDcrpt
      {
      PD_NoIn,        //!< 不在块内
      PD_WholeIn,     //!< 全部在块内
      PD_HeadIn,      //!< 前部在块内，尾部在块外
      PD_TailIn,      //!< 前部在块外，尾部在块内
      PD_SubIn,       //!< 中间部分在块内
      };
  public:
    //! 默认构造表示非法块
    xblk();
    //! 允许设置起始与结束位置初始化，自动识别起始与结束
    xblk(void* starts, void* ends);
    //! 允许设置起始位置与大小初始化，允许size为负值
    xblk(void* starts, const intptr_t sizes);
    //! 返回块首
    void* start() const;
    //! 返回块尾
    void* end() const;
    //! 返回块大小
    size_t      size() const;
    //! 判定目标块与本块的关系
    PosDcrpt    checkin(void* starts, void* ends) const;
    PosDcrpt    checkin(void* starts, const intptr_t sizes = 1) const;
    PosDcrpt    checkin(const xblk& b) const;
    //! 比较操作
    bool operator==(const xblk& b) const;
    bool operator!=(const xblk& b) const;
  private:
    void*   _start;
    void*   _end;
    size_t  _size;
  };

#endif  // _XLIB_XBLK_H_
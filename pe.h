/**
  \file  pe.h
  \brief 定义了解析 PE 文件的类。

  \version    1.1.1.161013
  \note       For All

  \author     triones
  \date       2012-07-20
  
  \section history 版本记录

  - 2012-07-20 初步实现 Ring3 支持。 0.1 。
  - 2014-01-23 完成 Ring0 支持。 1.0 。
  - 2014-05-08 细节改进。 1.0.1 。
  - 2016-07-20 添加合法判定。 1.0.2 。
  - 2016-07-21 添加异常处理。 1.1 。
  - 2016-10-13 修正上一版本引入的循环递归 BUG 。 1.1.1 。
*/
#ifndef _XLIB_PE_H_
#define _XLIB_PE_H_

#ifdef _WIN32

#include "xblk.h"

class pe
  {
  public:
    pe(const HMODULE hMod = nullptr);
    pe(LPCTSTR name);
    /// 返回 DosHead ，即 hMod 。
    const IMAGE_DOS_HEADER* GetDosHead() const;
    /// 当 mod 非 PE 时，返回 nullptr 。
    const IMAGE_NT_HEADERS* GetPeHead() const;
    /// 当 mod 非 PE 时，返回 nullptr 。
    void*   EntryPoint()  const;
    /// 当 mod 非 PE 时，返回空范围。
    xblk    GetImage()    const;
    /// 当 mod 非 PE 时，返回空范围。
    xblk    GetCode()     const;
    HMODULE Module()      const;
    bool    IsPE()        const;
  private:
    HMODULE _hMod;
  };

#endif  // _WIN32

#endif  // _XLIB_PE_H_
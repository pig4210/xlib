/**
  \file  xlib.h
  \brief 用于预编译包含，也是不指定功能的整体包含。

  \author     triones
  \date       2014-01-06

  \section more 额外说明

  - \b 注意 xlib_base 需要在几个基本文件之前，否则编译重定义。
  - \b 注意 注释处 '-' 代表只有头文件， '+' 代表有对应代码文件。
  - \b 注意 文件名全部小写表示 Ring0 与 Ring3 通用。\n
            有大写字母表示只适用于 Ring3 。

  \code
    #include <xlib.h>           // 使 xlib 库所有功能可用。
    // 如果不需要包含 xlib 库所有功能，也可根据需要包含相应的头文件，如：
    #include <ws_s.h>           // 只使用 xlib 库的 ws_s 功能。
  \endcode

  \section history 版本记录

  - 2014-03-06 建立 updatelog.h 。
  - 2018-02-06 新建 HISTORY.md ，从 updatelog 中转移日志。
  - 2018-03-09 决定放弃 VS 项目，使用命令行编译。
  - 2018-03-11 放弃并移除 xlib_link 自动链接机制模块。
  - 2018-04-13 参考 <https://semver.org/lang/zh-CN/> ，重新定义版本号。
  - 2019-06-13 放弃 HISTORY.md ，所有记录信息重新分散到各个文件。
  - 2019-06-13 重新开始优化、重构，以及重制版本号。计划使用 makefile 编译。
  - 2019-06-19 完成基本改造。
*/
#ifndef _XLIB_H_
#define _XLIB_H_

#include "xlib_def.h"           //-20190613   locked        base/def
#include "xlib_base.h"          //+20190613   locked        base/base
//          xlib_def.h
//          xlib_struct.h
//          xlib_struct_ring0.h
//          xlib_struct_ring3.h
//          xlib_test.h
#include "xlib_struct_ring0.h"  //-20190613   open          base/struct
#include "xlib_struct_ring3.h"  //-20190613   open          base/struct
#include "xlib_struct.h"        //-20190613   open          base/struct

#include "xlib_test.h"          //+20190613   locked        test

// 以下文件都默认包含 xlib_base.h 。
#include "xlib_nt.h"            //+20190613   open          base/nt

#include "crc.h"                //+20190613   locked        algorithm/crc
#include "swap.h"               //-20190613   locked        memory/swap
#include "xrand.h"              //+20190613   locked        algorithm/xrand
#include "xblk.h"               //+20190801   locked        memory/xblk
//              swap.h
#include "ws_s.h"               //+20190613   locked        string/ws&s
//              xlib_nt.h
#include "ws_utf8.h"            //+20190613   locked        string/ws&utf8
//          ws_s.h
#include "xmsg.h"               //+20190613   locked        string/xmsg
//          ws_utf8.h
#include "xlog.h"               //+20190613   locked        string/xlog
//          xmsg.h
#include "hex_bin.h"            //+20190614   locked        string/hex&bin
//              xmsg.h
#include "syssnap.h"            //+20190614   to add        system_object/syssnap
//              xlib_nt.h
#include "pe.h"                 //+20190614   to add        memory/pe
//          xblk.h
//              ws_s.h
//              syssnap.h
#include "xline.h"              //-20190614   locked        container/xline
//          swap.h
#include "hook.h"               //+20190617   check         memory/hook
//              xlib_nt.h
//              xblk.h
//              xline.h
//              xmsg.h
//              hex_str.h
//              xlog.h
//              syssnap.h
#include "xevent.h"             //+20190617   locked        system_object/event
#include "caller.h"             //+20190618   locked        memory/caller
//              hook.h
//              xevent.h
#include "varint.h"             //-20190618   check         algorithm/varint
#include "vline.h"              //-20170906   check         container/vline
//          varint.h
#include "signaturematcher.h"   //+20170906   check         memory/signaturematcher
//          vline.h
//          xblk.h
//          xlog.h
//              hex_bin.h
//              pe.h
//              syssnap.h
//              swap.h





#include "aes.h"                //+20170724   locked        algorithm/aes
#include "des.h"                //+20170725   locked        algorithm/des
#include "md5.h"                //+20171117   locked        algorithm/md5
//              swap.h
#include "tea.h"                //+20170602   locked        algorithm/tea
//              xrand.h
#include "singleton.h"          //-20121112   locked        container/singleton
#include "xSock.h"              //+20121113   close         sock/xsock
//          xWSA.h
//          xline.h
//          xlog.h

#endif  // _XLIB_H_
/*!
  \file  xlib.h
  \brief 用于预编译包含，也是不指定功能的整体包含。

  \section more 额外说明

  - \b 注意 xlib_base需要在几个基本文件之前，否则编译重定义
  - \b 注意 注释处'-'代表只有头文件，'+'代表有对应代码文件
  - \b 注意 文件名全部小写表示Ring0与Ring3通用\n
            有大写字母表示目前只适用于Ring3

  \code
    #include <xlib.h>           //使xlib库所有功能可用
    //如果不需要包含xlib库所有功能，也可根据需要包含相应的头文件，如：
    #include <ws_s.h>           //只使用xlib库的ws_s功能
  \endcode

  \author     triones
  \date       2014-01-06
*/
#ifndef _XLIB_H_
#define _XLIB_H_

#include "xlib_def.h"           //-20161114   locked        base/def
#include "xlib_base.h"          //+20161114   locked        base/base
//          xlib_def.h
//          xlib_link.h
//          xlib_struct.h
//          xlib_struct_ring0.h
//          xlib_struct_ring3.h
//          xlib_test.h
#include "xlib_struct_ring0.h"  //-20170904   open          base/struct
#include "xlib_struct_ring3.h"  //-20161114   open          base/struct
#include "xlib_struct.h"        //-20161114   open          base/struct

#include "xlib_test.h"          //+20161114   locked        test

//以下文件都默认包含xlib_base.h
#include "xlib_nt.h"            //+20161114   open          base/nt

#include "aes.h"                //+20170724   locked        algorithm/aes
#include "crc.h"                //+20170724   locked        algorithm/crc
#include "des.h"                //+20170725   locked        algorithm/des
#include "swap.h"               //-20161114   locked        memory/swap
#include "md5.h"                //+20171117   locked        algorithm/md5
//              swap.h
#include "xrand.h"              //+20161114   locked        algorithm/xrand
#include "tea.h"                //+20170602   locked        algorithm/tea
//              xrand.h
#include "varint.h"             //-20170911   check         algorithm/varint
#include "singleton.h"          //-20121112   locked        container/singleton
#include "xline.h"              //-20170906   locked        container/xline
//          swap.h
#include "vline.h"              //-20170906   check         container/vline
#include "xblk.h"               //+20161115   locked        memory/xblk
//              swap.h
#include "ws_s.h"               //+20161223   locked        string/ws&s
//              xlib_nt.h
#include "ws_utf8.h"            //+20170103   locked        string/ws&utf8
//          ws_s.h
#include "xmsg.h"               //+20161115   locked        string/xmsg
//          ws_utf8.h
#include "xlog.h"               //+20161115   locked        string/xlog
//          xmsg.h
#include "hex_bin.h"            //+20180207   locked        string/hex&bin
//          xmsg.h
//              ws_s.h
//              ws_utf8.h
#include "syssnap.h"            //+20140219   to add        system_object/syssnap
//          xline.h
//              xlib_nt.h
#include "pe.h"                 //+20160721   to add        memory/pe
//          xblk.h
//              ws_s.h
//              syssnap.h
#include "xSock.h"              //+20121113   close         sock/xsock
//          xWSA.h
//          xline.h
//          xlog.h
#include "hook.h"               //+20170727   check         memory/hook
//              xlib_nt.h
//              xblk.h
//              xline.h
//              xmsg.h
//              hex_str.h
//              xlog.h
//              syssnap.h
#include "xevent.h"             //+20130301   locked        system_object/event
#include "caller.h"             //+20150721   locked        memory/caller
//              hook.h
//              xevent.h
#include "signaturematcher.h"   //+20170906   check         memory/signaturematcher
//          xline.h
//          xblk.h
//              xlog.h
//              hex_str.h
//              ws_s.h

#endif  // _XLIB_H_
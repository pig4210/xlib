# 这个 Makefile 用于 编译 xlib 的文件包含 。

xlib_test.o         : xlib_test.cc
xcrc.o              : xcrc.cc               xcrc.h
xswap.o             : xswap.cc              xswap.h
xrand.o             : xrand.cc              xrand.h
xblk.o              : xblk.cc               xblk.h
as_ws_u8.o          : as_ws_u8.cc           as_ws_u8.h
xmsg.o              : xmsg.cc               xmsg.h as_ws_u8.h
xlog.o              : xlog.cc               xlog.h xmsg.h as_ws_u8.h
hexbin.o            : hexbin.cc             hexbin.h xswap.h as_ws_u8.h
varint.o            : varint.cc             varint.h
xbin.o              : xbin.cc               xbin.h xswap.h varint.h

%.o : xlib_test.h

OBJS    := xlib_test.o \
           xcrc.o \
           xswap.o \
           xrand.o \
           xblk.o \
           as_ws_u8.o \
           xmsg.o \
           xlog.o \
           hexbin.o \
           varint.o \
           xbin.o

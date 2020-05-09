# 这个 Makefile 用于 编译 xlib 的文件包含 。

xlib_test.o         : 
xcrc.o              : xcrc.h
xswap.o             : xswap.h
xrand.o             : xrand.h
xblk.o              : xblk.h
as_ws_u8.o          : as_ws_u8.h
xmsg.o              : xmsg.h as_ws_u8.h
xlog.o              : xlog.h xmsg.h as_ws_u8.h
hexbin.o            : hexbin.h xswap.h as_ws_u8.h
varint.o            : varint.h
xbin.o              : xbin.h xswap.h varint.h

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

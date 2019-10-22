# 这个 Makefile 用于 编译 xlib 的文件包含 。

xlib_test.o         : xlib_test.cc
xcrc.o              : xcrc.cc               xcrc.h
xswap.o             : xswap.cc              xswap.h
xrand.o             : xrand.cc              xrand.h
xblk.o              : xblk.cc               xblk.h
xcodecvt.o          : xcodecvt.cc           xcodecvt.h
xmsg.o              : xmsg.cc               xmsg.h xcodecvt.h
xlog.o              : xlog.cc               xlog.h xmsg.h xcodecvt.h
hexbin.o            : hexbin.cc             hexbin.h xswap.h xcodecvt.h
varint.o            : varint.cc             varint.h
xbin.o              : xbin.cc               xbin.h xswap.h

%.o : xlib_test.h

OBJS    := xlib_test.o \
           xcrc.o \
           xswap.o \
           xrand.o \
           xblk.o \
           xcodecvt.o \
           xmsg.o \
           xlog.o \
           hexbin.o \
           varint.o \
           xbin.o

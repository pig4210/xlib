# 这个 Makefile 用于 编译 xlib 的文件包含 。

xlib_test.o         : xlib_test.cc          xlib_test.h
xcrc.o              : xcrc.cc               xcrc.h xlib_test.h
xswap.o             : xswap.cc              xswap.h xlib_test.h
xrand.o             : xrand.cc              xrand.h xlib_test.h
xblk.o              : xblk.cc               xblk.h xlib_test.h
xcodecvt.o          : xcodecvt.cc           xcodecvt.h xlib_test.h
xmsg.o              : xmsg.cc               xmsg.h xcodecvt.h xlib_test.h
xlog.o              : xlog.cc               xlog.h xmsg.h xcodecvt.h xlib_test.h
hexbin.o            : hexbin.cc             hexbin.h xcodecvt.h xlib_test.h

OBJS    := xlib_test.o \
           xcrc.o \
           xswap.o \
           xrand.o \
           xblk.o \
           xcodecvt.o \
           xmsg.o \
		   xlog.o \
           hexbin.o

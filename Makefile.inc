# 这个 Makefile 用于 编译 xlib 的文件包含 。

xlib_test.o         : 
xcrc.o              : xcrc.h
xswap.o             : xswap.h
xrand.o             : xcompilerspecial.h xrand.h
xblk.o              : xblk.h
xcodecvt.o          : xcompilerspecial.h xcodecvt.h
xmsg.o              : xcompilerspecial.h xcodecvt.h xmsg.h
xlog.o              : xcompilerspecial.h xcodecvt.h xmsg.h xlog.h
xhexbin.o           : xswap.h xcompilerspecial.h xcodecvt.h xmsg.h xhexbin.h
xvarint.o           : xvarint.h
xbin.o              : xswap.h xvarint.h xbin.h

%.o : xlib_test.h

OBJS := xlib_test.o \
        xcrc.o \
        xswap.o \
        xrand.o \
        xblk.o \
        xcodecvt.o \
        xmsg.o \
        xlog.o \
        xhexbin.o \
        xvarint.o \
        xbin.o
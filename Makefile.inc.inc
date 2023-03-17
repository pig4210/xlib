# 这个 Makefile 用于 编译 xlib 的文件包含 。

xlib_test.o         : xlib_test.h
xcrc.o              : xlib_test.h xcrc.h
xswap.o             : xlib_test.h xswap.h
xrand.o             : xlib_test.h xrand.h
xblk.o              : xlib_test.h xblk.h
xcodecvt.o          : xlib_test.h xcodecvt_win.h xcodecvt.h
xmsg.o              : xlib_test.h xcodecvt_win.h xcodecvt.h xmsg.h
xlog.o              : xlib_test.h xcodecvt_win.h xcodecvt.h xmsg.h xlog.h
xhexbin.o           : xlib_test.h xswap.h xcodecvt_win.h xcodecvt.h xmsg.h xlog.h xhexbin.h
xvarint.o           : xlib_test.h xvarint.h
xbin.o              : xlib_test.h xswap.h xvarint.h xbin.h
xxstring.o          : xlib_test.h xcodecvt_win.h xcodecvt.h xmsg.h xxstring.h
xhook.o             : xlib_test.h xhook.h
xsig.o              : xlib_test.h xswap.h xblk.h xcodecvt_win.h xcodecvt.h xmsg.h xlog.h xhexbin.h xvarint.h xbin.h xsig.h

OBJS := xlib_test.o     \
        xcrc.o          \
        xswap.o         \
        xrand.o         \
        xblk.o          \
        xcodecvt.o      \
        xmsg.o          \
        xlog.o          \
        xhexbin.o       \
        xvarint.o       \
        xbin.o          \
        xxstring.o      \
        xhook.o         \
        xsig.o
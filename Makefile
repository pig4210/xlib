# 这个 Makefile 用于使用 GNU make 在 Windows 编译 xlib 。

# 如果只是单纯的 clean ，则无需 环境 和 路径
ifneq "$(MAKECMDGOALS)" "clean"
  ifeq "$(filter x64 x86,$(Platform))" ""
	$(error Need VS Environment)
  endif

  ifeq "$(SRCPATH)" ""
    $(error Need SRCPATH)
  endif
endif


.PHONY : all
all : test lib
	@echo make done.


XLIB_BASE_H	:= xlib_def.h \
			   xlib_struct.h \
			   xlib_struct_ring0.h \
			   xlib_struct_ring3.h \
			   xlib_base.h

xlib_base.o 		: xlib_base.cc
xlib_test.o 		: xlib_test.cc 			xlib_test.h
xlib_nt.o			: xlib_nt.cc 			xlib_nt.h
crc.o				: crc.cc 				crc.h
swap.o				: swap.cc 				swap.h
xrand.o				: xrand.cc 				xrand.h
xblk.o				: xblk.cc				xblk.h swap.h
ws_s.o				: ws_s.cc				ws_s.h xlib_nt.h
ws_utf8.o			: ws_utf8.cc			ws_utf8.h ws_s.h
xmsg.o				: xmsg.cc				xmsg.h ws_s.h ws_utf8.h
xlog.o				: xlog.cc				xlog.h ws_s.h ws_utf8.h xmsg.h
hex_bin.o			: hex_bin.cc			hex_bin.h ws_s.h ws_utf8.h xmsg.h
syssnap.o			: syssnap.cc			syssnap.h xlib_nt.h
pe.o				: pe.cc					pe.h xblk.h ws_s.h syssnap.h
xline.o				: xline.cc				xline.h swap.h
hook.o				: hook.cc				hook.h xlib_nt.h swap.h xblk.h xline.h ws_s.h ws_utf8.h xmsg.h hex_bin.h xlog.h syssnap.h
xevent.o			: xevent.cc				xevent.h
caller.o			: caller.cc				caller.h hook.h xevent.h
varint.o			: varint.cc				varint.h
vline.o				: vline.cc				vline.h varint.h
signaturematcher.o  : signaturematcher.cc   signaturematcher.h varint.h vline.h xblk.h ws_s.h ws_utf8.h xmsg.h xlog.h hex_bin.h pe.h syssnap.h swap.h

xSock.o				: xSock.cc				xSock.h	swap.h xline.h ws_s.h ws_utf8.h xmsg.h xlog.h

LIB_O	:= xlib_base.o \
		   xlib_nt.o \
		   crc.o \
		   swap.o \
		   xrand.o \
		   xblk.o \
		   ws_s.o \
		   ws_utf8.o \
		   xmsg.o \
		   xlog.o \
		   hex_bin.o \
		   syssnap.o \
		   pe.o \
		   xline.o \
		   hook.o \
		   xevent.o \
		   caller.o \
		   varint.o \
		   vline.o \
		   signaturematcher.o \
		   xSock.o

DESTPATH	:= $(Platform)

CC 			:= cl.exe
LINK		:= link.exe
AR			:= lib.exe

######## CFLAGS
CFLAGS		= /c /MP /GS- /Qpar /GL /analyze- /W4 /Gy /Zc:wchar_t /Zi /Gm- /Ox /Zc:inline /fp:precise /DWIN32 /DNDEBUG /D_UNICODE /DUNICODE /fp:except- /errorReport:none /GF /WX /Zc:forScope /GR- /Gd /Oy /Oi /MT /EHa /nologo
CFLAGS		+= /I"$(SRCPATH)"
CFLAGS		+= /Fd"$(DESTPATH)/"
CFLAGS		+= /D_LIB

CFLAGS		+= $(MyCFLAGS)


ifeq "$(Platform)" "x86"
CFLAGS		+= /D_USING_V110_SDK71_
endif

######## ARFLAGS
ARFLAGS		= /LTCG /ERRORREPORT:NONE /NOLOGO /MACHINE:$(Platform)
ARFLAGS		+= /LIBPATH:"$(DESTPATH)"

######## LDFLAGS
LDFLAGS		= /MANIFEST:NO /LTCG /NXCOMPAT /DYNAMICBASE "kernel32.lib" "user32.lib" "gdi32.lib" "winspool.lib" "comdlg32.lib" "advapi32.lib" "shell32.lib" "ole32.lib" "oleaut32.lib" "uuid.lib" "odbc32.lib" "odbccp32.lib" /OPT:REF /INCREMENTAL:NO /OPT:ICF /ERRORREPORT:NONE /NOLOGO /MACHINE:$(Platform)
LDFLAGS		+= /LIBPATH:"$(DESTPATH)"

ifeq "$(Platform)" "x86"
LDFLAGS_CONSOLE	:= /SAFESEH /SUBSYSTEM:CONSOLE",5.01"
LDFLAGS_WINDOWS	:= /SAFESEH /SUBSYSTEM:WINDOWS",5.01"
else
LDFLAGS_CONSOLE	:= /SUBSYSTEM:CONSOLE
LDFLAGS_WINDOWS	:= /SUBSYSTEM:WINDOWS
endif


# 源文件搜索路径
vpath %.cc 	$(SRCPATH)
vpath %.h 	$(SRCPATH)

# 最终目标文件搜索路径
vpath %.o 	$(DESTPATH)
vpath %.lib $(DESTPATH)
vpath %.exe $(DESTPATH)

######## 格式匹配规则
# 编译
%.o : %.cc $(XLIB_BASE_H) | $(DESTPATH)
	$(CC) $(CFLAGS) /Fo"$(DESTPATH)/$(@F)" "$<"


$(DESTPATH) :
	@mkdir "$@"

.PHONY : lib
lib : xlib.lib

xlib.lib : $(LIB_O)
	$(AR) $(ARFLAGS) /OUT:"$(DESTPATH)/$(@F)" $^

.PHONY : test
test :
	$(MAKE) SRCPATH="$(SRCPATH)" DESTPATH="$(DESTPATH)/test" MyCFLAGS="/D_XLIB_TEST_" test.exe --no-print-directory

test.exe : xlib_test.o $(LIB_O)
	$(LINK) $(LDFLAGS) $(LDFLAGS_CONSOLE) /OUT:"$(DESTPATH)/$(@F)" $^
	@copy /y "$(DESTPATH)\\$(@F)" "$(DESTPATH)/..\\$(@F)"
	@"$(DESTPATH)\\$(@F)"

.PHONY : clean
clean :
	@if exist x64 @rd /s /q x64
	@if exist x86 @rd /s /q x86

include Makefile.inc.inc

# 修正位置，取消注释，可测试使用 iconv 库。
#CFLAGS += /I"$(SRCPATH)/../work/gnu/iconv/include"
#LDFLAGS += /LIBPATH:"$(SRCPATH)/../work/gnu/iconv/$(ARCH)" iconv.lib

all : test.exe
	@echo make done.

test.exe : $(OBJS)| $(DSTPATH)
	$(LINK) $(LDFLAGS) $(LDFLAGS_CONSOLE) /OUT:"$(DSTPATH)/$(@F)" $(^F)
	@"$(DSTPATH)\\$(@F)"
include Makefile.inc.inc

all : test.exe
	@echo make done.

test.exe : $(OBJS)| $(DSTPATH)
	$(LINK) $(LDFLAGS) $(LDFLAGS_CONSOLE) /OUT:"$(DSTPATH)/$(@F)" $(^F)
	@"$(DSTPATH)\\$(@F)"
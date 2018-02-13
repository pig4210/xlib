CC := g++
CFLAGS := -lstdc++ -std=c++11 -lc -O3 -Wall

SUFFIX := cc
LIB := xlib.a
SHOWCMD := @

SOURCES := $(wildcard *.$(SUFFIX))
OBJS := $(SOURCES:.$(SUFFIX)=.o)
DFILES := $(SOURCES:.$(SUFFIX)=.d)

all : $(DFILES) $(OBJS) $(LIB)
	$(SHOWCMD)rm -f *.d *.o

objs : $(OBJS)

%.d : %.cc
	$(SHOWCMD)$(CC) -MM $< >$@;\
	echo -e "\t$(SHOWCMD)$(CC) -c $(CFLAGS) $< -o$(<:.$(SUFFIX)=.o)" >>$@\

$(LIB) : $(OBJS)
	$(SHOWCMD)ar -rcus $@ $(OBJS)

-include ./$(DFILES)

.PHONY clean:
	$(SHOWCMD)rm -f *.o $(LIB) *.d
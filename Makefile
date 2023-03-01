ifeq "$(filter x64 x86,$(ARCH))" ""
  $(error Need VS Environment)
endif

ifeq "$(ProjectName)" ""
  $(error Need ProjectName)
endif

ifeq "$(SRCPATH)" ""
  $(error Need SRCPATH)
endif

.PHONY : all
all :

DSTPATH     := $(ARCH)

CC          := cl.exe
LINK        := link.exe
AR          := lib.exe

######## CFLAGS
CFLAGS      = /c /MP /GS- /Qpar /GL /analyze- /W4 /Gy /Zc:wchar_t /Zi /Gm- /Ox /Zc:inline /fp:precise /DWIN32 /DNDEBUG /D_UNICODE /DUNICODE /fp:except- /errorReport:none /GF /WX /Zc:forScope /GR- /Gd /Oy /Oi /MT /EHa /nologo /std:c++latest
CFLAGS      += /I"$(SRCPATH)"
CFLAGS      += /Fd"$(DSTPATH)/"

ifeq "$(ARCH)" "x86"
CFLAGS      += /D_USING_V110_SDK71_
endif

CFLAGS      += $(MyCFLAGS)

######## ARFLAGS
ARFLAGS     = /LTCG /ERRORREPORT:NONE /NOLOGO /MACHINE:$(ARCH)
ARFLAGS     += /LIBPATH:"$(DSTPATH)"

######## LDFLAGS
LDFLAGS     = /MANIFEST:NO /LTCG /NXCOMPAT /DYNAMICBASE "kernel32.lib" "user32.lib" "gdi32.lib" "winspool.lib" "comdlg32.lib" "advapi32.lib" "shell32.lib" "ole32.lib" "oleaut32.lib" "uuid.lib" "odbc32.lib" "odbccp32.lib" /OPT:REF /INCREMENTAL:NO /OPT:ICF /ERRORREPORT:NONE /NOLOGO /MACHINE:$(ARCH) /DEBUG:FULL
LDFLAGS     += /LIBPATH:"$(DSTPATH)"

ifeq "$(ARCH)" "x86"
LDFLAGS_CONSOLE := /SAFESEH /SUBSYSTEM:CONSOLE",5.01"
LDFLAGS_WINDOWS := /SAFESEH /SUBSYSTEM:WINDOWS",5.01"
else
LDFLAGS_CONSOLE := /SUBSYSTEM:CONSOLE
LDFLAGS_WINDOWS := /SUBSYSTEM:WINDOWS
endif

vpath %.cc  $(SRCPATH)
vpath %.h   $(SRCPATH)

vpath %.o   $(DSTPATH)
vpath %.lib $(DSTPATH)
vpath %.dll $(DSTPATH)
vpath %.exe $(DSTPATH)

$(DSTPATH) :
	@mkdir "$@"

%.o : %.cc | $(DSTPATH)
	$(CC) $(CFLAGS) /Fo"$(DSTPATH)/$(@F)" "$<"

include Makefile.inc
# 此 Makefile 要满足以下操作：
#     1. 如果当前没有编译环境：
#         - 默认会分别编译 x64 和 x86 。
#         - 如果指定 x64/x86 ，则指定编译。
#     2. 如果当前有编译环境：
#         - 如果判定需要新环境，会新建编译环境。
#         - 如果当前环境是 x64 ，默认编译 x64 。
#               - 如果指定 x86 ，必定新建编译环境编译 x86 。
#         - 如果当前环境是 x86 ，默认编译 x86 。
#               - 如果指定 x64 ，必定新建编译环境编译 x64 。
#     3. 如果当前目录不是 Makefile 所在目录，也能正确执行。

# 在 win7 下，可能默认使用 bash 。这里强制使用 cmd.exe
SHELL = cmd.exe

# 允许的参数：[x64,x86] 或 空。参数为空时，默认 all 包含 x64 和 x86 。

.PHONY : all
all :
.PHONY : x64
x64 :
.PHONY : x86
x86 :

# all_make 让 Makefile.inc 指定生成的内容。
.PHONY : all_make
all_make :

################################################################ SRCPATH
# 获取本 Makefile 的路径。
MyPath  := $(abspath $(dir $(firstword $(MAKEFILE_LIST))))
SRCPATH := $(MyPath)
## $(info SRCPATH = $(SRCPATH))
ifeq "$(SRCPATH)" ""
    $(error SRCPATH empty)
endif

################################################################ VCPath
VCPath := $(shell "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere" -latest -property installationPath 2>nul)
## $(info VCPath = $(VCPath))
ifeq "$(VCPath)" ""
    $(error VCPath empty)
endif
VCPath := $(VCPath)\VC\Auxiliary\Build
vcvarsall := $(VCPath)\vcvarsall.bat

################################################################ ARCH
# msvc 有效时，ARCH 有值。 
ifeq "$(shell cl >nul 2>&1 || echo yes)" ""
    ifeq "$(Platform)" ""
        $(error No Platfrom, prehaps < VS2017)
    endif
    ARCH := $(Platform)
    ## $(info ARCH = $(ARCH))
endif

################################################################ make_direct
# 在当前的编译环境中直接编译

# 注意到： :: 只是连接指令，指令都在依赖后执行，所以要求在依赖前执行输出， :: 并不符合要求。
#         只能添加一个前置虚依赖，以使输出在其他依赖前执行。
.PHONY : make_direct_pre
make_direct_pre :
	@echo.
	@echo ==== ==== ==== ==== Building ($(ARCH)) ==== ==== ==== ====
	@echo.
.PHONY : make_direct
make_direct : make_direct_pre all_make
	@echo.
	@echo ==== ==== ==== ====      Done      ==== ==== ==== ====

################################################################ make_x64_new & make_x86_new
# 已在 x64/x86 编译环境下，新建编译环境编译。
#     1. x64 编译环境时，要求编译 x86 。需要新环境。
#     2. x86 编译环境时，要求编译 x64 。需要新环境。
#     3. SDK 环境与要求不一致时，需要新环境。
# 因为子环境会继承当前环境，所以需要用 start /I 以使新环境使用默认环境。
#     - 注意到，直接 start 会有问题，所以需要先 echo 一下。
#     - 因为是 start 新环境，所以无法得知新环境的 make 结果。
#     - 所以新开环境的 make 如果成功，则会默认直接退出。make 失败，则会暂停。
.PHONY : make_x64_new
make_x64_new :
	@echo.
	@echo ==== ==== ==== ==== Building (x64) at new environment ==== ==== ==== ====
	@echo.
	@echo. & start "New x64" /I cmd /C \
        "echo ==== ==== ==== ==== Prepare VS Environment for (x64) ... & \
        "$(vcvarsall)" x64 >nul && \
        cd /d "$(SRCPATH)" && \
        "$(MAKE)" --no-print-directory -f "$(abspath $(firstword $(MAKEFILE_LIST)))" || pause>nul"
.PHONY : make_x86_new
make_x86_new :
	@echo.
	@echo ==== ==== ==== ==== Building (x86) at new environment ==== ==== ==== ====
	@echo.
	@echo. & start "New x86" /I cmd /C \
        "echo ==== ==== ==== ==== Prepare VS Environment for (x86) ... & \
        "$(vcvarsall)" x86 >nul && \
        cd /d "$(SRCPATH)" && \
        "$(MAKE)" --no-print-directory -f "$(abspath $(firstword $(MAKEFILE_LIST)))" || pause>nul"

################################################################ make_x64 & make_x86
# 当前没有编译环境，在子环境中建立编译环境，并编译。
#     - 因为当前没有编译环境，所以继承，对子环境无影响。
#     - 命令运行的是子环境，而子环境对当前环境无影响。
.PHONY : make_x64
make_x64 :
	@echo.
	@echo ==== ==== ==== ==== Prepare VS Environment for (x64) ...
	@"$(vcvarsall)" x64 >nul && \
    cd /d "$(SRCPATH)" && \
    "$(MAKE)" --no-print-directory -f "$(abspath $(firstword $(MAKEFILE_LIST)))"
.PHONY : make_x86
make_x86 :
	@echo.
	@echo ==== ==== ==== ==== Prepare VS Environment for (x86) ...
	@"$(vcvarsall)" x86 >nul && \
    cd /d "$(SRCPATH)" && \
    "$(MAKE)" --no-print-directory -f "$(abspath $(firstword $(MAKEFILE_LIST)))"

################################################################
ifeq "$(ARCH)" ""
    # 如果当前没有编译环境，默认编译 x64 和 x86 。
    all : x64 x86
    x64 : make_x64
    x86 : make_x86
else ifeq "$(ARCH)" "x64"
    # 如果当前环境是 x64 ，默认编译 x64 。
    all : x64
    # 根据是否需要新环境，采用不同的规则。
    ifeq "$(NeedNewEnv)" ""
        x64 : make_direct
    else
        x64 : make_x64_new
    endif
    # 如果指定编译 x86 ，则需要启动新环境。
    x86 : make_x86_new
else ifeq "$(ARCH)" "x86"
    # 如果当前环境是 x86 ，默认编译 x86 。
    all : x86
    # 根据是否需要新环境，采用不同的规则。
    ifeq "$(NeedNewEnv)" ""
        x86 : make_direct
    else
        x86 : make_x86_new
    endif
    # 如果指定编译 x64 ，则需要启动新环境。
    x64 : make_x64_new
else
    $(error ARCH must be [x86,x64] : $(ARCH))
endif

test :
	@echo tested

################################################################
DSTPATH     := $(SRCPATH)/$(ARCH)

CC          := cl.exe
LINK        := link.exe
AR          := lib.exe

######## CFLAGS
CFLAGS      = /c /MP /GS- /Qpar /GL /analyze- /W4 /Gy /Zc:wchar_t /Zi /Gm- /Ox /Zc:inline /fp:precise /DWIN32 /DNDEBUG /D_UNICODE /DUNICODE /fp:except- /errorReport:none /GF /WX /Zc:forScope /GR- /Gd /Oy /Oi /MT /EHa /nologo /std:c++latest
CFLAGS      += /I"$(SRCPATH)"

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
	$(CC) $(CFLAGS) /Fd"$(DSTPATH)/" /Fo"$(DSTPATH)/$(@F)" "$<"

include $(abspath $(dir $(firstword $(MAKEFILE_LIST))))/Makefile.inc
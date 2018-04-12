# xlib

这里提供的[Makefile.bat](./Makefile.bat)，使用VS2017命令行编译项目

如需要使用其它VS，请修改如下配置：

    set VCPATH=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build

在已有VC命令行环境下运行脚本，只编译当前平台相符的库，并且输出编译信息

无VC命令行环境时，脚本静默编译x64 & x86

---- ---- ---- ----
    
xlib封装了一些常用的函数，用以自用简化编程

xlib使用**UTF-8 编码**，工程适配`VS x64/x86 ring0/ring3`。同时适配`g++`

xlib使用C++ 11**部分**语法

xlib for ring0，需要自行安装[Windows Kits](https://developer.microsoft.com/zh-cn/windows/hardware/windows-driver-kit)

所有注释说明使用**Doxygen**语法书写，函数说明文档请使用[Doxygen](http://www.doxygen.nl)生成

GIT上不提供生成的说明文档，请自行阅读注释或查看doxygen生成的文档，或查阅源代码、注释

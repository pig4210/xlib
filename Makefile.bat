@echo off

:begin
    setlocal
    set MyPath=%~dp0

:config
    if "%1" == "" (
      set PLAT=x64
    ) else (
      set PLAT=x86
    )

    set VCPATH=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build
    set VPATH=%MyPath%
    set GPATH=%MyPath%\\%PLAT%

    set CC=cl
    set AR=lib
    set LNK=link

:compileflags
    set CFLAGS= /c /MP /GS- /TP /Qpar /GL /analyze- /W4 /Gy /Zc:wchar_t /Zi /Gm- /Ox /Zc:inline /fp:precise /D "WIN32" /D "NDEBUG" /D "_UNICODE" /D "UNICODE" /fp:except- /errorReport:none /GF /WX /Zc:forScope /GR- /Gd /Oy /Oi /MT /nologo /Fo"%GPATH%\\"

    set MyCFLAGS= /wd"4456" /wd"4302" /wd"4311" /wd"4312"

    if not "%1" == "" set MyCFLAGS=%MyCFLAGS% /D "_USING_V110_SDK71_"

:arflags
    set ARFLAGS= /LTCG /MACHINE:%PLAT% /ERRORREPORT:NONE /NOLOGO

:linkflags
    if "%1" == "" (
        set LFLAGS_PLAT_CONSOLE= /SUBSYSTEM:CONSOLE
    ) else (
        set LFLAGS_PLAT_CONSOLE= /SAFESEH /SUBSYSTEM:CONSOLE",5.01"
    )

    set LFLAGS= /MANIFEST:NO /LTCG /NXCOMPAT /DYNAMICBASE "kernel32.lib" "user32.lib" "gdi32.lib" "winspool.lib" "comdlg32.lib" "advapi32.lib" "shell32.lib" "ole32.lib" "oleaut32.lib" "uuid.lib" "odbc32.lib" "odbccp32.lib" /MACHINE:%PLAT% /OPT:REF /INCREMENTAL:NO /OPT:ICF /ERRORREPORT:NONE /NOLOGO

:start
    echo ==== ==== ==== ==== Prepare dest folder(%PLAT%)...

    rd /S /Q "%GPATH%" >nul
    if exist "%GPATH%" goto fail
    mkdir "%GPATH%" >nul

    echo ==== ==== ==== ==== Prepare environment(%PLAT%)...

    cd /d %VCPATH%
    if "%1" == "" (
        call vcvarsall.bat amd64 >nul
    ) else (
        call vcvarsall.bat x86 >nul
    )

    cd /d %VPATH%

:test
    echo ==== ==== ==== ==== Building TEST(%PLAT%)...

    %CC% %CFLAGS% %MyCFLAGS% /EHa /Fd"%GPATH%\\test.pdb" /D "_XLIB_TEST_" "%VPATH%\\*.cc" >nul
    if not %errorlevel%==0 goto compile_error
    
    %LNK% /OUT:"%GPATH%\\test.exe" %LFLAGS% %LFLAGS_PLAT_CONSOLE% "%GPATH%\\*.obj" >nul
    if not %errorlevel%==0 goto link_error

    del "%GPATH%\\*.obj"

    "%GPATH%\\test.exe" >nul
    if not %errorlevel%==0 (
        "%GPATH%\\test.exe"
        goto end
    )

:lib
    echo ==== ==== ==== ==== Building LIB(%PLAT%)...

    %CC% %CFLAGS% %MyCFLAGS% /EHa /Fd"%GPATH%\\xlib.pdb" /D "_LIB" "%VPATH%\\*.cc" >nul
    if not %errorlevel%==0 goto compile_error

    %AR% %ARFLAGS% /OUT:"%GPATH%\\xlib.lib" "%GPATH%\\*.obj" >nul
    if not %errorlevel%==0 goto link_error

    del "%GPATH%\\*.obj"

goto done

:libforring0
    echo ==== ==== ==== ==== Building LIB for ring0(%PLAT%)...

    if "%1" == "" (
        set RING0CFLAGS= /D "_AMD64_" /D "FOR_RING0"
    ) else (
        set RING0CFLAGS= /D "_X86_" /D "FOR_RING0"
    )

    %CC% %CFLAGS% %MyCFLAGS% /Fd"%GPATH%\\xlib0.pdb" /D "_LIB" %RING0CFLAGS% /I"SgiSTL" /I"%WindowsSdkDir%\\include\\%WindowsSDKVersion%\\km" /I"%WindowsSdkDir%\\include\\%WindowsSDKVersion%\\km\\crt" "%VPATH%\\*.cc" >nul
    if not %errorlevel%==0 goto compile_error

    %AR% /OUT:"%GPATH%\\xlib0.lib" /LTCG /MACHINE:%PLAT% /NOLOGO "ntoskrnl.lib" /NODEFAULTLIB:"libcimt.lib" /LIBPATH:"%WindowsSdkDir%\\lib\\%WindowsSDKVersion%\\km\\%PLAT%" "%GPATH%\\*.obj" >nul
    if not %errorlevel%==0 goto link_error

    del "%GPATH%\\*.obj"

:done
    echo.
    endlocal

    if "%1" == "" (
        cmd /C %~f0 x86
    ) else (
        exit /B 0
    )

    echo done.
    goto end

:compile_error
    echo !!!!!!!!Compile error!!!!!!!!
    goto end

:link_error
    echo !!!!!!!!Link error!!!!!!!!
    goto end

:fail
    echo !!!!!!!!Fail!!!!!!!!
    goto end

:end
    pause >nul
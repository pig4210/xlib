@echo off

::begin
    setlocal
    pushd "%~dp0"
    
::baseconfig
    set VCPATH=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build
    set MyPath=%CD%

    for /d %%P in (.) do set ProjectName=%%~nP
    if "%ProjectName%"=="" (
        echo !!!!!!!! Empty project name !!!!!!!!
        goto end
    )
    echo ==== ==== ==== ==== Got project name [ %ProjectName% ]
    setlocal enabledelayedexpansion
    for %%I in (a b c d e f g h i j k l m n o p q r s t u v w x y z) do set ProjectName=!ProjectName:%%I=%%I!
    setlocal disabledelayedexpansion

    set VPATH=%MyPath%
    echo ==== ==== ==== ==== Got source folder [ %VPATH% ]
    echo.

::biuldconfig
    set CC=cl
    set AR=lib
    set LNK=link

    set CFLAGS=/c /MP /GS- /TP /Qpar /GL /analyze- /W4 /Gy /Zc:wchar_t /Zi /Gm- /Ox /Zc:inline /fp:precise /D "WIN32" /D "NDEBUG" /D "_UNICODE" /D "UNICODE" /fp:except- /errorReport:none /GF /WX /Zc:forScope /GR- /Gd /Oy /Oi /MT /nologo

    set ARFLAGS=/LTCG /ERRORREPORT:NONE /NOLOGO

    set LFLAGS=/MANIFEST:NO /LTCG /NXCOMPAT /DYNAMICBASE "kernel32.lib" "user32.lib" "gdi32.lib" "winspool.lib" "comdlg32.lib" "advapi32.lib" "shell32.lib" "ole32.lib" "oleaut32.lib" "uuid.lib" "odbc32.lib" "odbccp32.lib" /OPT:REF /INCREMENTAL:NO /OPT:ICF /ERRORREPORT:NONE /NOLOGO

::main
    echo.
    %CC% >nul 2>&1
    if %errorlevel%==0 (
        echo ==== ==== ==== ==== Build %Platform% ^& processing ==== ==== ==== ==== 
        echo.
        call :do || goto end
    ) else (
        echo ==== ==== ==== ==== Build x64 ^& x86 by silence ==== ==== ==== ==== 
        echo.
        call :do x64 || goto end
        call :do x86 || goto end
    )

    popd
    endlocal
    echo.
    echo ==== ==== ==== ==== Done ==== ==== ==== ====
    cl >nul 2>&1 || pause >nul
    exit /B 0

:end
    popd
    endlocal
    echo.
    echo ==== ==== ==== ==== Done ==== ==== ==== ====
    cl >nul 2>&1 || pause >nul
    exit /B 1

:do
    setlocal

    if "%1"=="" (
        set PLAT=%Platform%
        set SUF=
    ) else (
        set PLAT=%1
        set SUF=^>nul
    )
    if "%PLAT%"=="" (
        echo !!!!!!!! Need arg with x64/x86 !!!!!!!!
        goto done
    )
    set GPATH=%MyPath%\\%PLAT%

    echo.

::prepare
    if not "%1"=="" (
        echo ==== ==== ==== ==== Prepare environment^(%PLAT%^)...
        
        cd /d "%VCPath%"
        if "%PLAT%"=="x64" (
            call vcvarsall.bat amd64 >nul
        ) else (
            call vcvarsall.bat x86 >nul
        )
    )

    echo ==== ==== ==== ==== Prepare dest folder(%PLAT%)...

    if exist "%GPATH%" rd /s /q "%GPATH%" >nul
    if exist "%GPATH%" (
        echo !!!!!!!! Can't clear dest folder !!!!!!!!
        goto done
    )
    md "%GPATH%" >nul

    echo.

    cd /d "%VPATH%"

::localbuildconfig
    set CFLAGS=%CFLAGS% /wd"4456" /wd"4302" /wd"4311" /wd"4312"
    if "%PLAT%" == "x86" set CFLAGS=%CFLAGS% /D "_USING_V110_SDK71_"

    set ARFLAGS=%ARFLAGS% /MACHINE:%PLAT%

    set LFLAGS=%LFLAGS% /MACHINE:%PLAT%

    if "%PLAT%" == "x64" (
        set LFLAGS_CONSOLE= /SUBSYSTEM:CONSOLE
    ) else (
        set LFLAGS_CONSOLE= /SAFESEH /SUBSYSTEM:CONSOLE",5.01"
    )

::test
    set CIN="%VPATH%\\*.cc"

    set COUT=/Fo"%GPATH%\\" /Fd"%GPATH%\\test.pdb"

    set MyCFlags=/D "_XLIB_TEST_" /EHa

    set LIN="%GPATH%\\*.obj"

    set LOUT=/OUT:"%GPATH%\\test.exe"

    set MyLFlags=%LFLAGS_CONSOLE%

    echo ==== ==== ==== ==== Building test(%PLAT%)...

    call :compile || goto done
    call :link || goto done

    del "%GPATH%\\*.obj"

    echo ==== ==== ==== ==== Testing(%PLAT%)...
    "%GPATH%\\test.exe" %SUF%
    if not %errorlevel%==0 (
        echo !!!!!!!! Test fail !!!!!!!!
        goto done
    )

::lib
    set CIN="%VPATH%\\*.cc"

    set COUT=/Fo"%GPATH%\\" /Fd"%GPATH%\\%ProjectName%.pdb"

    set MyCFlags=/D "_LIB" /EHa

    set ARIN="%GPATH%\\*.obj"

    set AROUT=/OUT:"%GPATH%\\%ProjectName%.lib"

    set MyARFlags=

    echo ==== ==== ==== ==== Building LIB(%PLAT%)...

    call :compile || goto done
    call :ar || goto done

    del "%GPATH%\\*.obj" >nul

goto ok
::libforring0
    set CIN="%VPATH%\\*.cc" /I"SgiSTL" /I"%WindowsSdkDir%\\include\\%WindowsSDKVersion%\\km" /I"%WindowsSdkDir%\\include\\%WindowsSDKVersion%\\km\\crt"

    set COUT=/Fo"%GPATH%\\" /Fd"%GPATH%\\%ProjectName%0.pdb"

    set MyCFlags=/D "_LIB" /D "FOR_RING0"
    if "%PLAT%"=="" (
        set MyCFlags=%MyCFlags% /D "_AMD64_"
    ) else (
        set MyCFlags=%MyCFlags% /D "_X86_"
    )

    set ARIN="%GPATH%\\*.obj" "ntoskrnl.lib" /NODEFAULTLIB:"libcimt.lib" /LIBPATH:"%WindowsSdkDir%\\lib\\%WindowsSDKVersion%\\km\\%PLAT%"

    set AROUT=/OUT:"%GPATH%\\%ProjectName%0.lib"

    set MyARFlags=

    echo ==== ==== ==== ==== Building LIB for ring0(%PLAT%)...

    call :compile || goto done
    call :ar || goto done

    del "%GPATH%\\*.obj" >nul

:ok
    endlocal
    echo.
    exit /B 0

:done
    endlocal
    echo.
    exit /B 1

:compile
    if "%SUF%"=="" (
        echo.
        echo %CC% %CFLAGS% %MyCFlags% %COUT% %CIN%
        echo.
    )
    %CC% %CFLAGS% %MyCFlags% %COUT% %CIN% %SUF% && exit /B 0
    
    echo !!!!!!!! Compile Error !!!!!!!!
    exit /B 1

:ar
    if "%SUF%"=="" (
        echo.
        echo %AR% %ARFLAGS% %MyARFlags% %AROUT% %ARIN%
        echo.
    )
    %AR% %ARFLAGS% %MyARFlags% %AROUT% %ARIN% %SUF% && exit /B 0

    echo !!!!!!!! AR Error !!!!!!!!
    exit /B 1

:link
    if "%SUF%"=="" (
        echo.
        echo %LNK% %LFLAGS% %MyLFlags% %LOUT% %LIN%
        echo.
    )
    %LNK% %LFLAGS% %MyLFlags% %LOUT% %LIN% %SUF% && exit /B 0

    echo !!!!!!!! Link Error !!!!!!!!
    exit /B 1
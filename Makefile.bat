@echo off

::begin
    setlocal
    pushd "%~dp0"

    :: if has arg , it must be : x64 or x86.
    if not "%1" == "" (
        if not "%1" == "x64" (
            if not "%1" == "x86" (
                echo Need arg x64/x86 !!!
                goto end
            )
        )
    )

    set ARCH=
    set BOTHARCH=

    cl >nul 2>&1
    :: if in the compilation env , check ARCH.
    if not ERRORLEVEL 1 goto checkarch

    if "%1" == "" (
        set ARCH=x64
        set BOTHARCH=1
    ) else (
        set ARCH=%1
    )

    goto findmsvc

:checkarch
    :: VS2013 x86 has no Platform.
    if "%Platform%" == "" set Platform=x86

    set ARCH=x%Platform:~-2%

    if not "%ARCH%" == "x64" (
        if not "%ARCH%" == "x86" (
            echo Unknow ARCH : %ARCH% !
            goto end
        )
    )

    :: if has no arg , use ARCH default . otherwise, check equ.
    if not "%1" == "" (
        if not "%ARCH%" == "%1" (
            :: if arg no equ ARCH , change the ARCH.
            set ARCH=%1
            goto findmsvc
        )
    )
    goto baseconfig

:findvswhere
    set VSWHERE=vswhere
    %VSWHERE% -help >nul 2>&1
    if not ERRORLEVEL 1 exit /b 0

    set "VSWHERE=C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere"
    "%VSWHERE%" -help >nul 2>&1
    if ERRORLEVEL 1 exit /b 1

    exit /b 0

:checkmsvc
    if not exist "%VCPath%" exit /b 1

    echo.
    echo ==== ==== ==== ==== Prepare VS Environment for %ARCH% ...
    call "%VCPath%\vcvarsall.bat" %ARCH% >nul
    ::echo on
    if ERRORLEVEL 1 exit /b 1

    cl >nul 2>&1
    if ERRORLEVEL 1 exit /b 1

    exit /b 0

:findmsvc
    call :findvswhere
    if ERRORLEVEL 1 goto oldervs

    for /f "tokens=* usebackq" %%i in (`"%VSWHERE%" -latest -property installationPath`) do (
        set "VCPath=%%i\VC\Auxiliary\Build"
    )

    call :checkmsvc
    if not ERRORLEVEL 1 goto baseconfig

:oldervs
    set "VCPath=%VS150COMNTOOLS%\..\..\VC"
    call :checkmsvc
    if not ERRORLEVEL 1 goto baseconfig
    
    set "VCPath=%VS140COMNTOOLS%\..\..\VC"
    call :checkmsvc
    if not ERRORLEVEL 1 goto baseconfig

    set "VCPath=%VS120COMNTOOLS%\..\..\VC"
    call :checkmsvc
    if not ERRORLEVEL 1 goto baseconfig

    echo No MSVC compiler available. or no support older MSVC.
    goto end
    
:baseconfig
    set MyPath=%CD%
    set MAKE=%CD%/tools/make.exe

    for /d %%P in (.) do set ProjectName=%%~nP
    if "%ProjectName%"=="" (
        echo !!!!!!!! Empty project name !!!!!!!!
        goto end
    )

    set SRCPATH=%MyPath%

::main
    call :do %ARCH% || goto end
    popd

    pushd "%~dp0"

    if "%1" == "" (
        if "%BOTHARCH%" == "1" (
            echo.
            echo.
            if "%ARCH%" == "x64" (
                call %~n0 x86
            ) else (
                call %~n0 x64
            )
        )
    )

    popd
    endlocal
    echo.
    cl >nul 2>&1 || pause >nul
    exit /B 0

:end
    popd
    endlocal
    echo.
    cl >nul 2>&1 || pause >nul
    exit /B 1

:do
    setlocal

    cd /d "%MyPath%"

    echo.
    echo ==== ==== ==== ==== Building (%ARCH%) ==== ==== ==== ====
    echo.

    call :make || goto done

    echo.

::ok
    endlocal
    echo.
    echo ==== ==== ==== ====      Done      ==== ==== ==== ====
    exit /B 0

:done
    endlocal
    echo.
    echo ==== ==== ==== ====      Done      ==== ==== ==== ====
    exit /B 1

:make
    echo.
    echo "%MAKE%" SRCPATH=%SRCPATH%
    echo.
    "%MAKE%" SRCPATH=%SRCPATH% && exit /B 0
    
    echo !!!!!!!! Make Error !!!!!!!!
    exit /B 1
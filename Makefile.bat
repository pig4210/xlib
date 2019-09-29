@echo off

::begin
    setlocal
    pushd "%~dp0"
    
::baseconfig
    set VCPATH=D:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build
    set MyPath=%CD%
    set MAKE=E:/work/gnu/make/make.exe

    for /d %%P in (.) do set ProjectName=%%~nP
    if "%ProjectName%"=="" (
        echo !!!!!!!! Empty project name !!!!!!!!
        goto end
    )
    echo ==== ==== ==== ==== Got project name [ %ProjectName% ]

    set SrcPath=%MyPath%
    echo ==== ==== ==== ==== Got source folder [ %SrcPath% ]
    echo.

    cl >nul 2>&1
    if %errorlevel%==0 (
        set SUF=
    ) else (
        set SUF=^>nul
    )

::main
    echo.
    cl >nul 2>&1
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

    echo.

::prepare
    if not "%1"=="" (
        echo ==== ==== ==== ==== Prepare environment^(%PLAT%^)...
        
        cd /d "%VCPath%"
        if "%PLAT%"=="x64" (
            call vcvarsall.bat x64 >nul
        ) else (
            call vcvarsall.bat x86 >nul
        )
    )

    cd /d "%MyPath%"

    echo ==== ==== ==== ==== Building %PLAT%...

    call :make || goto done

::ok
    endlocal
    echo.
    exit /B 0

:done
    endlocal
    echo.
    exit /B 1

:make
    if "%SUF%"=="" (
        echo.
        echo %MAKE% -f Makefile.msvc SrcPath=%SrcPath%
        echo.
    )
    %MAKE% -f Makefile.msvc SRCPATH="%SrcPath%" %SUF% && exit /B 0
    
    echo !!!!!!!! Make Error !!!!!!!!
    exit /B 1
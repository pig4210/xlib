::@echo off

pushd "%~dp0"

if not exist .\include\xlib mkdir .\include\xlib

for %%F in (D:\xlib\*.h) do mklink .\include\xlib\%%~nF.h %%F
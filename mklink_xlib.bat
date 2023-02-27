::@echo off

pushd "%~dp0"

if not exist .\include\xlib mkdir .\include\xlib

mklink .\include\xlib\*.h D:\xlib\*.h
rem @echo off

@echo ***************************************************************************
@echo * CF Imager Source Extraction
@echo ***************************************************************************


@echo ****************** BUILD ****************************************************
cd src
C:\Tools\MSVS2008\Common7\IDE\devenv.com cfimager.sln /build release
if ERRORLEVEL 1 goto ERROR
move release\cfimager.exe ..\CFImager.exe

@echo ****************** CLEAN ****************************************************
if exist  .\Release rmdir /S /Q .\Release
if exist  .\Debug   rmdir /S /Q .\Debug
del /Q .\*.aps
del /Q .\*.bak
del /Q .\*.user
del /Q .\*.ncb
del /Q .\*.suo	

@echo ****************** VERSIONING ****************************************************
%PERL%\bin\perl.exe "version.pl" release
call ver.bat

@echo ****************** ZIP ****************************************************
cd ..
if exist "CFImager.v%STMP_BUILD_VERSION%_bin+src.zip" del "CFImager.v%STMP_BUILD_VERSION%_bin+src.zip"
"%TOOLS%\pkzipc.exe" -silent -add -rec -excl=.git* -excl=*.bat -excl=*.zip -excl=*.patch -dir=relative "CFImager.v%STMP_BUILD_VERSION%_bin+src.zip"  "*.*"

@echo ****************** DONE ****************************************************
goto EXIT

:ERROR
@echo CFIMager.exe did not build.
cd ..

:EXIT

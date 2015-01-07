@echo off
rem MSVC80OPTS.BAT
rem
rem    Compile and link options used for building MEX-files
rem    using the Microsoft Visual C++ compiler version 8.0
rem
rem    $Revision: 1.1.10.2 $  $Date: 2006/06/23 19:04:53 $
rem
rem ********************************************************************
rem General parameters
rem ********************************************************************

set MATLAB=%MATLAB%
set VS80COMNTOOLS=%VS80COMNTOOLS%
set VSINSTALLDIR=C:\Program Files (x86)\Microsoft Visual Studio 9.0
set VCINSTALLDIR=%VSINSTALLDIR%\VC
set PATH=%VCINSTALLDIR%\BIN\;%VCINSTALLDIR%\PlatformSDK\bin;%VSINSTALLDIR%\Common7\IDE;%VSINSTALLDIR%\SDK\v2.0\bin;%VSINSTALLDIR%\Common7\Tools;%VSINSTALLDIR%\Common7\Tools\bin;%VCINSTALLDIR%\VCPackages;%MATLAB_BIN%;C:\Program Files\Microsoft SDKs\Windows\v6.0A\Bin;%PATH%
set INCLUDE=%VCINSTALLDIR%\ATLMFC\INCLUDE;%VCINSTALLDIR%\INCLUDE;%VCINSTALLDIR%\PlatformSDK\INCLUDE;%VSINSTALLDIR%\SDK\v2.0\include;%INCLUDE%
set LIB=%VCINSTALLDIR%\ATLMFC\LIB\AMD64;%VCINSTALLDIR%\PlatformSDK\lib\AMD64;%VSINSTALLDIR%\SDK\v2.0\lib\AMD64;%MATLAB%\extern\lib\win64;%LIB%;%VCINSTALLDIR%\LIB\amd64;C:\Program Files\Microsoft SDKs\Windows\v6.0A\Lib\x64
set MW_TARGET_ARCH=win64

rem ********************************************************************
rem Compiler parameters
rem ********************************************************************
rem set COMPILER=cl
set COMPILER=nvcc
set COMPFLAGS= -c -Xcompiler "/c /Zp8 /GR /W3 /EHs /D_CRT_SECURE_NO_DEPRECATE /D_SCL_SECURE_NO_DEPRECATE /D_SECURE_SCL=0 /DMATLAB_MEX_FILE /nologo /MTd"
set OPTIMFLAGS=-Xcompiler "/O2 /Oy- /DNDEBUG"
set DEBUGFLAGS=-Xcompiler "/Zi /Fd"%OUTDIR%%MEX_NAME%%MEX_EXT%.pdb""
set NAME_OBJECT= 
rem set NAME_OBJECT=/Fo

rem ********************************************************************
rem Linker parameters
rem ********************************************************************
set LIBLOC=%MATLAB%\extern\lib\win64\microsoft
set LINKER=link
set LINKFLAGS=/dll /export:%ENTRYPOINT% /MAP /LIBPATH:"%LIBLOC%" libmx.lib libmex.lib libmat.lib /implib:%LIB_NAME%.x /MACHINE:X64 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib
set LINKOPTIMFLAGS=
set LINKDEBUGFLAGS=/DEBUG /PDB:"%OUTDIR%%MEX_NAME%%MEX_EXT%.pdb"
set LINK_FILE=
set LINK_LIB=
set NAME_OUTPUT=/out:"%OUTDIR%%MEX_NAME%%MEX_EXT%"
set RSP_FILE_INDICATOR=@

rem ********************************************************************
rem Resource compiler parameters
rem ********************************************************************
set RC_COMPILER=rc /fo "%OUTDIR%mexversion.res"
set RC_LINKER=

set POSTLINK_CMDS=del "%OUTDIR%%MEX_NAME%.map"
set POSTLINK_CMDS1=del %LIB_NAME%.x
set POSTLINK_CMDS2=mt -outputresource:"%OUTDIR%%MEX_NAME%%MEX_EXT%";2 -manifest "%OUTDIR%%MEX_NAME%%MEX_EXT%.manifest"
set POSTLINK_CMDS3=del "%OUTDIR%%MEX_NAME%%MEX_EXT%.manifest" 

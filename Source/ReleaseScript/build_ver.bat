
set UE4VER=%1
set PLATFORM=%2
set BUILDTYPE=%3
set ZIPNAME=../../../../_zip/%5
::set PROJECTNAMEEDITOR="MyProjectBuildScriptEtidor"
set PROJECTNAMEEDITOR=%4

set UE4BASE=D:\Program Files\Epic Games
set UPROJECT="C:\Users\ruyo\Documents\Unreal Projects\MyProjectBuildScript\MyProjectBuildScript\MyProjectBuildScript.uproject"
set UNREALVERSIONSELECTOR="C:\Program Files (x86)\Epic Games\Launcher\Engine\Binaries\Win64\UnrealVersionSelector.exe"


powershell -ExecutionPolicy RemoteSigned .\delIntermediate.ps1

::cd ../../../Plugins

git reset HEAD ./
git checkout ./

::cd VRM4U/Source/ReleaseScript 

powershell -ExecutionPolicy RemoteSigned .\version.ps1 \"%UE4VER%\"


set UE4PATH=UE_%UE4VER%

set CLEAN="%UE4BASE%\%UE4PATH%\Engine\Build\BatchFiles\Clean.bat"
set BUILD="%UE4BASE%\%UE4PATH%\Engine\Build\BatchFiles\Build.bat"
set REBUILD="%UE4BASE%\%UE4PATH%\Engine\Build\BatchFiles\Rebuild.bat"
set PROJECTNAME="../../../../MyProjectBuildScript.uproject"


if %UE4VER% == 5.0EA (
  call "D:\Program Files\Epic Games\UE_5.0EA\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" -projectfiles -project=%UPROJECT% -game -rocket -progress
) else (
  call %UNREALVERSIONSELECTOR% /projectfiles %UPROJECT%
)

if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)

if %PLATFORM% == Android (
    echo android
) else (
    echo not android
    call %CLEAN% %PROJECTNAMEEDITOR% %PLATFORM% %BUILDTYPE% %UPROJECT% -waitmutex
)

set tmpOldFlag=FALSE
if %UE4VER% == 4.21 set tmpOldFlag=TRUE
if %UE4VER% == 4.20 set tmpOldFlag=TRUE
if %UE4VER% == 4.19 set tmpOldFlag=TRUE

if %tmpOldFlag% == TRUE (
del "../Plugins\VRM4U\Source\VRM4U\Private\VRM4U_AnimSubsystem.cpp"
del "../Plugins\VRM4U\Source\VRM4U\Public\VRM4U_AnimSubsystem.h"
)


call %BUILD% %PROJECTNAMEEDITOR%       %PLATFORM% %BUILDTYPE% %UPROJECT% -waitmutex

if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)
:: releasetool

cd ../Plugins

del *.lib /s


cd ../releasetool

powershell -ExecutionPolicy RemoteSigned .\compress.ps1 %ZIPNAME%



:finish
exit /b 0

:err
exit /b 1



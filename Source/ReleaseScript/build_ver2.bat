
set UE5VER=%1
set PLATFORM=%2
set BUILDTYPE=%3
set ZIPNAME=../../../../_zip/%4

set UE5BASE=D:\Program Files\Epic Games
set UPLUGIN="%~dp0../../VRM4U.uplugin"
set OUTPATH=d:/tmp/_out

git reset --hard HEAD

powershell -ExecutionPolicy RemoteSigned .\version.ps1 \"%UE5VER%\"

set UE5PATH=UE_%UE5VER%

set BUILD="%UE5BASE%\%UE5PATH%\Engine\Build\BatchFiles\RunUAT.bat"

::: delete

set /a UEVersion=%UE5VER%
for /f %%i in ('wsl echo "%UEVersion% * 100"') do set UEVersion100=%%i
for /f "tokens=1 delims=." %%a in ("%UEVersion%") do set UEMajorVersion=%%a


del "..\..\..\VRM4U\Content\Util\Actor\latest\WBP_MorphTarget.uasset"

set isRetargeterEnable=TRUE

if %UE5VER% == 5.0 set isRetargeterEnable=FALSE
if %UE5VER% == 5.1 set isRetargeterEnable=FALSE

if %isRetargeterEnable% == FALSE (
del "..\..\..\VRM4U\Source\VRM4U\Private\VrmAnimInstanceRetargetFromMannequin.cpp"
del "..\..\..\VRM4U\Source\VRM4U\Public\VrmAnimInstanceRetargetFromMannequin.h"
)

set isRenderModuleEnable=TRUE
if %UE5VER% == 5.0 set isRenderModuleEnable=FALSE

if %isRenderModuleEnable% == FALSE (
del "..\..\..\VRM4U\Source\VRM4URender\VRM4URender.Build.cs"
)


::::::::::::::::::::::::::: generate


call %BUILD% BuildPlugin -plugin=%UPLUGIN% -package=%OUTPATH% -TargetPlatforms=%PLATFORM% -clientconfig=%BUILDTYPE% %UPROJECT%

if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)


powershell -ExecutionPolicy RemoteSigned .\compress2.ps1 %ZIPNAME% %OUTPATH%



:finish
exit /b 0

:err
exit /b 1



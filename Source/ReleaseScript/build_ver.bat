
set UE4VER=%1
set PLATFORM=%2
set BUILDTYPE=%3
set ZIPNAME=../../../../_zip/%5
::set PROJECTNAMEEDITOR="MyProjectBuildScriptEtidor"
set PROJECTNAMEEDITOR=%4

set UE4BASE=D:\Program Files\Epic Games
set UPROJECT="C:\Users\ruyo\Documents\Unreal Projects\MyProjectBuildScript\MyProjectBuildScript.uproject"
set UNREALVERSIONSELECTOR="C:\Program Files (x86)\Epic Games\Launcher\Engine\Binaries\Win64\UnrealVersionSelector.exe"


git reset --hard HEAD

powershell -ExecutionPolicy RemoteSigned .\version.ps1 \"%UE4VER%\"

powershell -ExecutionPolicy RemoteSigned .\delIntermediate.ps1


set UE4PATH=UE_%UE4VER%

set CLEAN="%UE4BASE%\%UE4PATH%\Engine\Build\BatchFiles\Clean.bat"
set BUILD="%UE4BASE%\%UE4PATH%\Engine\Build\BatchFiles\Build.bat"
set REBUILD="%UE4BASE%\%UE4PATH%\Engine\Build\BatchFiles\Rebuild.bat"
set PROJECTNAME="../../../../MyProjectBuildScript.uproject"

::: delete

set tmpOldFlag=FALSE
if %UE4VER% == 4.21 set tmpOldFlag=TRUE
if %UE4VER% == 4.20 set tmpOldFlag=TRUE
if %UE4VER% == 4.19 set tmpOldFlag=TRUE

if %tmpOldFlag% == TRUE (
del "..\..\..\VRM4U\Source\VRM4U\Private\VRM4U_AnimSubsystem.cpp"
del "..\..\..\VRM4U\Source\VRM4U\Public\VRM4U_AnimSubsystem.h"
del "..\..\..\VRM4U\Source\VRM4URender\Private\VRM4U_RenderSubsystem.cpp"
del "..\..\..\VRM4U\Source\VRM4URender\Public\VRM4U_RenderSubsystem.h"
)

:: del for version <= 5.1
del "..\..\..\VRM4U\Source\VRM4U\Private\RigUnit_VrmDynamicHierarchy.cpp"
del "..\..\..\VRM4U\Source\VRM4U\Public\RigUnit_VrmDynamicHierarchy.h"

set /a UEVersion=%UE4VER%
for /f %%i in ('wsl echo "%UEVersion% * 100"') do set UEVersion100=%%i
for /f "tokens=1 delims=." %%a in ("%UEVersion%") do set UEMajorVersion=%%a


set isUE4=FALSE
if %UEMajorVersion% == 4 (
    set isUE4=TRUE
)

if %isUE4% == FALSE (
del "..\..\..\VRM4U\Content\Util\Actor\latest\WBP_MorphTarget.uasset"
)

if %isUE4% == TRUE (
del "..\..\..\VRM4U\Source\VRM4U\Public\VrmAssetUserData.h"
del "..\..\..\VRM4U\Source\VRM4U\Private\VrmAssetUserData.cpp"
)


set isRetargeterEnable=TRUE
if %UEMajorVersion% == 4 (
    set isRetargeterEnable=FALSE
)
if %UE4VER% == 5.0 set isRetargeterEnable=FALSE
if %UE4VER% == 5.1 set isRetargeterEnable=FALSE

if %isRetargeterEnable% == FALSE (
del "..\..\..\VRM4U\Source\VRM4U\Private\VrmAnimInstanceRetargetFromMannequin.cpp"
del "..\..\..\VRM4U\Source\VRM4U\Public\VrmAnimInstanceRetargetFromMannequin.h"
del "..\..\..\VRM4U\Source\VRM4U\Private\VrmAnimInstanceTemplate.cpp"
del "..\..\..\VRM4U\Source\VRM4U\Public\VrmAnimInstanceTemplate.h"
)

set isRenderModuleEnable=TRUE
if %UE4VER% == 5.0 set isRenderModuleEnable=FALSE
if %UE4VER% == 5.1 set isRenderModuleEnable=FALSE
if %isUE4% == TRUE set isRenderModuleEnable=FALSE

if %isRenderModuleEnable% == FALSE (
del "..\..\..\VRM4U\Source\VRM4URender\VRM4URender.Build.cs"
)

set isAssetDefinition=TRUE
if %UE4VER% == 5.1 set isAssetDefinition=FALSE
if %UE4VER% == 5.0 set isAssetDefinition=FALSE
if %isUE4% == TRUE set isAssetDefinition=FALSE
if %isAssetDefinition% == FALSE (
del "..\..\..\VRM4U\Source\VRM4UImporter\Private\AssetTypeActions\AssetDefinition_VRM1License.h"
del "..\..\..\VRM4U\Source\VRM4UImporter\Private\AssetTypeActions\AssetDefinition_VRM1License.cpp"
del "..\..\..\VRM4U\Source\VRM4UImporter\Private\AssetTypeActions\AssetDefinition_VRMLicense.h"
del "..\..\..\VRM4U\Source\VRM4UImporter\Private\AssetTypeActions\AssetDefinition_VRMLicense.cpp"
del "..\..\..\VRM4U\Source\VRM4UImporter\Private\AssetTypeActions\AssetDefinition_VRMAssetList.h"
del "..\..\..\VRM4U\Source\VRM4UImporter\Private\AssetTypeActions\AssetDefinition_VRMAssetList.cpp"
del "..\..\..\VRM4U\Source\VRM4UImporter\Private\AssetTypeActions\AssetDefinition_VRMMeta.h"
del "..\..\..\VRM4U\Source\VRM4UImporter\Private\AssetTypeActions\AssetDefinition_VRMMeta.cpp"
)


::::::::::::::::::::::::::: generate

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

call %BUILD% %PROJECTNAMEEDITOR%       %PLATFORM% %BUILDTYPE% %UPROJECT% -waitmutex

if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)
:: releasetool

::cd ../Plugins

::del *.lib /s


::cd ../releasetool

powershell -ExecutionPolicy RemoteSigned .\compress.ps1 %ZIPNAME%



:finish
exit /b 0

:err
exit /b 1



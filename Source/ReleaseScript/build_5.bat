set V_DATE=%date:~0,4%%date:~5,2%%date:~8,2%

set BUILD_SCRIPT=build_ver2.bat

::5_7
call %BUILD_SCRIPT% 5.7 Win64 Shipping VRM4U_5_7_%V_DATE%.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)
call %BUILD_SCRIPT% 5.7 Android Development VRM4U_5_7_%V_DATE%_android.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)

::5_6
call %BUILD_SCRIPT% 5.6 Win64 Shipping VRM4U_5_6_%V_DATE%.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)
call %BUILD_SCRIPT% 5.6 Android Development VRM4U_5_6_%V_DATE%_android.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)

::5_5
call %BUILD_SCRIPT% 5.5 Win64 Shipping VRM4U_5_5_%V_DATE%.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)
call %BUILD_SCRIPT% 5.5 Win64 Debug VRM4U_5_5_%V_DATE%_debug.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)


::5_4
call %BUILD_SCRIPT% 5.4 Win64 Shipping VRM4U_5_4_%V_DATE%.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)

::call build_ver.bat 5.4 Win64 Development MyProjectBuildScriptEditor VRM4U_5_4_%V_DATE%.zip
::if not %errorlevel% == 0 (
::    echo [ERROR] :P
::    goto err
::)


::5_3
call %BUILD_SCRIPT% 5.3 Win64 Shipping VRM4U_5_3_%V_DATE%.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)

::5_2
call %BUILD_SCRIPT% 5.2 Win64 Shipping VRM4U_5_2_%V_DATE%.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)


:finish
exit /b 0

:err
exit /b 1



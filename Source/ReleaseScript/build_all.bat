set V_DATE=%date:~0,4%%date:~5,2%%date:~8,2%

::5_3
call build_ver.bat 5.3 Win64 Development MyProjectBuildScriptEditor VRM4U_5_3_%V_DATE%.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)
call build_ver.bat 5.3 Win64 Development MyProjectBuildScript VRM4U_5_3_%V_DATE%_gam.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)
call build_ver.bat 5.3 Win64 Shipping MyProjectBuildScript VRM4U_5_3_%V_DATE%_gam_shipping.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)

::5_2
call build_ver.bat 5.2 Win64 Development MyProjectBuildScriptEditor VRM4U_5_2_%V_DATE%.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)
call build_ver.bat 5.2 Win64 Development MyProjectBuildScript VRM4U_5_2_%V_DATE%_gam.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)
call build_ver.bat 5.2 Win64 Shipping MyProjectBuildScript VRM4U_5_2_%V_DATE%_gam_shipping.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)

::5_1
call build_ver.bat 5.1 Win64 Development MyProjectBuildScriptEditor VRM4U_5_1_%V_DATE%.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)
call build_ver.bat 5.1 Win64 Development MyProjectBuildScript VRM4U_5_1_%V_DATE%_gam.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)
call build_ver.bat 5.1 Win64 Shipping MyProjectBuildScript VRM4U_5_1_%V_DATE%_gam_shipping.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)


::5_0
call build_ver.bat 5.0 Win64 Development MyProjectBuildScriptEditor VRM4U_5_0_%V_DATE%.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)
call build_ver.bat 5.0 Win64 Development MyProjectBuildScript VRM4U_5_0_%V_DATE%_gam.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)
call build_ver.bat 5.0 Android Development MyProjectBuildScript VRM4U_5_0_%V_DATE%_android.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)


::5_0EA
::call build_ver.bat 5.0EA Win64 Development MyProjectBuildScriptEditor VRM4U_5_0EA_%V_DATE%.zip
::if not %errorlevel% == 0 (
::    echo [ERROR] :P
::    goto err
::)
::call build_ver.bat 5.0EA Win64 Development MyProjectBuildScript VRM4U_5_0EA_%V_DATE%_gam.zip
::if not %errorlevel% == 0 (
::    echo [ERROR] :P
::    goto err
::)

::4_27
call build_ver.bat 4.27 Win64 Development MyProjectBuildScriptEditor VRM4U_4_27_%V_DATE%.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)
call build_ver.bat 4.27 Win64 Development MyProjectBuildScript VRM4U_4_27_%V_DATE%_gam.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)


::4_26
call build_ver.bat 4.26 Win64 Development MyProjectBuildScriptEditor VRM4U_4_26_%V_DATE%.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)

::4_25
call build_ver.bat 4.25 Win64 Development MyProjectBuildScriptEditor VRM4U_4_25_%V_DATE%.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)

::4_24
call build_ver.bat 4.24 Win64 Development MyProjectBuildScriptEditor VRM4U_4_24_%V_DATE%.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)

::4_23
call build_ver.bat 4.23 Win64 Development MyProjectBuildScriptEditor VRM4U_4_23_%V_DATE%.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)

::4_22

call build_ver.bat 4.22 Win64 Development MyProjectBuildScriptEditor VRM4U_4_22_%V_DATE%.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)

::4_21


call build_ver.bat 4.21 Win64 Development MyProjectBuildScriptEditor VRM4U_4_21_%V_DATE%.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)


call build_ver.bat 4.20 Win64 Development MyProjectBuildScriptEditor VRM4U_4_20_%V_DATE%.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)

:: game

call build_ver.bat 4.26 Win64 Development MyProjectBuildScript VRM4U_4_26_%V_DATE%_gam.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)

call build_ver.bat 4.25 Win64 Development MyProjectBuildScript VRM4U_4_25_%V_DATE%_gam.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)

call build_ver.bat 4.24 Win64 Development MyProjectBuildScript VRM4U_4_24_%V_DATE%_gam.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)

call build_ver.bat 4.23 Win64 Development MyProjectBuildScript VRM4U_4_23_%V_DATE%_gam.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)

call build_ver.bat 4.22 Win64 Development MyProjectBuildScript VRM4U_4_22_%V_DATE%_gam.zip
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)


:finish
exit /b 0

:err
exit /b 1



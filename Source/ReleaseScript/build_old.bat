set V_DATE=%date:~0,4%%date:~5,2%%date:~8,2%


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


:finish
exit /b 0

:err
exit /b 1



call build_5.bat
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)
call build_old.bat
if not %errorlevel% == 0 (
    echo [ERROR] :P
    goto err
)


:finish
exit /b 0

:err
exit /b 1



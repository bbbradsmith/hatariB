call c:\msys64\msys2_shell.cmd -ucrt64 -defterm -no-start -c "hatariB/vscode/build.sh"
@IF ERRORLEVEL 1 GOTO error

REM to load core with no disk:
call test_core.bat

REM to start retroarch normally:
REM call test_manual_launch.bat

@echo.
@echo.
@echo Build successful!
@GOTO end
:error
@echo.
@echo.
@echo Build error!
:end

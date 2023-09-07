call c:\msys64\msys2_shell.cmd -ucrt64 -defterm -no-start -c "hatariB/vscode/build.sh"
@IF ERRORLEVEL 1 GOTO error

@echo.
@echo.
@echo Build successful!
@GOTO end
:error
@echo.
@echo.
@echo Build error!
:end

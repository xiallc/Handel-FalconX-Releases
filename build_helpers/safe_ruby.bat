REM Get ruby installation path
FOR /F "usebackq tokens=2,3" %%i IN (`reg query HKCU\Software\RubyInstaller\MRI\1.9.2 /v InstallLocation`) DO SET RUBY192_PATH=%%j

"%RUBY192_PATH%\bin\ruby.exe" %*

SET RUBY192_PATH=

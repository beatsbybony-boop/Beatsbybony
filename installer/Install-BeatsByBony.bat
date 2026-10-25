@echo off
rem Copies BeatsByBony into the standard VST3 folder. Right-click this file and choose "Run as administrator".
set "DEST=%CommonProgramFiles%\VST3"
if not exist "%DEST%" mkdir "%DEST%"
xcopy /E /I /Y "%~dp0BeatsByBony.vst3" "%DEST%\BeatsByBony.vst3"
if errorlevel 1 (
  echo.
  echo Something went wrong. Did you right-click and pick "Run as administrator"?
) else (
  echo.
  echo Done! Open FL Studio, go to Options - Manage plugins, and press "Find installed plugins".
)
pause

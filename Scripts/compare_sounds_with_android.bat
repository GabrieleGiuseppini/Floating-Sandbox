@echo off

SET ANDROID_ROOT=C:\Users\Neurodancer\source\repos\Floating-Sandbox-Mobile\Baking\Sounds
SET PC_ROOT=C:\Users\Neurodancer\source\repos\Floating-Sandbox\Data\Sounds

pushd %ANDROID_ROOT%
for /r %%i in (*) do (
if not exist %PC_ROOT%\%%~ni.flac echo NOT EXIST IN PC: %%~ni
)
popd

pushd %PC_ROOT%
for /r %%i in (*) do (
if not exist %ANDROID_ROOT%\%%~ni.raw echo NOT EXIST IN ANDROID: %%~ni
)
popd



cd ..\workdir\sounds
FOR /r %%i in (*.ogg) do (
 del %%i
)
pause

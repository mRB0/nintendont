@echo off
PATH=C:\wla\bin
@echo ---------------------------------
@echo Cleaning....
@echo ---------------------------------
del build\*.obj
del *.smc
@echo ---------------------------------
@echo Compiling:
@echo ---------------------------------
wla-65816 -ov src/main.asm build/main.obj
@echo ---------------------------------
@echo Linking:
@echo ---------------------------------
wlalink -rvS main.link demo.smc

pause
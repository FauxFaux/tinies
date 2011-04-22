@echo off

nmake cleanall
nmake
mkdir %TARGET_CPU%
copy *.exe %TARGET_CPU%
del vc100.pdb
mkdir syms\%TARGET_CPU%
copy *.pdb syms\%TARGET_CPU%

CC = cl /nologo /EHsc /MD /Os /Zi /D_UNICODE /DUNICODE /W2 /Oi /GL /GS /Gy
MSVCLINK = /OPT:REF /OPT:ICF
LINK = /link /dynamicbase /nxcompat /ltcg /debug $(MSVCLINK) /manifest
MT = mt /nologo -manifest $@.manifest -outputresource:$@;1

all: aukiller.exe \
	noelev.exe \
	powerstatustray.exe \
	quickkey.exe \
	topkey.exe \
	mousex.exe \
	keydump.exe \
	keytoputty.exe \
	loaddlls.exe \
	foobar2000-loader.exe \
	shiftfocus.exe \
	spotify-ctl.exe \
	unrequireadmin.exe

.cpp.exe:
	$(CC) $*.cpp $(LINK)
	$(MT)


clean:
	del *.obj
	del *.manifest
	del *.ilk
	
cleanall: clean
	del *.exe
	del *.pdb
	
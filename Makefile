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
	unrequireadmin.exe

aukiller.exe: $*.cpp
	$(CC) $*.cpp $(LINK) user32.lib
	$(MT)

noelev.exe: $*.cpp
	$(CC) $*.cpp $(LINK)
	$(MT)

powerstatustray.exe: $*.cpp
	$(CC) $*.cpp $(LINK) user32.lib shell32.lib
	$(MT)

quickkey.exe: $*.cpp
	$(CC) $*.cpp $(LINK) user32.lib
	$(MT)

mousex.exe: $*.cpp
	$(CC) $*.cpp $(LINK) xinput.lib user32.lib
	$(MT)

topkey.exe: $*.cpp
	$(CC) $*.cpp $(LINK) user32.lib shell32.lib ole32.lib
	$(MT)

keydump.exe: $*.cpp
	$(CC) $*.cpp $(LINK) user32.lib dinput8.lib
	$(MT)

keytoputty.exe: $*.cpp
	$(CC) $*.cpp $(LINK) user32.lib dinput8.lib
	$(MT)

loaddlls.exe: $*.cpp
	$(CC) $*.cpp $(LINK)
	$(MT)

foobar2000-loader.exe: $*.cpp
	$(CC) $*.cpp $(LINK)
	$(MT)

shiftfocus.exe: $*.cpp
	$(CC) $*.cpp $(LINK) user32.lib
	$(MT)

unrequireadmin.exe: $*.cpp
	$(CC) $*.cpp $(LINK)
	$(MT)

clean:
	del *.obj
	del *.manifest
	del *.ilk
	
cleanall: clean
	del *.exe
	del *.pdb
	
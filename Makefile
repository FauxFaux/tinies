CC = cl /nologo /EHsc /MD /Os /Zi /D_UNICODE /DUNICODE /W1 /Oi /GL /GS /Gy
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
	unrequireadmin.exe

aukiller.exe: aukiller.cpp
	$(CC) aukiller.cpp $(LINK) user32.lib
	$(MT)

noelev.exe: noelev.cpp
	$(CC) noelev.cpp $(LINK)
	$(MT)

powerstatustray.exe: powerstatustray.cpp
	$(CC) powerstatustray.cpp $(LINK) user32.lib shell32.lib
	$(MT)

quickkey.exe: quickkey.cpp
	$(CC) quickkey.cpp $(LINK) user32.lib
	$(MT)

mousex.exe: mousex.cpp
	$(CC) mousex.cpp $(LINK) xinput.lib user32.lib
	$(MT)

topkey.exe: topkey.cpp
	$(CC) topkey.cpp $(LINK) user32.lib shell32.lib ole32.lib
	$(MT)

keydump.exe: keydump.cpp
	$(CC) keydump.cpp $(LINK) user32.lib dinput8.lib
	$(MT)

keytoputty.exe: keytoputty.cpp
	$(CC) keytoputty.cpp $(LINK) user32.lib dinput8.lib
	$(MT)

loaddlls.exe: loaddlls.cpp
	$(CC) loaddlls.cpp $(LINK)
	$(MT)

foobar2000-loader.exe: foobar2000-loader.cpp
	$(CC) foobar2000-loader.cpp $(LINK)
	$(MT)

unrequireadmin.exe: unrequireadmin.cpp
	$(CC) unrequireadmin.cpp $(LINK)
	$(MT)

clean:
	del *.obj
	del *.manifest
	del *.ilk
	
cleanall: clean
	del *.exe
	del *.pdb
	
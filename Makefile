CC = cl /nologo /EHsc /MT /Os /Zi /D_UNICODE /DUNICODE

all: aukiller.exe \
	noelev.exe \
	powerstatustray.exe \
	quickkey.exe \
	topkey.exe \
	unrequireadmin.exe

aukiller.exe: aukiller.cpp
	$(CC) aukiller.cpp /link user32.lib

noelev.exe: noelev.cpp
	$(CC) noelev.cpp

powerstatustray.exe: powerstatustray.cpp
	$(CC) powerstatustray.cpp /link user32.lib shell32.lib

quickkey.exe: quickkey.cpp
	$(CC) quickkey.cpp /link user32.lib

topkey.exe: topkey.cpp
	$(CC) topkey.cpp /link user32.lib

unrequireadmin.exe: unrequireadmin.cpp
	$(CC) unrequireadmin.cpp

clean:
	del *.obj
	del *.manifest
	del *.ilk
	
cleanall: clean
	del *.exe
	del *.pdb
	
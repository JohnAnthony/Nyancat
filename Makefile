RES = /usr/share/nyancat
BIN = /usr/bin/nyancat
LIBS = -lSDL -lSDL_image -lSDL_mixer -lX11
FLAGS = -pedantic -Wall -O2 -std=gnu99 -D_GNU_SOURCE
INCS = -I. -I/usr/include ${XINERAMAINC}

XINERAMAINC = -I/usr/X11R6/include
XINERAMALIBS = -L/usr/X11R6/lib -lXinerama
XINERAMAFLAGS = -DXINERAMA

nyancat:  nyan.c
	cc -g nyan.c -o nyancat ${LIBS} ${XINERAMALIBS} ${XINERAMAINC} ${FLAGS} ${XINERAMAFLAGS} 

install:
	cp nyancat ${BIN}
	mkdir --parents ${RES}
	cp -rv res/* ${RES}

clean:
	rm nyancat

uninstall:
	rm ${BIN}
	rm -rv ${RES}

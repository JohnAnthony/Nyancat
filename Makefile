RES = /usr/share/nyancat
BIN = /usr/bin/nyancat
LIBS = -lSDL -lSDL_image -lSDL_mixer
FLAGS = -Wall

nyancat:  nyan.c
	cc -g nyan.c -o nyancat ${LIBS} ${FLAGS}

install:
	cp nyancat ${BIN}
	mkdir --parents ${RES}
	cp -v res/* ${RES}

clean:
	rm nyancat

uninstall:
	rm ${BIN}
	rm -rv ${RES}

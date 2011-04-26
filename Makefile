nyancat:  nyan.c
	gcc -g nyan.c -o nyancat -lSDL -lSDL_image -lSDL_mixer -Wall
	chmod +x nyancat

install:
	cp nyan /usr/bin

clean:
	rm nyancat

uninstall:
	rm /usr/bin/nyancat

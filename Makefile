nyancat:  nyan.c
	gcc -g nyan.c -o nyancat -lSDL -lSDL_image -lSDL_mixer -Wall
	chmod +x nyancat

install:
	cp nyancat /usr/bin
	mkdir --parents /usr/share/nyancat
	cp -v res/* /usr/share/nyancat

clean:
	rm nyancat

uninstall:
	rm /usr/bin/nyancat
	rm -rv /usr/share/nyancat

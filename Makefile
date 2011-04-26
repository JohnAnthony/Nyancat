nyahcat:  nyah.c
	gcc -g nyah.c -o nyahcat -lSDL -lSDL_image -Wall
	chmod +x nyahcat

install:
	cp nyah /usr/bin

clean:
	rm nyahcat

uninstall:
	rm /usr/bin/nyahcat

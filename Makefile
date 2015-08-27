all: shpion

shpion: shpion.c
	 gcc -Wall -o shpion shpion.c -lpaho-mqtt3a -lpaho-mqtt3c -lssl


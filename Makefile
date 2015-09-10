all: shpion shpion_sub

shpion: shpion.c
	 gcc -Wall -o shpion shpion.c -lpaho-mqtt3a -lpaho-mqtt3c -lssl 

shpion_sub: shpion_sub.c
	 gcc -Wall -o shpion_sub shpion_sub.c -lpaho-mqtt3c 


clean:
	-rm -f shpion shpion_sub *.o

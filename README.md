shpion
=======

Introduction
-----------
shpion is a little program that publishes counts of keyboard and mouse input events anonymously to a mqtt broker. shpion only works on linux. 

INSTALLATION
------------

This program requires cmake, the paho-mqtt3c library, & the open ssl library to build. It installs the compiled binaries to /usr/local/bin.

Build Procedure:
```
 cd shpion
 mkdir target
 cmake ..
 make install
 sudo make install
```

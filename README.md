shpion
=======

Introduction
-----------
shpion is a little program that counts keyboard and mouse input events and publishes them anonymously to a mqtt broker.

INSTALLATION
------------

This program requires cmake, lib-paho3c, & ssl to build. It installs the compiled binaries to /usr/local/bin.

Build Procedure:
```
 cd shpion
 mkdir target
 cmake ..
 make install
 sudo make install
```

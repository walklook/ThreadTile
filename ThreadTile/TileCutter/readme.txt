You need to install biplist, libjpeg and PIL to use this python scripts.

install biplist:
   1. sudo easy_install biplist
install libjpeg:
   1. download jpegsrc.v9a.tar.gz from http://www.ijg.org/files/
   2. install the library according to its install.txt
install PIL
   1. download PIL 1.1.7 from http://www.pythonware.com/products/pil/
   2. modify setup.py, change "JPEG_ROOT = NONE" to "JPEG_ROOT = '/usr/local/lib'"
   3. install PIL according to its Readme.txt, you can use "sudo ARCHFLAGS=-Wno-error=unused-command-line-argument-hard-error-in-future" when you meet any clang problem.

After all done, you can use command like "python TileCutter.py level1_HD.png 512 512 jpg" to get the tiles, the first 512 is the width of the tile, the second is the height. The script will generate the tiles and a plist file which is used in game to describe the necessary information.
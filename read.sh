#!/bin/bash

esptool.py --port /dev/ttyUSB0 read_flash 0x00000 0x3E000 eagle.flash.bin 

echo "Reset ESP8266 with boot mode then press ENTER to read next"
read
esptool.py --port /dev/ttyUSB0 read_flash 0x40000 0x3C000 eagle.irom0text.bin
hexdump -C eagle.flash.bin  > eagle.flash.bin.rd
hexdump -C ../bin/eagle.flash.bin  > eagle.flash.bin.orig
hexdump -C eagle.irom0text.bin > eagle.irom0text.bin.rd
hexdump -C ../bin/eagle.irom0text.bin > eagle.irom0text.bin.orig

#!/bin/bash
flashrom=0
flashblank=0
flashconfig=0
runuart=0

if [[ "x${1}x" != "xx" ]]; then
  flashrom=1;
  runuart=1;
else
  if [[ `expr index "$1" r` ]]; then
    flashrom=1;
  fi
  if [[ `expr index "$1" u` ]]; then
    runuart=1;
  fi
fi

if [[ $flashrom -gt 0 ]]; then
  echo "============FLASH NEW ROM============"
  esptool.py --port /dev/ttyUSB0 write_flash 0x00000 ../bin/eagle.flash.bin 0x40000 ../bin/eagle.irom0text.bin;
fi

if [[ $runuart -gt 0 ]]; then
  echo "============RUN UART=============="
  miniterm.py /dev/ttyUSB0 115200;
fi

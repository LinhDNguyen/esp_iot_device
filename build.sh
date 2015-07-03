#!/bin/bash

export PATH=`pwd`/../../xtensa-lx106-elf/bin:$PATH
touch user/user_main.c

if [[ "x${1}x" == "xx" ]]; then
  make COMPILE=gcc BOOT=none APP=0 SPI_SPEED=40 SPI_MODE=QIO SPI_SIZE_MAP=0
else
  if [[ `expr index "$1" 1` > 0 ]]; then
    make COMPILE=gcc BOOT=new APP=1 SPI_SPEED=40 SPI_MODE=QIO SPI_SIZE_MAP=2 OTAUPGRADE=1
  elif [[ `expr index "$1" 2` > 0 ]]; then
    make COMPILE=gcc BOOT=new APP=2 SPI_SPEED=40 SPI_MODE=QIO SPI_SIZE_MAP=2 OTAUPGRADE=1
  else
    make COMPILE=gcc BOOT=none APP=0 SPI_SPEED=40 SPI_MODE=QIO SPI_SIZE_MAP=0 OTAUPGRADE=0
  fi

fi

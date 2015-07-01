#!/bin/bash
flashrom=1
flashblank=0
flashconfig=0
runuart=1
usecom=/dev/ttyUSB0
romnumber=0

while [[ $# > 0 ]]
do
  key="$1"

  case $key in
    -h|--help)
    echo "Flash ESP8266 and run miniterm"
    echo "Command: flash.sh [options]"
    echo "Options:"
    echo "  -h|--help: print this help"
    echo "  -f|--flash <0|1>: enable flash new rom"
    echo "  -u|--runuart <0|1>: enable run miniterm after flash"
    echo "  -c|--com <COM2>: specify uart name (COMx on windows and /dev/ttyUSB0 on Linux)"
    echo "  -r|--romnumber <0|1|2>: specify the rom number"
    echo "     0: normal rom (without OTA)"
    echo "     1: OTA rom user slot 1"
    echo "     2: OTA rom user slot 2"
    echo "Example: flash.sh -c COM3 -r 2"
    exit 0
    ;;
    -f|--flash)
    flashrom="$2"
    shift # past argument
    ;;
    -u|--runuart)
    runuart="$2"
    shift # past argument
    ;;
    -c|--com)
    usecom="$2"
    shift # past argument
    ;;
    -r|--romnumber)
    romnumber="$2"
    shift # past argument
    ;;
    --default)
    flashrom=1
    runuart=1
    romnumber=0
    ;;
    *)
            # unknown option
    ;;
  esac
shift # past argument or value
done
echo "flashrom    ${flashrom}"
echo "runuart     ${runuart}"
echo "usecom      ${usecom}"
echo "romnumber   ${romnumber}"

if [[ "x${flashrom}x" != "x0x" ]]; then
  echo "============FLASH NEW ROM - ${romnumber}============"
  if [[ $romnumber == 0 ]]; then
    ./esptool.py --port ${usecom} write_flash 0x00000 ../bin/eagle.flash.bin 0x40000 ../bin/eagle.irom0text.bin;
  elif [[ $romnumber == 1 ]]; then
    ./esptool.py --port ${usecom} write_flash 0x00000 ../bin/boot_v1.4\(b1\).bin 0x01000 ../bin/upgrade/user1.1024.new.2.bin;
  elif [[ $romnumber == 2 ]]; then
    ./esptool.py --port ${usecom} write_flash 0x00000 ../bin/boot_v1.2.bin 0x81000 ../bin/upgrade/user2.1024.new.2.bin;
  fi
fi

if [[ $runuart -gt 0 ]]; then
  echo "============RUN UART=============="
  ./miniterm.py ${usecom} 115200;
fi

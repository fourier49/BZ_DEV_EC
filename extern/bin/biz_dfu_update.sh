#!/bin/bash

DFU_DEVICE=0483:df11
BIZ_DFUBOOT=0483:df11
DFU_RO_IMG=extern/release/RO_DfuBoot.bin

BOARD=$1
RW_IMG=build/$BOARD/ec.RW.bin

ADDR=0x08000000

if ! lsusb | grep $DFU_DEVICE > /dev/null 2>&1; then
	echo "The BizLink Type-C Device is not in DFU mode"
	exit -1
fi

if [ ! -f $DFU_RO_IMG ]; then
	echo "BizLink DFU_BOOT image doesn't exist."
	echo "Please check this path: $DFU_RO_IMG"
	exit -1
fi

if [ ! -f $RW_IMG ]; then
	echo "BizLink App FW image doesn't exist."
	echo "Please check this path: build/xxx/ec.RW.bin"
	exit -1
fi

# The least required version: dfu-util 0.7
DFU_UTIL_VER=$(dfu-util -V | head -1 | sed -e 's/dfu-util 0\.//')
if [ $DFU_UTIL_VER -lt 7 ]; then 
	echo "Your DFU_UTIL is too old: $(dfu-util -V | head -1)"
	echo "Please execute: sudo cp <Chromium-ROOT>/chroot/usr/bin/dfu-util /usr/bin/dfu-util"
	exit -1
fi

IMG=/tmp/dfu_${BOARD}.bin
cat $DFU_RO_IMG $RW_IMG > ${IMG}
SIZE=$(wc -c ${IMG} | cut -d' ' -f1)

sudo dfu-util -d ${DFU_DEVICE} -a 0 -s ${ADDR} -D ${IMG} -R
echo -e "\n============================================================="
if [ $? -eq 0 ]; then
	echo "DONE"
else
	echo "unlock the protected FLASH"
	sleep 2
	sudo dfu-util -d ${DFU_DEVICE} -a 0 -s "${ADDR}:${SIZE}:force:unprotect" -D ${IMG}
	sleep 2
	sudo dfu-util -d ${DFU_DEVICE} -a 0 -s "${ADDR}:${SIZE}" -D "${IMG}"

	echo -e "\n============================================================="
	echo "Please re-enter into DFU mode for the BizLink Type-C Device"
fi


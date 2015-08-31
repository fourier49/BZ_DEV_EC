#!/bin/bash

BOARD=$1

if id | grep chronos  > /dev/null 2>&1; then
	make clobber
	make -j BOARD=$BOARD USE_GIT_DATE=1
fi
if [ ! -f build/$BOARD/ec.RW.bin ]; then
	echo -e "The board $BOARD hasn't been built"
	exit 1
fi

PID=`egrep -r '^#define *CONFIG_USB_PID'     board/$BOARD | head -1 | sed -e 's/.*_USB_PID *\([^ ]*\).*/\1/' -e 's/0x//'`
DID=`egrep -r '^#define *CONFIG_USB_BCD_DEV' board/$BOARD | head -1 | sed -e 's/.*_USB_BCD_DEV *\([^ ]*\).*/\1/' -e 's/0x//'`

if egrep -r '^#define *BIZCFG_ENTER_DFU_VIA_BILLBOARD' board/$BOARD > /dev/null 2>&1; then
	echo -e "\n\nRelease FW with Bizlink DFU/BOOT mode, PID=$PID DID=$DID"
	if egrep -r '^#define *BIZCFG_BILLBOARD_ALWAYS_CONNECT' board/$BOARD > /dev/null 2>&1; then
		echo -e "USB/BB always ON, can update on any Windows"
	else
		echo -e "can only be updated on non-Alt-Mode Windows"
	fi
	cat extern/release/RO_DfuBoot.bin build/$BOARD/ec.RW.bin > /tmp/ec-rel.$BOARD.bin
else
	echo -e "\n\nRelease FW with Google GFU VDM mode, PID=$PID DID=$DID"
	cp -v build/$BOARD/ec.bin /tmp/ec-rel.$BOARD.bin
fi

echo -e "\nSHA1: `sha1sum /tmp/ec-rel.$BOARD.bin`\n"
SHA1=`sha1sum /tmp/ec-rel.$BOARD.bin | cut -b 1-8`
cp -v /tmp/ec-rel.$BOARD.bin /tmp/ec-rel.$BOARD.ID-$PID-$DID.`date +%Y-%m%d`.$SHA1.bin


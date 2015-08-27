#!/bin/bash

BOARD=$1

if [ ! -f build/$BOARD/ec.RW.bin ]; then
	echo -e "The board $BOARD hasn't been built"
	exit 1
fi

if egrep -r '^#define *BIZCFG_ENTER_DFU_VIA_BILLBOARD' board/$BOARD > /dev/null 2>&1; then
	echo -e "Release FW with Bizlink DFU/BOOT mode"
	cat extern/release/RO_DfuBoot.bin build/$BOARD/ec.RW.bin > /tmp/ec.$BOARD-rel.bin
else
	echo -e "Release FW with Google GFU VDM mode"
	cp -v build/$BOARD/ec.bin /tmp/ec.$BOARD-rel.bin
fi


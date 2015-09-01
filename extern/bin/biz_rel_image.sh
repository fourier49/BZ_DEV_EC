#!/bin/bash
#-------------------------------------------------------------------------------------------
# Usage:
#	biz_rel_image.sh <board>   : to build specific <board>
#	biz_rel_image.sh           : to build all BizLink boards (contains biz_board.conf)
# Env Var:
#	RELBIN_DIR                 : the folder where the generated binary images will be stored
#-------------------------------------------------------------------------------------------

function build_image()
{
	BOARD=$1
	BINDIR=$RELBIN_DIR/$BOARD
	mkdir -p $BINDIR

	if id | grep chronos  > /dev/null 2>&1; then
		make clobber > /dev/null
		make -j BOARD=$BOARD USE_GIT_DATE=1 > /dev/null
	fi
	if [ ! -f build/$BOARD/ec.RW.bin ]; then
		echo -e "The board $BOARD hasn't been built"
		exit 1
	fi

	PID=`egrep -r '^#define *CONFIG_USB_PID'     board/$BOARD | head -1 | sed -e 's/.*_USB_PID *\([^ ]*\).*/\1/' -e 's/0x//'`
	DID=`egrep -r '^#define *CONFIG_USB_BCD_DEV' board/$BOARD | head -1 | sed -e 's/.*_USB_BCD_DEV *\([^ ]*\).*/\1/' -e 's/0x//'`
	CHIP=`egrep -r '^#define *CONFIG_CHIP_INFO'  board/$BOARD | head -1 | sed -e 's/.*_CHIP_INFO *\([^ ]*\).*/\1/' -e 's/0x//'`

	if [ -n "$CHIP" ]; then
		ID=${BOARD}_${CHIP}.ID-$PID-$DID
	else
		ID=${BOARD}.ID-$PID-$DID
	fi

	echo -e "\n\n$(printf '=%.0s' {1..80})"
	if egrep -r '^#define *BIZCFG_ENTER_DFU_VIA_BILLBOARD' board/$BOARD > /dev/null 2>&1; then
		UPDATE_MODE="DFU"
		echo -e "Bizlink DFU/BOOT mode\t\t BOARD=$BOARD PID=$PID DID=$DID CHIP=$CHIP \n$(printf '=%.0s' {1..80})"
		if egrep -r '^#define *BIZCFG_BILLBOARD_ALWAYS_CONNECT' board/$BOARD > /dev/null 2>&1; then
			echo -e "USB/BB always ON, can update on any Windows"
		else
			echo -e "can only be updated on non-Alt-Mode Windows"
		fi
		cat extern/release/RO_DfuBoot.bin build/$BOARD/ec.RW.bin > /tmp/ec-rel.bin
	else
		UPDATE_MODE="GFU"
		echo -e "Google GFU VDM mode\t\t BOARD=$BOARD PID=$PID DID=$DID CHIP=$CHIP \n$(printf '=%.0s' {1..80})"
		cp -v build/$BOARD/ec.bin /tmp/ec-rel.bin
	fi

	echo -e "SHA1: `sha1sum /tmp/ec-rel.bin`"
	SHA1=`sha1sum /tmp/ec-rel.bin | cut -b 1-8`
	mv /tmp/ec-rel.bin $BINDIR/ec-$UPDATE_MODE.${ID}.D-`date +%Y%m%d`.$SHA1.bin

	cp build/$BOARD/ec.RW.bin $BINDIR/ec.RW.`date +%Y%m%d`.bin
	cp build/$BOARD/ec.bin    $BINDIR/ec.`date +%Y%m%d`.bin
}
 
#--------------------------------------------------------------------------
# RELBIN_DIR: will automatically prepend with its GIT BRANCH path
# default: /tmp/rel, /tmp/master, /tmp/hp
if [ -z "$RELBIN_DIR" ]; then
	RELBIN_DIR=/tmp
fi
RELBIN_DIR=$RELBIN_DIR/`git status | head -1 | cut -d '/' -f 2`

echo -e "\nOutput folder: $RELBIN_DIR"

#--------------------------------------------------------------------------
if [ -n "$1" ]; then
	build_image $1
	exit 0
fi

for bb in `ls board/`; do
	if [ -f board/$bb/biz_board.conf ]; then
		build_image $bb
	fi
done

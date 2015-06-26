
srec_cat "..\STM32F0x2_USB-FS-Device_Lib V1.0.0\Projects\DFU\MDK-ARM\STM32F072B-EVAL\STM32F072B-EVAL.hex"  -intel -crop 0x8010000 0x801FFFC  --Checksum_Negative_Little_Endian 0x801FFFC 4 4 -Output .\DFUBoot_Check.hex -intel

srec_cat .\DFUBoot_Check.hex -intel -offset -0x8010000 -o .\RW_DfuBoot.bin -binary


srec_cat .\DFUBoot.hex  -intel -crop 0x8000000 0x800FFFC  --Checksum_Negative_Little_Endian 0x800FFFC 4 4 -Output .\DFUBoot_Check.hex -intel

srec_cat .\DFUBoot_Check.hex -intel -offset -0x8000000 -o .\RO_DfuBoot.bin -binary

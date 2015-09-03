
..\..\..\..\tool\srec_cat ".\STM32F072B-EVAL\STM32F072B-EVAL.hex"  -intel -crop 0x8000000 0x8004FFC  --Checksum_Negative_Little_Endian 0x8004FFC 4 4 -Output "D:\SoftwareInstall\Bizlink_Software_Installed\Bz_DFU_drivers\DfuSe_v3.0.5_Bin\DFUBoot_Check.hex" -intel

..\..\..\..\tool\srec_cat "D:\SoftwareInstall\Bizlink_Software_Installed\Bz_DFU_drivers\DfuSe_v3.0.5_Bin\DFUBoot_Check.hex" -intel -offset -0x8000000 -o "D:\SoftwareInstall\Bizlink_Software_Installed\Bz_DFU_drivers\DfuSe_v3.0.5_Bin\RO_DfuBoot.bin" -binary

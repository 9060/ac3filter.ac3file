@echo off
echo ------------------------
echo Register debug version
regsvr32 /s /u debug\ac3file.ax
regsvr32 /s debug\ac3file.ax

# Zmodem-CP-M
This repository is intended to foster a RetroBrewComputers community effort to port the original Zmodem source code for Unix to CP/M so everyone can use Zmodem for file transfers and modify source code as needed for their specific platforms.  Binary programs for Zmodem (sz & rz) are available but without source code available making modifications extremely difficult if not impossible.

The proposed environment and toolset for this effort is z88dk.  The plan is to build the CP/M Zmodem application using the following steps

zcc +cpm sz.c -create-app

zcc +cpm rz.c -create-app

z88dk can be obtained here for Linux and Windows  https://z88dk.org/site/

Due to the age of the original Zmodem source files for Unix (1986) many of the constructs used in them are obsolete.  Files which many years ago used to be commonly available are no longer available so the code will need to be adjusted or suitable replacements found.  For example, the code "includes" both termio.h and signal.h which appear to be obsolete based on the original SysIII/SysV Unix.  Possible replacements from the era are included in the file set but do not appear to be correct.

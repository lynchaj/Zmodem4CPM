# Zmodem-CP-M
This repository is intended to foster a RetroBrewComputers community effort to port the original Zmodem source code for Unix
to CP/M so everyone can use Zmodem for file transfers and modify source code as needed for their specific platforms.
Binary programs for Zmodem (sz & rz) are available but without source code available making modification extremely difficult
if not impossible.

The proposed environment and toolset for this effort is z88dk.  The plan is to build the CP/M Zmodem application using the
following steps

zcc +cpm sz.c -create-app
zcc +cpm rz.c -create-app

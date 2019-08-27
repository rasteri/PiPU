#!/bin/sh

export HOME='/root'
mkdir /boot
mount /dev/mmcblk0p1 /boot
SDL_VIDEODRIVER=dummy chocolate-doom -iwad /boot/DOOM.WAD
SDL_VIDEODRIVER=dummy chocolate-doom -iwad /boot/DOOM2.WAD
SDL_VIDEODRIVER=dummy chocolate-doom -iwad /opt/PiPU/DOOM1.WAD
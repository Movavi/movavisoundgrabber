#! /bin/sh

KEXTNAME=MovaviSoundTap.kext

if [ ! -d $KEXTFULL ]; then
    echo 'NO KEXT'
    exit 1
fi

sudo kextunload $KEXTNAME
kextstat | grep SoundTap
kextstat | grep flower

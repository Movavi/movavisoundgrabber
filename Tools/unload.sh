#! /bin/sh

KEXTNAME=MovaviSoundGrabber.kext

if [ ! -d $KEXTFULL ]; then
    echo 'NO KEXT'
    exit 1
fi

sudo kextunload $KEXTNAME
kextstat | grep SoundGrabber
kextstat | grep flower

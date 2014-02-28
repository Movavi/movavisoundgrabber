#! /bin/sh

KEXTPATH=Build
KEXTNAME=MovaviSoundGrabber.kext
KEXTFULL=$KEXTPATH/$KEXTNAME

if [ ! -d $KEXTFULL ]; then
    echo 'NO KEXT'
    exit 1
fi

sudo rm -rf $KEXTNAME
sudo cp -r $KEXTFULL .
sudo chown -R root:wheel $KEXTNAME
sudo chmod -R 0755 $KEXTNAME
sudo kextload -tv $KEXTNAME
kextstat | grep SoundGrabber
kextstat | grep flower

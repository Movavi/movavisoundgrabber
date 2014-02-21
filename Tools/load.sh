#! /bin/sh

KEXTPATH=Build
KEXTNAME=MovaviSoundTap.kext
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
kextstat | grep SoundTap
kextstat | grep flower

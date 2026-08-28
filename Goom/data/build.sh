#!/bin/bash

echo "Building Game.001"

cd "`dirname $0`"
. config.sh $1

# rm -fr $BUILD_DIR
mkdir -p "$BUILD_DIR/gfx"
mkdir -p "$BUILD_DIR/sfx"
mkdir -p "$BUILD_DIR/music"
mkdir -p gfx-$1
mkdir -p gfx-tmp

DEF=$1

# echo "Build images"
python3 build-images.py $DEF $1 # > $BUILD_DIR/Description.gsl

# echo "Copy fonts"
cp gfx-src/numberfont.ttf gfx-$1/
cp gfx-src/MyriadWebPro.ttf gfx-$1/
# cp gfx-src/Suplex.ttf gfx-$1/

# echo "Copy images"
# cp gfx/*.png $BUILD_DIR/gfx/

# Copy SVG files too
# cp gfx/*.svg $BUILD_DIR/gfx/

# Make PVR (disabled, because there are no performance issues for now, so there is no need to reduce visual quality)
#	/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/texturetool -e PVRTC -f pvr -o $BUILD_DIR/gfx/all.png.pvr $BUILD_DIR/gfx/all.png
#	rm -f $BUILD_DIR/gfx/all.png

# Install JPEG
# cp gfx/*.jpg $BUILD_DIR/gfx/

# Make PVR (disabled, because there are no performance issues for now, so there is no need to reduce visual quality)
#for f in gfx/*-small.png
#do
#	test -f $f.pvr || /Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/texturetool -e PVRTC -f pvr -o $f.pvr $f
	# rm -f $BUILD_DIR/gfx/all.png
#done

echo "Install images, sound, music and locales to $BUILD_DIR"
for snd in \
	tick.wav \
    heartbeat.wav
do
	install_sound sfx/$snd $BUILD_DIR/sfx/$snd
done

# for music in \
#	chicken.xm
# do
#	install_music music/$music $BUILD_DIR/music/$music
# done

# rsync --modify-window=10 -C -c -rv \
#	--exclude '.*' \
#	--delete-excluded \
#   --delete \
#	locale "$BUILD_DIR/"

rsync --modify-window=10 -C -c -rv \
    --exclude '.*' \
    --delete-excluded \
    --delete \
    gfx-$1/ "$BUILD_DIR/gfx"

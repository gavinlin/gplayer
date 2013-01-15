#!/bin/sh
# Author: Cedric Fung (wolfplanet@gmail.com)

SYSROOT=$ANDROID_NDK/platforms/android-14/arch-arm
TOOLCHAIN=`echo $ANDROID_NDK/toolchains/arm-linux-androideabi-4.4.3/prebuilt/linux-x86`
export PATH=$TOOLCHAIN/bin:$PATH
FLAGS="--target-os=linux --cross-prefix=arm-linux-androideabi- --arch=arm --cpu=armv7-a"
FLAGS="$FLAGS --sysroot=$SYSROOT"
FLAGS="$FLAGS --disable-avdevice --disable-static --enable-shared --disable-ffplay --disable-doc --disable-ffmpeg --disable-ffprobe --disable-ffserver --disable-avfilter --disable-encoders --disable-muxers --disable-filters --disable-devices --enable-version3 --enable-asm  --enable-neon --enable-libstagefright-h264"

EXTRA_CFLAGS="-I$ANDROID_NDK/sources/cxx-stl/system/include"

#EXTRA_CFLAGS="$EXTRA_CFLAGS -march=armv7-a -mfloat-abi=softfp -mfpu=neon"
EXTRA_CFLAGS="$EXTRA_CFLAGS -march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16"
EXTRA_LDFLAGS="-Wl,--fix-cortex-a8"
EXTRA_CXXFLAGS="-Wno-multichar -fno-exceptions -fno-rtti"
ABI="armeabi-v7a"

echo $FLAGS --extra-cflags="$EXTRA_CFLAGS" --extra-ldflags="$EXTRA_LDFLAGS" --extra-cxxflags="$EXTRA_CXXFLAGS" > info.txt
./configure $FLAGS --extra-cflags="$EXTRA_CFLAGS" --extra-ldflags="$EXTRA_LDFLAGS" --extra-cxxflags="$EXTRA_CXXFLAGS" | tee configuration.txt
#[ $PIPESTATUS == 0 ] || exit 1

#`cat libavcodec/avcodec.h | libavcodec/codec_names.sh config.h libavcodec/codec_names.h`

#!/bin/bash
set -e

mkdir -p native/macos/debug
cd native/macos/debug
cmake ../../.. -DCMAKE_BUILD_TYPE=Debug -DXT_ENABLE_ASIO=0 -DXT_ENABLE_WASAPI=0 -DXT_ENABLE_DSOUND=0 -DXT_ENABLE_CORE_AUDIO=1 -DXT_ENABLE_PULSE=0 -DXT_ENABLE_ALSA=0 -DXT_ENABLE_JACK=0
make
cd ../../..
cp ../dist/core/xt/Debug/libxt-audio.dylib ../dist/cpp/sample/Debug/libxt-audio.dylib || :

mkdir -p native/macos/release
cd native/macos/release
cmake ../../.. -DCMAKE_BUILD_TYPE=Release -DXT_ENABLE_ASIO=0 -DXT_ENABLE_WASAPI=0 -DXT_ENABLE_DSOUND=0 -DXT_ENABLE_CORE_AUDIO=1 -DXT_ENABLE_PULSE=0 -DXT_ENABLE_ALSA=0 -DXT_ENABLE_JACK=0
make
cd ../../..
cp ../dist/core/xt/Release/libxt-audio.dylib ../dist/cpp/sample/Release/libxt-audio.dylib || :
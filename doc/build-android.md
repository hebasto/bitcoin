ANDROID BUILD NOTES
======================

This guide describes how to build and package the `bitcoin-qt` GUI for Android on Linux and macOS.


## Dependencies

Before proceeding with an Android build one needs to get the [Android SDK](https://developer.android.com/studio) and use the "SDK Manager" tool to download the NDK and one or more "Platform packages" (these are Android versions and have a corresponding API level).

The minimum supported Android NDK version is [r23](https://github.com/android/ndk/wiki/Changelog-r23).

In order to build `ANDROID_API_LEVEL` (API level corresponding to the Android version targeted, e.g. Android 9.0 Pie is 28 and its "Platform package" needs to be available) and `ANDROID_TOOLCHAIN_BIN` (path to toolchain binaries depending on the platform the build is being performed on) need to be set.

API levels from 24 to 29 have been tested to work.

If the build includes Qt, environment variables `ANDROID_SDK` and `ANDROID_NDK` need to be set as well but can otherwise be omitted.
This is an example command for a default build with no disabled dependencies:

    ANDROID_SDK=/home/user/Android/Sdk ANDROID_NDK=/home/user/Android/Sdk/ndk-bundle make HOST=aarch64-linux-android ANDROID_API_LEVEL=28 ANDROID_TOOLCHAIN_BIN=/home/user/Android/Sdk/ndk-bundle/toolchains/llvm/prebuilt/linux-x86_64/bin


## Building and packaging

After the depends are built configure with one of the resulting prefixes and run `make && make apk` in `src/qt`.



## UPDATE

1. Qt 6.7 supports Android 6 - 13 (API 23 - 33).

Qt 6.8 LTS supports Android 9 - 16 (API 28 - 37).


https://www.qt.io/blog/qt-for-android-supported-versions-guidelines


https://doc.qt.io/qt-6/whatsnew66.html:

> Updated Android target SDK level to 33 to match Play Store requirement for 2023.

`ANDROID_API_LEVEL=33` is OK.

Tools: https://dl.google.com/android/repository/build-tools_r33.0.3-linux.zip



2. https://doc.qt.io/qt-6/whatsnew67.html

https://github.com/android/ndk/wiki/Changelog-r26

NDK version: r26b (26.1.10909125)
https://dl.google.com/android/repository/android-ndk-r26b-linux.zip

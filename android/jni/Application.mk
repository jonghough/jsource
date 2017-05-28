# this file will be overrided by caller's Application.mk if any
APP_OPTIM=release
APP_MODULES := libj jconsole jconsole-nopie libtsdll libjpcre netdefs netdefs-nopie hostdefs hostdefs-nopie
# APP_MODULES := blas libf2c cholrl lapack
# use ndk r14 gcc-4.9 to build armeabi and armeabi-v7a
# APP_ABI := armeabi armeabi-v7a
APP_ABI := x86 x86_64 arm64-v8a
# will be enable inside jconsole, it is easier to enable than disable
APP_PIE := false

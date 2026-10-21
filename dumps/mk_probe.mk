# Parses jni/Android.mk without an NDK: stub the ndk-build glue, then dump what
# the gspace1 module would compile and where it would look for headers.
# Usage: make -f dumps/mk_probe.mk -C src/main/jni
NDK_ROOT := /nonexistent

TARGET_ARCH_ABI := arm64-v8a
CLEAR_VARS :=
PREBUILT_STATIC_LIBRARY :=
BUILD_STATIC_LIBRARY :=
BUILD_SHARED_LIBRARY :=

# ndk-build's my-dir macro, stubbed to the directory make was started in.
my-dir = $(CURDIR)

include Android.mk

probe:
	@echo "INCLUDES: $(LOCAL_C_INCLUDES)"
	@echo "SOURCES: $(LOCAL_SRC_FILES)"

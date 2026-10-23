APP_ABI             := arm64-v8a
APP_PLATFORM        := android-21
APP_STL             := c++_static
APP_OPTIM           := release
APP_THIN_ARCHIVE    := true
APP_PIE 		    := true
APP_STRIP_MODE      := none

ifneq ($(strip $(NDK_LIBS_OUT)),)
override NDK_LIBS_OUT := $(abspath $(NDK_LIBS_OUT))
endif
ifneq ($(strip $(NDK_APP_LIBS_OUT)),)
NDK_APP_LIBS_OUT := $(abspath $(NDK_APP_LIBS_OUT))
endif


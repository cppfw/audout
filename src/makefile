include prorab.mk

$(eval $(call prorab-config, ../config))

this_name := audout

this_soname := $(shell cat $(d)/soname.txt)

this_srcs += $(call prorab-src-dir, .)

ifeq ($(os), linux)
    this_ldlibs += -lpthread -lnitki -lopros -lpulse-simple # -lasound
else ifeq ($(os), windows)
    this_ldlibs += -lnitki -lopros -ldsound
else ifeq ($(os), macosx)
    this_ldlibs += -framework AudioUnit
endif

$(eval $(prorab-build-lib))

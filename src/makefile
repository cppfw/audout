include prorab.mk
include prorab-license.mk
include prorab-clang-format.mk
include prorab-install-dbgsrc.mk

$(eval $(call prorab-config, ../config))

this_src_dir := audout

this_name := $(this_src_dir)$(this_dbg)

this_soname := $(shell cat $(d)/soname.txt)

this_srcs += $(call prorab-src-dir, $(this_src_dir))

ifeq ($(os), linux)
    this_ldlibs += -l nitki$(this_dbg)
    this_ldlibs += -l opros$(this_dbg)

    this_ldlibs += -l pthread
    this_ldlibs += -l pulse-simple
#    this_ldlibs += -lasound
else ifeq ($(os), windows)
    this_ldlibs += -l nitki$(this_dbg)
    this_ldlibs += -l opros$(this_dbg)

    this_ldlibs += -ldsound
else ifeq ($(os), macosx)
    this_ldlibs += -framework AudioUnit
endif

this_ldlibs += -l utki$(this_dbg)

$(eval $(prorab-build-lib))

this_license_file := ../LICENSE
$(eval $(prorab-license))

$(eval $(prorab-install-dbgsrc))

$(eval $(prorab-clang-format))

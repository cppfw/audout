include prorab.mk
include prorab-test.mk

$(eval $(call prorab-config, ../../config))

this_name := tests

this_srcs := $(call prorab-src-dir, .)

this_libaudout := ../../src/out/$(c)/libaudout$(dot_so)

ifeq ($(os),linux)
    this_ldlibs += -lpthread
endif

this_ldlibs += $(this_libaudout)
this_ldlibs += -lm -lutki

this_no_install := true

$(eval $(prorab-build-app))

$(eval $(call prorab-depend, $(prorab_this_name), $(this_libaudout)))

this_run_name := $(notdir $(abspath $(d)))
this_test_cmd := $(prorab_this_name)
this_test_deps := $(prorab_this_name)
this_test_ld_path := $(dir $(this_libaudout))
$(eval $(prorab-run))

$(eval $(call prorab-include, ../../src/makefile))

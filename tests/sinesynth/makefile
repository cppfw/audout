include prorab.mk
include prorab-test.mk

$(eval $(call prorab-config, ../../config))

this_name := tests

this_srcs := $(call prorab-src-dir, .)

ifeq ($(os),linux)
    this_ldlibs += -l pthread
endif

this_ldlibs += -lm

this_ldlibs += -l utki$(this_dbg)

this_ldlibs += ../../src/out/$(c)/libaudout$(this_dbg)$(dot_so)

this_no_install := true

$(eval $(prorab-build-app))

this_run_name := $(notdir $(abspath $(d)))
this_test_cmd := $(prorab_this_name)
this_test_deps := $(prorab_this_name)
this_test_ld_path := ../../src/out/$(c)
$(eval $(prorab-run))

$(eval $(call prorab-include, ../../src/makefile))

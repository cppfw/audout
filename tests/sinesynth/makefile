include prorab.mk

$(eval $(call prorab-config, ../../config))

this_name := tests

this_srcs := $(call prorab-src-dir, .)

this_libaudout := $(d)../../src/out/$(c)/libaudout$(dot_so)

ifeq ($(os),linux)
    this_ldlibs += -lpthread
endif

this_ldlibs += $(this_libaudout)
this_ldlibs += -lm -lutki

this_no_install := true

$(eval $(prorab-build-app))

ifeq ($(os),windows)
    this_run_cmd := (cd $(d) && cmd //C 'set PATH=../../src/out/$(c);%PATH% && out\$(c)\$$(notdir $$^)')
else ifeq ($(os),macosx)
    this_run_cmd := (cd $(d) && DYLD_LIBRARY_PATH=../../src/out/$(c) out/$(c)/$$(notdir $$^))
else ifeq ($(os),linux)
    this_run_cmd := (cd $(d) && LD_LIBRARY_PATH=../../src/out/$(c) out/$(c)/$$(notdir $$^))
else
    $(error "Unknown OS")
endif

.PHONY: test

define this_rules :=
run:: $(prorab_this_name)
$(.RECIPEPREFIX)@echo running $$^...
$(.RECIPEPREFIX)$(a)$(this_run_cmd)
endef
$(eval $(this_rules))

$(prorab_this_name): $(abspath $(this_libaudout))

$(eval $(call prorab-include, ../../src/makefile))

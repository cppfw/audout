include prorab.mk

this_name := tests

this_srcs := main.cpp

this_cxxflags += -Wall
this_cxxflags += -Wno-comment # no warnings on nested comments
this_cxxflags += -Werror
this_cxxflags += -fstrict-aliasing
this_cxxflags += -g
this_cxxflags += -std=c++11

ifeq ($(debug), true)
    this_cxxflags += -DDEBUG
else
    this_cxxflags += -O3
endif

ifeq ($(os),windows)
    this_ldlibs += $(d)../../src/build/libaudout$(dot_so)
    this_ldlibs += -lnitki -lopros -ldsound
else ifeq ($(os),macosx)
    this_ldlibs += $(d)../../src/build/libaudout$(dot_so)
    this_ldlibs += -framework AudioUnit
else ifeq ($(os),linux)
    this_ldlibs += -lopros -lnitki -lpthread -lpulse -lpulse-simple #-lasound
    this_ldlibs += $(d)../../src/build/libaudout$(dot_so)
endif

this_ldlibs += -lstdc++ -lm

this_no_install := true

$(eval $(prorab-build-app))


ifeq ($(os),windows)
    this_run_cmd := (cd $(d) && cmd //C 'set PATH=../../src/build;%PATH% && $$(notdir $$^)')
else ifeq ($(os),macosx)
    this_run_cmd := (cd $(d) && DYLD_LIBRARY_PATH=../../src/build ./$$(notdir $$^))
else ifeq ($(os),linux)
    this_run_cmd := (cd $(d) && LD_LIBRARY_PATH=../../src/build ./$$(notdir $$^))
else
    $(error "Unknown OS")
endif

.PHONY: test

define this_rules :=
run:: $(prorab_this_name)
$(.RECIPEPREFIX)@echo running $$^...
$(.RECIPEPREFIX)$(Q)$(this_run_cmd)

test::

endef
$(eval $(this_rules))


$(prorab_this_name): $(abspath $(d)../../src/build/libaudout$(dot_so))

$(eval $(call prorab-include, ../../src/makefile))

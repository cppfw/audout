include prorab.mk

this_name := tests

this_srcs := main.cpp

this_cxxflags += -Wall
this_cxxflags += -Wno-comment #no warnings on nested comments
this_cxxflags += -Werror
this_cxxflags += -fstrict-aliasing #strict aliasing!!!
this_cxxflags += -g
this_cxxflags += -std=c++11

ifeq ($(debug), true)
    this_cxxflags += -DDEBUG
else
    this_cxxflags += -O3
endif

ifeq ($(os),windows)
    this_ldlibs += $(d)../../src/libaudout$(dot_so)
    this_ldlibs += -lnitki -lopros -ldsound
else ifeq ($(os),macosx)
    this_ldlibs += $(d)../../src/libaudout$(dot_so)
    this_ldlibs += -framework AudioUnit
else ifeq ($(os),linux)
    this_ldlibs += -lopros -lnitki -lpthread -lpulse -lpulse-simple #-lasound
    this_ldlibs += $(d)../../src/libaudout$(dot_so)
endif

this_ldlibs += -lstdc++ -lm

this_no_install := true

$(eval $(prorab-build-app))

define this_rules
run:: $(prorab_this_name)
$(.RECIPEPREFIX)@echo running $$^...
$(.RECIPEPREFIX)$(Q)(cd $(d); LD_LIBRARY_PATH=../../src $$^)

test::

endef
$(eval $(this_rules))

#======= if Windows ========
ifeq ($(os),windows)

define this_rules
$(d)libaudout$(dot_so): $(abspath $(d)../../src/libaudout$(dot_so))
$(.RECIPEPREFIX)$(Q)cp $< $@

$(prorab_this_name): $(d)libaudout$(dot_so)

clean::
$(.RECIPEPREFIX)$(Q)rm -f $(d)libaudout$(dot_so)
endef
$(eval $(this_rules))

# ======= else ========
else

$(prorab_this_name): $(abspath $(d)../../src/libaudout$(dot_so))

endif
# ======== endif =============

$(eval $(call prorab-include, ../../src/makefile))

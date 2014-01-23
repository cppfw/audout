subdirs :=
subdirs += src
subdirs += tests

#build docs only from linux, because there is doxygen
ifeq ($(platform),windows)

else

subdirs += docs

endif


include ./targets.mk

$(possible_targets_list):
	@$(MAKE) $(subdirs) target=$@

.PHONY: $(subdirs)
$(subdirs):
	@$(MAKE) -C $@ $(target)


install:
	@./install.sh

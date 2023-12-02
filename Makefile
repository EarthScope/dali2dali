
.PHONY: all clean
all clean: libdali
	$(MAKE) -C src $@

.PHONY: libdali
libdali:
	$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY: install
install:
	@echo
	@echo "No install method"
	@echo "Copy the binary and documentation to desired location"
	@echo

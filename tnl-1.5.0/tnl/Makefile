default:
	@$(MAKE) -C tnl
	@$(MAKE) -C libtomcrypt
	@$(MAKE) -C master
	@$(MAKE) -C masterclient
	@$(MAKE) -C test

.PHONY: clean 

clean:
	@$(MAKE) -C tnl clean
	@$(MAKE) -C libtomcrypt clean
	@$(MAKE) -C master clean
	@$(MAKE) -C masterclient clean
	@$(MAKE) -C test clean

docs:
	@$(MAKE) -C docs


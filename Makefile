.PHONY: all clean 3ds wiiu

all: 3ds wiiu
	@echo "\033[92mDone!\033[0m"

3ds:
	@echo "\033[92mBuilding $@...\033[0m"
	@$(MAKE) --no-print-directory -C $(CURDIR)/3ds

wiiu:
	@echo "\033[92mBuilding $@...\033[0m"
	@$(MAKE) --no-print-directory -C $(CURDIR)/wiiu

clean:
	@echo "\033[92mCleaning $@...\033[0m"
	@$(MAKE) --no-print-directory -C $(CURDIR)/3ds clean
	@$(MAKE) --no-print-directory -C $(CURDIR)/wiiu clean

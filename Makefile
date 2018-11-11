.PHONY: clean All

All:
	@echo "----------Building project:[ NFA - Release ]----------"
	@cd "NFA" && "$(MAKE)" -f  "NFA.mk"
clean:
	@echo "----------Cleaning project:[ NFA - Release ]----------"
	@cd "NFA" && "$(MAKE)" -f  "NFA.mk" clean

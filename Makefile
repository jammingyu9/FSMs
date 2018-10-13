.PHONY: clean All

All:
	@echo "----------Building project:[ NFA - Debug ]----------"
	@cd "NFA" && "$(MAKE)" -f  "NFA.mk"
clean:
	@echo "----------Cleaning project:[ NFA - Debug ]----------"
	@cd "NFA" && "$(MAKE)" -f  "NFA.mk" clean

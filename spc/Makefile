

TARGET := spcunit
TASM := /c/tasm301/TASM.EXE -07 -b -l
DEPS := 

all: $(TARGET).obj

%.obj : %.asm $(DEPS)
	@echo assembling...
	@$(TASM) $<
	
clean:
	rm $(TARGET).obj

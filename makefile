
BASEOBJNAMES := hlslparser compiler optimizer common generator
OBJS := $(patsubst %,obj/%.obj,$(BASEOBJNAMES))

.PHONY: test
test: sltest.exe
	sltest

sltest.exe: $(OBJS) obj/test.obj
	link /nologo /out:$@ $^ /DEBUG

hlslparser.exe: $(OBJS) obj/main.obj
	link /nologo /out:$@ $^ /DEBUG

obj/%.obj: src/%.cpp src/hlslparser.hpp src/common.hpp src/compiler.hpp
	cl /nologo /Fo$@ /MDd /EHsc /D_DEBUG /Zi /c $<

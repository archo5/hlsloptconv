
BASEOBJNAMES := hlslparser compiler optimizer common generator
OBJS := $(patsubst %,obj/%.obj,$(BASEOBJNAMES))

.PHONY: tools test html5test
tools: sltest.exe hlsloptconv.exe
test: sltest.exe
	sltest
test5: sltest.exe
	sltest -t tests/500-intrin.hlsl
html5test: hlsloptconv.exe
	py runtests/html5-compile.py

sltest.exe: $(OBJS) obj/test.obj
	link /nologo /out:$@ $^ /DEBUG

hlsloptconv.exe: $(OBJS) obj/cli.obj
	link /nologo /out:$@ $^ /DEBUG

obj/%.obj: src/%.cpp src/hlslparser.hpp src/common.hpp src/compiler.hpp | obj
	cl /nologo /Fo$@ /MDd /EHsc /D_DEBUG /Zi /c $<

obj:
	mkdir obj

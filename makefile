
BASEOBJNAMES := hlslparser compiler optimizer common generator
OBJS := $(patsubst %,obj/%.obj,$(BASEOBJNAMES))

.PHONY: tools test html5test
tools: sltest.exe hlsloptconv.exe
test: sltest.exe
	sltest
test4: sltest.exe
	sltest -t tests/400-func.hlsl
test5: sltest.exe
	sltest -t tests/500-intrin.hlsl
html5test: hlsloptconv.exe
	py runtests/html5-compile.py
four: four.exe
	four

sltest.exe: $(OBJS) obj/test.obj
	link /nologo /out:$@ $^ /DEBUG

four.exe: $(OBJS) obj/four.obj
	link /nologo /out:$@ $^ /DEBUG user32.lib gdi32.lib msimg32.lib d3d9.lib d3d11.lib d3dcompiler.lib

hlsloptconv.exe: $(OBJS) obj/cli.obj
	link /nologo /out:$@ $^ /DEBUG

obj/%.obj: src/%.cpp src/hlslparser.hpp src/common.hpp src/compiler.hpp | obj
	cl /nologo /Fo$@ /MDd /EHsc /D_DEBUG /Zi /c $<

obj/%.obj: src/tools/%.cpp src/hlslparser.hpp src/common.hpp src/compiler.hpp | obj
	cl /nologo /Fo$@ /MDd /EHsc /D_DEBUG /Zi /c $<

obj:
	mkdir obj

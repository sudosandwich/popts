all: tests singlefile

tests:
	cd build; \
	cl -EHsc -Zi -MD -std:c++17 ../src/test.cpp ../src/main.cpp; \
	cd ..

singlefile:
	sed -e '/#[[:space:]]*include "opt.h"/{r src/opt.h' -e 'd}' src/opts.h > build/singleheader.h
	sed -i -e '/#[[:space:]]*include "opt.inl.h"/{r src/opt.inl.h' -e 'd}' build/singleheader.h
	sed -i -e '/#[[:space:]]*include "opts.inl.h"/{r src/opts.inl.h' -e 'd}' build/singleheader.h
	sed -i -e '/#[[:space:]]*include "typedefs.h"/{r src/typedefs.h' -e 'd}' build/singleheader.h
	clang-format -i -style file -fallback-style llvm build/singleheader.h
	mv build/singleheader.h include/popts.hpp

.PHONY: clean
clean:
	rm -rf build/*
	rm -rf include/*

all:
	cmake -Bbuild -H. $(shell test -d build || { echo -DCMAKE_BUILD_TYPE=RelWithDebInfo; if ninja --version >/dev/null 2>/dev/null; then echo -GNinja; fi; })
	cmake --build build
clean:
	cmake --build build --target clean
.PHONY: test
test: all
	ctest --build build -V

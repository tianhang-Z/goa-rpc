CMAKE_OPTIONS ?= -DCMAKE_BUILD_EXAMPLES=ON

format:
	find . -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.cc" -o -name "*.hpp" -o -name "*.cxx" \) -exec clang-format -i {} +

clean:
	rm -rf build

prepare: clean
	mkdir build &&  cd build  && cmake $(CMAKE_OPTIONS) .. 

build: prepare
	cd build && make  

install:
	cd build && sudo make install

all : build install
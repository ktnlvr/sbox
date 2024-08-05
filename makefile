run:
	mkdir -p build && cd build && cmake .. && CXX=clang++ cmake --build .
	
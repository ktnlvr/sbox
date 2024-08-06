run:
	mkdir -p build && cd build && cmake .. -D CMAKE_BUILD_TYPE=Debug && CXX=clang++ cmake --build . 

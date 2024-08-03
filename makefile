run:
	meson setup build
	meson compile -C build
	./build/sbox

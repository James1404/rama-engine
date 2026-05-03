init:
    @mkdir -p build
    @cd build
    @cmake ..

build:
    @cmake ./build

clean:
    @rm -rf build

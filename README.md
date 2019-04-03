# nstool

## History

- v1.0.0 @ 2018.07.01 - Support romfs
- v1.0.1 @ 2018.08.27 - Support nso
- v1.0.2 @ 2019.04.03 - Fix nso hash

## Platforms

- Windows
- Linux
- macOS

## Building

### Dependencies

- cmake
- libiconv
- openssl-devel / libssl-dev

### Compiling

- make 64-bit version
~~~
mkdir build
cd build
cmake -DUSE_DEP=OFF ..
make
~~~

- make 32-bit version
~~~
mkdir build
cd build
cmake -DBUILD64=OFF -DUSE_DEP=OFF ..
make
~~~

### Installing

~~~
make install
~~~

## Usage

~~~
nstool [option...] [option]...
~~~

## Options

See `nstool --help` messages.

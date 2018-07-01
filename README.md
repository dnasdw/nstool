# nstool

## History

- v1.0.0 @ 2018.07.01 - Support romfs

## Platforms

- Windows
- Linux
- macOS

## Building

### Dependencies

- cmake
- libiconv

### Compiling

- make 64-bit version
~~~
mkdir build
cd build
cmake ..
make
~~~

- make 32-bit version
~~~
mkdir build
cd build
cmake -DBUILD64=OFF ..
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

# xatslsp

Implementing Language Server for Xanadu

## Building

Please ensure you have a recent CMake. For Debian/Ubuntu, you can use
[the Kitware APT repository](https://apt.kitware.com/).

``` shell
mkdir -p .build && cd .build && cmake ..
make
```

## Running tests

In the build directory, run:

``` shell
make test
```

# OpenWS - A Window System Frame In OpenGL Programs

## Introduction

The Window System Frame Library (OpenWS) is a cross-platform, open-source C/C++ extension library. It provides an easy-use frame which supports mutiple-virtual-window processing in OpenGL programs.

## Dependences

OpenWS is based on glfw, glew and the headers and libraries of your system.

As it uses framebuffer to maintain windows, this library may doesn't support the old machines.

## Compiling OpenWS

OpenWS uses cmake to compile.

To build, please run following commands:
```bash
$ git clone https://github.com/H1KHC/openWS
$ mkdir .build && cd .build
$ cmake ..
$ make
```

And to make samples, run `cmake .. -DBUILD_SAMPLES=ON`


## Using OpenWS

Maybe you can have a look at the samples?

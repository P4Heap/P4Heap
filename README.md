# P4Heap

A general TopK framework to filter giant flows in order to increase the accuracy of various types of sketches.

## Build and Run 
To build the code, you need to install `clang`(at least version 10) first.

Type `make` in the shell in the root directory (which constains this README file), the compiler will compile the code and build the executable located at `build/exp`. You can alter the defination of macro `TARGET_EXEC` to change the name of the generated executable.

To run the code, you should obtain a dataset (such as [CAIDA](https://data.caida.org/datasets/passive-2018)) first. Type `make run` in the shell in the root directory, then the generated executable will be executed.

## Debug
To debug the code, you need to install `lldb` debuger first.

To generate the executable which constains extre infomation for debugging, type `make debug` in the shell. Then the desired executable will be built and placed at `debug/debug.out`.

To debug the code, type `make debug_run` in the shell, `lldb` debugger will launch `debug/debug.out` and ready for debugging. 

## Add more types of sketch
First of all, add the class decleration of the desired sketch in `src/include/sketch.h` inherited from abstract class `BaseSketch`.

Then, add `src/sketch/*.cpp` file to define the member functions of the new class. Make sure to override all the virtual functions of class BaseSketch.


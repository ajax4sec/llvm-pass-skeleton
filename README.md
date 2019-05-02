# llvm-pass-skeleton

A completely useless LLVM pass.

Build:

    $ cd llvm-pass-skeleton
    $ mkdir build
    $ cd build
    $ cmake ..
    $ make
    $ cd ..

Build2:

    $ gcc -c rtdetector.c 
    
    $ clang -Xclang -load -Xclang build/skeleton/libSkeletonPass.so -c example.c
    
    $ gcc ./example.o ../rtdetector.o

Run:
    $ ./a.out

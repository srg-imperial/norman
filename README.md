## Building
A minimally configured build using ninja:

```console
$ mkdir build
$ cd build
$ cmake -GNinja $PATH_TO_SOURCES
$ ninja       # build project
$ ninja test  # run tests
```

### Relevant CMake Variables
- `-DCMAKE_BUILD_TYPE`: set to a value that matches your use case, e.g., `Debug` or `RelWithDebInfo` ([upstream docs](https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html))
- `-DLLVM_DIR`: the location of your (preferred) LLVM installation, e.g., `$(llvm-config-${LLVM} --libdir)`
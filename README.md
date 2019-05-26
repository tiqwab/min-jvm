```
$ mkdir build
$ cd build
$ cmake ../
$ make
$ ctest # `--verbose` to see output too
```

### TODO

For run hello world written in Java:

- [ ] call static methods without arguments
  - [x] inst: `invokestatic`
  - [ ] inst: `istore_1`
    - implement stack to pass args and accept return value
    - where to store and load?
  - [ ] inst: `iload_1`
- [ ] call static methods with arguments
- [ ] call static methods of other classes

For run hello world written in Scala:

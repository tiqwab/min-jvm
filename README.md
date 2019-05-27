```
$ mkdir build
$ cd build
$ cmake ../
$ make
$ ctest # `--verbose` to see output too
```

### TODO

For run hello world written in Java:

- [x] call static methods without arguments
  - [x] inst: `invokestatic`
  - [x] inst: `istore_1`
    - [x] implement stack to pass args and accept return value
    - [x] where to store and load?
    - [x] implement push and pop
  - [x] inst: `iload_1`
- [x] call static methods with arguments
- [ ] call static methods of other classes

For run hello world written in Scala:

Others:

- [ ] Handle other types of method descriptor than Int (4.3.3)
- [ ] Handle ReturnDescriptor (4.3.3)

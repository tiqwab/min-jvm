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
- [x] call static methods of other classes

For run hello world written in Scala:

Others:

- [x] Support static fields
- [x] Initialization of class
- [x] Support instance creation
- [x] Support instance methods
- [x] Support instance fields
- [x] Initialization of instance
- [ ] Support static fields other than Int (particularly object references)
  - [x] object reference
  - [ ] string?
- [x] Introduce System.exit
- [x] Fix main function signature
- [ ] Try running codes compiled from Scala?
- [ ] String literal
- [ ] Create Class object (and let it manage static fields)
- [ ] Handle other types of method descriptor than Int (4.3.3)
- [ ] Handle ReturnDescriptor (4.3.3)
- [ ] Support Array
- [ ] Exceptions
  - e.g. NoClassDefFoundError (5.3)
- [ ] Lazy loading of class file
- [ ] How to prepare stdlib?
  - In OpenJDK (and Oracle JDK), rt.jar provides stdlib?
- [ ] Disable to execute main function having int as return type

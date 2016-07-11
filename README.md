## olta - Online Litmus Test Assembler

Working in progress / proof of concept assembler for memory model litmus tests.
At present this only supports a limited set of ARMv8 tests.

### Usage

    cd olta
    make
    ./olta <test-file>

Alternatively test files can be fed to the running tool on stdin and result will be output on stdout.

### Configuration

The tool allows extensive configuration and extenion of the test environment.
Please review the files in the `doc` directory.


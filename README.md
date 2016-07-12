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

### el0_pmu

In the `el0_pmu` directory you will find a kernel module which enables usermode access to parts of the ARMv8 PMU.  This can be used to enable cycle timing in the `olta` tool.

    cd el0_pmu
    make
    su insmod ./el0_pmu.ko

Usermode access to the PMU is revoked simply by removing the module from the running kernel.

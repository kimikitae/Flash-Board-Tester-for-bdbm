# BlueDBM Board Tester

## Overview

This program tests the BlueDBM flash board in the ZCU102 motherboard.

## Prerequisite

You must install the following packages.

```bash
sudo apt update -y
sudo apt install gcc g++ make libssl-dev libglib2.0-dev
```

## Build and Run

First of all, you have to move the root directory. Then, you can find
a file named `build.sh`. And type the below commands in your shell.

```bash
chmod +x ./build.sh
./build.sh
```

Now, you can get the execution tester program named `nohost.exe`
in the `project` directory.

If you want to test the board, you execute that file.

## Install library

You can install the library by using `./build.sh install`.
After executing this command, you can use the library without `-L` flags.
In other words, you can use a library like below.

```bash
g++ -D USER_MODE *.c -lmemio -lpthread -lcrypto
```

## How to use

You can test the device by tuning the below factors.

| name |path| contents |
|:----:|:---:|:---------|
| `WRITE_START` | `project/main.c` | start segment/block number |
| `WRITE_END` | `project/main.c` | last segment/block number (NOT INCLUDE) |

Additionally, you are able to manipulate the device information in the `project/include/settings.h`.

HOWEVER, you must also change parameters in the below files.

```bash
devices/common/dev_params.c
devices/nohost/dm_nohost.cpp
devices/nohost/dm_nohost_org.cpp
```

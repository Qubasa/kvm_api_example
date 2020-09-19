## Description

This is a simple (not feature complete) c++20 wrapper around the kvm api to start a vm in 16bit real mode
and run two instructions.

Uses all options of clangs ubsan for robustness.

## Build
```bash
$ nix-shell shell.nix # Dependencies
$ mkdir build
$ cd build
$ cmake ..
$ make
```

## Example output
```
DEBUG: Max mem slots 509
DEBUG: copy_to_mem guest_addr 0x0, found vm slot 0 guest_addr 0x0 end 0x2000
RIP: 0 RAX: 0
RIP: 0 RAX: 0
Starting vm RIP: 0 RAX: 0

RIP: 0 RAX: 0
Exited vm
RIP: 4 RAX: 42
Successfully executed vm with ax == 42
RIP: 4 RAX: 42
RIP: 4 RAX: 42
```


Linux kernel module that directly controls the LEDs of PS/2 keyboard (Num Lock, Caps Lock, and Scroll Lock).
Two basic routines are provided by the module: get led state and set led state. User programs must be able to access these routines. This can be done through sysfs, device ﬁles, or system calls.
running multiple user programs in parallel will lead to race conditions if the programs are concurrently accessing the LEDs through the module. Finally, applying kernel synchronization mechanisms (semaphores) to add atomicity to the module.

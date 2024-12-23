# Hardware Level emulator for the Zilog Z80 CPU
This is a Hardware Level Emulator for the Zilog Z80 Processor.

This emulator is created with the purpose of testing the functioning of the code to create a Z80 Emulator that will run on a Raspberry Pi PICO.

The emulator is currently in its initial state where we tested the concept and functionality.

By: Lincă Marius Gheorghe.

Pitești, Argeș, România, Europe.

https://github.com/LincaMarius

## About the project, brief description
The goal of this project is to perform hardware-level behavioral simulation of the Zilog Z80 processor.

This is the first stage of the project. After completion I will convert this code into the embedded version to emulate the Z80 processor on an STM32 microcontroller or Raspberry Pi Pico.

It also gives me the opportunity to practice and at the same time improve my Embedded C programming knowledge.

The code was tested and we obtained these results for a Debug level of 6.

![ Figure 1 ](/Pictures/Figure1.png)

And for a Debug level of value 1

![ Figure 2 ](/Pictures/Figure2.png)

## Version 0.4
This is the version in which we implemented all the Z80 processor instructions that transfer data between registers.

The source code for Version 0.4 is here: \
[main.c](https://github.com/LincaMarius/Z80_Emulator/blob/main/z80emu/Version_0_4/main.c)

For testing, a file containing Z80 source code is needed that is in the same location as the executable file and has the name ROM.bin. \
[ROM.bin](https://github.com/LincaMarius/Z80_Emulator/blob/main/z80emu/Version_0_4/ROM.bin)
## Version 0.3
This is the version where we implemented all the instructions for loading an immediate value into a register.

The source code for Version 0.3 is here: \
[main.c](https://github.com/LincaMarius/Z80_Emulator/blob/main/z80emu/Version_0_3/main.c)

For testing, a file containing Z80 source code is needed that is in the same location as the executable file and has the name ROM.bin. \
[ROM.bin](https://github.com/LincaMarius/Z80_Emulator/blob/main/z80emu/Version_0_3/ROM.bin)

## Version 0.2
The source code for Version 0.2 is here: \
[z80emu.c](https://github.com/LincaMarius/Z80_Emulator/blob/main/z80emu/Version_0_2/z80emu.c)

## Version 0.1
This is the initial version where we verified the implementation method of the concept
The source code for Version 0.1 is here: \
[z80emu.c](https://github.com/LincaMarius/Z80_Emulator/blob/main/z80emu/Version_0_1/z80emu.c)

For testing, a file containing Z80 source code is needed that is in the same location as the executable file and has the name ROM.bin. \
[ROM.bin](https://github.com/LincaMarius/Z80_Emulator/blob/main/z80emu/Version_0_1/ROM.bin)



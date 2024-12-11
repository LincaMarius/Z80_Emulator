#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/*
    This is a Hardware Level Emulator for the Zilog Z80 Processor.

    This emulator is created with the purpose of testing the functioning of the code
    to create a Z80 emulator that will run on a Raspberry Pi PICO.

    Copyright (c) 2024 Linca Marius Gheorghe

    Version 0.1
*/

int Debug = 1;
long Limit = 100;  // nr of tick to run


typedef union
{
    uint8_t r1;
    uint8_t r2;
    uint16_t rr;
} z80reg;

typedef struct
{
    int c:1;
    int n:1;
    int pv:1;
    int h:1;
    int z:1;
    int s:1;
    int res1:1;
    int res2:1;
} flags;

typedef struct z80status
{
    z80reg z_regs[9];
    uint16_t z_pc;
    uint16_t z_sp;
    uint8_t z_index[2];
    int8_t z_cycles;
    int8_t z_step;
	int8_t z_opcode;
	int8_t z_operand;

    flags F;

} z80status;

z80status z80;

#define A z80.z_regs[0].r1
#define F z80.z_regs[0].r2
#define AF z80.z_regs[0].rr
#define B z80.z_regs[1].r1
#define C z80.z_regs[1].r2
#define BC z80.z_regs[1].rr
#define D z80.z_regs[2].r1
#define E z80.z_regs[2].r2
#define DE z80.z_regs[2].rr
#define H z80.z_regs[3].r1
#define L z80.z_regs[3].r2
#define HL z80.z_regs[3].rr
#define A1 z80.z_regs[4].r1
#define F1 z80.z_regs[4].r2
#define AF1 z80.z_regs[4].rr
#define B1 z80.z_regs[5].r1
#define C1 z80.z_regs[5].r2
#define BC1 z80.z_regs[5].rr
#define D1 z80.z_regs[6].r1
#define E1 z80.z_regs[6].r2
#define DE1 z80.z_regs[6].rr
#define H1 z80.z_regs[7].r1
#define L1 z80.z_regs[7].r2
#define HL1 z80.z_regs[7].rr
#define I z80.z_regs[8].r1
#define R z80.z_regs[8].r2
#define IR z80.z_regs[8].rr

#define PC z80.z_pc
#define SP z80.z_sp
#define IX z80.z_index[0]
#define IY z80.z_index[1]
#define Cycles z80.z_cycles
#define Step z80.z_step
#define ZOpcode z80.z_opcode
#define Operand z80.z_operand

#define CF z80.F.c
#define NF z80.F.n
#define PVF z80.F.pv
#define HF z80.F.h
#define ZF z80.F.z
#define SF z80.F.s
#define RF1 z80.F.res1
#define RF2 z80.F.res2

uint16_t address;       // Address Bus
uint8_t data;           // Data Bus

// control signals
uint8_t _m1; 	        // out
uint8_t _mreq;          // out
uint8_t _ireq;          // out
uint8_t _rd;            // out
uint8_t _wr;            // out
uint8_t _rfsh;          // out
uint8_t _halt;          // out
uint8_t _wait;          // in
uint8_t _int;           // in
uint8_t _nmi;           // in
uint8_t _reset;         // in
uint8_t _busrq;         // in
uint8_t _busack;        // out
uint8_t _clk;           // in
//TO DO: make them boolean

void resetZ80(void) {
    if (Debug >= 1)
        printf("\nReset Z80 Emulator ");
    PC = 0;
    if (Debug >= 2)
        printf("-> PC = 0, ");
	SP = 0;
    if (Debug >= 2)
        printf(" SP = 0, ");
	Cycles = 0;
    if (Debug >= 2)
        printf(" Cycles = 0, ");
	Step = 0;
    if (Debug >= 2)
        printf(" Step = 0");
    printf("\nDebug Level = %d\n\n", Debug);
}

void SetFlags(void){
}

int8_t add (int8_t a, int8_t b){
    int16_t result = a + b;
    //if (result > 255)
        //CF = 1;

    return (result & 0xFF);
}

int8_t addc (int8_t a, int8_t b){
    int16_t result;
    //if (CF == 1)
        //result = a + b + 1;
    //else
        //result = a + b;

    return (result & 0xFF);
}

void emuZ80(void) {
    if (Debug >= 4)
        printf("\nOpcode = %d, ", ZOpcode);
    if (Debug >= 4)
        printf("Step = %d, ", Step);
    if (Debug >= 4)
        printf("Clk = %d, ", _clk);
    if (Debug >= 4)
        printf("PC = %d -- ", PC);


    //Machine Cycle M1 - Step T1 - Opcode Fetch
    if (Step == 0) {
    	if (_clk == 1) {
    		address = PC;
		    _m1 = 0;
            if (Debug >= 3)
                printf(" M1 - T1 - P_Clk");
	    }
        else { // (_clk == 0)
            _mreq = 0;
            _rd = 0;
            Step++;
            Cycles++;
            if (Debug >= 3)
                printf(" - N_Clk, ");
        }
        return;
	}

    //Machine Cycle M1 - Step T2 - Opcode Fetch
    if (Step == 1) {
    	if (_clk == 1) {
            if (Debug >= 3)
                printf("M1 - T2 - P_Clk ");
        }
        else if (_clk == 0 && _wait == 1){
            if (Debug >= 3)
                printf("- TW ");
        }
        else if (_clk == 0 && _wait == 0) {
            ZOpcode = data;
            Step++;
            Cycles++;
            if (Debug >= 3)
                printf("- N_Clk, ");
        }
        return;
	}

    //Machine Cycle M1 - Step T3 - Opcode Fetch - Refresh
    if (Step == 2) {
    	if (_clk == 1) {
    		address = (PC | R);
            _mreq = 1;
            _rd = 1;
            _m1 = 1;
            _rfsh = 0;
            if (Debug >= 3)
                printf("M1 - T3 - P_Clk ");
        }
        else { // (_clk == 0)
            _mreq = 0;
            Step++;
            Cycles++;
            if (Debug >= 3)
                printf("- N_Clk, ");
        }
        return;
    }

    //Machine Cycle M1 - Step T4 - Opcode Fetch - Refresh
    if (Step == 3) {
    	if (_clk == 1) {
            PC++;
            if (Debug >= 3)
                printf(" M1 - T4 - P_Clk");
        }
        else { // (_clk == 0)

            // Decode instruction T5
            switch(ZOpcode) {
                case 0x00: // NOP - no operation
                    if (Debug >= 1)
                        printf("\nNOP ");
                    Step = 0;
                    if (Debug >= 3)
                        printf(" - N_Clk, ");
                    break;
                case 0x04: // INC B - adds one to B
                    add(B,1);
                    if (Debug >= 1)
                        printf("\nINC B ");
                    Step = 0;
                    if (Debug >= 3)
                        printf(" - N_Clk ==> B = %d ",B);
                    break;
                default:
                    _mreq = 1;
                    Step++;
                    Cycles++;
                    if (Debug >= 3)
                        printf(" - N_Clk, ");
                    break;
                }
        }
        return;
    }

    //Machine Cycle M2 - Step T1 - Memory Read/Write Cycle
    if (Step == 4) {

        // Decode instruction T5
        switch(ZOpcode) {
            case 0x01: // LD BC, nn - loads nn into BC
                if (_clk == 1) {
                    address = PC;
                    if (Debug >= 1)
                        printf("\nLD BC, nn ");
                    if (Debug >= 3)
                        printf(" - M2 - T1 - P_Clk");
                }
                else { // (_clk == 0)
                    _mreq = 0;
                    _rd = 0;
                    Step++;
                    Cycles++;
                    if (Debug >= 3)
                        printf(" - N_Clk, ");
                }
                break;
            case 0x02: // LD (BC), A - Stores A in memory location pointed by BC
                if (_clk == 1) {
                    address = ((B << 8) | C);
                    if (Debug >= 1)
                        printf("\nLD (BC), A ");
                    if (Debug >= 3)
                        printf(" - M2 - T1 - P_Clk");
                }
                else { // (_clk == 0)
                    data = A;
                    _mreq = 0;
                    Step++;
                    Cycles++;
                    if (Debug >= 3)
                        printf(" - N_Clk, ");
                }
                break;
            case 0x03: // INC BC - adds one to BC
                if (_clk == 1) {
                    add(C,1);
                    if (Debug >= 1)
                        printf("\nINC BC ");
                    if (Debug >= 3)
                        printf(" - M2 - T1 - P_Clk");
                }
                else { // (_clk == 0)
                    Step++;
                    Cycles++;
                    if (Debug >= 3)
                        printf(" - N_Clk, ");
                }
                break;
            default:
                break;
            }
        return;
        }


    //Machine Cycle M2 - Step T2 - Memory Read Cycle
    if (Step == 5) {
        // Decode instruction T5
        switch(ZOpcode) {
            case 0x01: // LD BC, nn - loads nn into BC
                if (_clk == 1) {
                    if (Debug >= 3)
                        printf(" M2 - T2 - P_Clk");
                }
                else if (_clk == 0 && _wait == 1){
                    if (Debug >= 3)
                        printf("- TW ");
                }
                else if (_clk == 0 && _wait == 0) {
                    Step++;
                    Cycles++;
                    if (Debug >= 3)
                        printf("- N_Clk, ");
                }
                break;
            case 0x02: // LD (BC), A - Stores A in memory location pointed by BC
                if (_clk == 1) {
                    if (Debug >= 3)
                        printf(" M2 - T2 - P_Clk");
                }
                else if (_clk == 0 && _wait == 1){
                    if (Debug >= 3)
                        printf("- TW ");
                }
                else if (_clk == 0 && _wait == 0) {
                    _wr = 0;
                    Step++;
                    Cycles++;
                    if (Debug >= 3)
                        printf(" - N_Clk, ");
                }
                break;
            case 0x03: // INC BC - adds one to BC
                if (_clk == 1) {
                    addc(B,0);
                    if (Debug >= 3)
                        printf(" M2 - T1 - P_Clk");
                }
                else { // (_clk == 0)
                    Step = 0;
                    Cycles++;
                    if (Debug >= 3)
                        printf(" - N_Clk ==> BC = %d ",BC);
                }
                break;
            default:
                break;
        }
        return;
	}

    //Machine Cycle M2 - Step T3 - Memory Read Cycle
    if (Step == 6) {

        // Decode instruction T7
        switch(ZOpcode) {
            case 0x01: // LD BC, nn - loads nn into BC
                if (_clk == 1) {
                    if (Debug >= 3)
                        printf(" M2 - T3 - P_Clk");
                }
                else { // (_clk == 0)
                    C = data;
                    _mreq = 1;
                    _rd = 1;
                    PC++;
                    Step++;
                    if (Debug >= 3)
                        printf(" - N_Clk ==> C = %d, ", C);
                }
                break;
                case 0x02: // LD (BC), A - Stores A in memory location pointed by BC
                    if (_clk == 1) {
                    if (Debug >= 3)
                        printf(" - M2 - T3 - P_Clk");
                    }
                    else { // (_clk == 0)
                        PC++;
                        _mreq = 1;
                        _wr = 1;
                        Step = 0;
                        Cycles++;
                        if (Debug >= 3)
                            printf(" ==> (%X) = %d - N_Clk, ",address,A);
                    }
                    break;
            default:
                break;
        }
        return;
	}

	    //Machine Cycle M3 - Step T1 - Memory Read/Write Cycle
    if (Step == 7) {

        // Decode instruction T8
        switch(ZOpcode) {
            case 0x01: // LD BC, nn - loads nn into BC
                if (_clk == 1) {
                    address = PC;
                    if (Debug >= 3)
                        printf(" M3 - T1 - P_Clk");
                }
                else { // (_clk == 0)
                    _mreq = 0;
                    _rd = 0;
                    Step++;
                    Cycles++;
                    if (Debug >= 3)
                        printf(" - N_Clk, ");
                }
                break;
            default:
                break;
            }
        return;
        }

    //Machine Cycle M3 - Step T2 - Memory Read Cycle
    if (Step == 8) {
    	if (_clk == 1) {
            if (Debug >= 3)
                printf(" M3 - T2 - P_Clk ");
        }
        else if (_clk == 0 && _wait == 1){
            if (Debug >= 3)
                printf("- TW ");
        }
        else if (_clk == 0 && _wait == 0) {
            Step++;
            Cycles++;
            if (Debug >= 3)
                printf("- N_Clk, ");
        }
        return;
	}

    //Machine Cycle M3 - Step T3 - Memory Read Cycle
    if (Step == 9) {

        // Decode instruction T10
        switch(ZOpcode) {
            case 0x01: // LD BC, nn - loads nn into BC
                if (_clk == 1) {
                    if (Debug >= 3)
                        printf(" M3 - T3 - P_Clk");
                }
                else { // (_clk == 0)
                    B = data;
                    _mreq = 1;
                    _rd = 1;
                    PC++;
                    Step++;
                    if (Debug >= 3)
                        printf(" - N_Clk ==> B = %d ==> BC = %d",B,BC);
                }
                break;
            default:
                break;
        }
        return;
	}


	    //Machine Cycle M4 - Step T1 - Memory Read/Write Cycle
    if (Step == 10) {

        // Decode instruction T11
        switch(ZOpcode) {
            case 0x01: // LD BC, nn - loads nn into BC
                if (_clk == 1) {
                    if (Debug >= 3)
                        printf(" M4 - T1 - P_Clk");
                }
                else { // (_clk == 0)
                    Step = 0;
                    Cycles++;
                    if (Debug >= 3)
                        printf(" - N_Clk, ");
                }
                break;
            default:
                break;
            }
        return;
        }


}





// main console program
int main(int argc, char *argv[]) {
    printf("Z80 Emulator\n");

    char *codeFile = "ROM.bin";
    FILE *fd;
    long filelen;
    long counter = 0;

    uint8_t ram[32768];
    uint8_t rom[32768];

    // Open the ROM file
    fd = fopen(codeFile,"rb");

    // Print some text if the file does not exist and exit
    if(fd == NULL) {
        printf("File ROM.bin not found! \n");
        printf("Press Any Key to Exit\n");
        fclose(fd);
        return 1;
    }

    // Move the position indicator to the end of the file
    fseek(fd, 0, SEEK_END);

    // Read the position
    filelen = ftell(fd);

    // Print some text if the file is too large and exist
    if(filelen > 32768) {
        printf("File ROM.bin is larger then 32768 Bytes! \n");
        printf("Press Any Key to Exit\n");
        fclose(fd);
        return 1;
    }

    // Move the position indicator to the beginning of the file
    rewind(fd);

    // Read the ROM file
    fread(rom, 1, filelen, fd);
    fclose(fd);

    resetZ80();

    // set control signals
	_wait = 0;	// in
	_int = 1;	// in
	_nmi = 1;	// in
 	_reset = 1;	// in
 	_busrq = 1;	// in
 	_clk = 0;	// in

    while(counter < Limit){
        //if (Debug >= 3)
            //printf("\n counter = %d", counter);


        // read from ROM Memory
        if (_mreq == 0 && _rd == 0 && address <= 32768)
            data = rom[address];

        // read/write to RAM Memory
        if (_mreq == 0 && _wr == 0 && address > 32768)
            ram[address] = data;
        if (_mreq == 0 && _rd == 0 && address > 32768)
            data = ram[address];

        _clk = 1;
        emuZ80();

        _clk = 0;
        emuZ80();

        counter++;
    }

    //TODO: ability to set the clock speed

    return 0;
}



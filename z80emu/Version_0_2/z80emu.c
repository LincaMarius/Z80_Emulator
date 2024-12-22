#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/*
    This is a Hardware Level Emulator for the Zilog Z80 Processor.

    This emulator is created with the purpose of testing the functioning of the code
    to create a Z80 emulator that will run on a Raspberry Pi PICO.

    Copyright (c) 2024 Linca Marius Gheorghe

    Version 0.2
*/

int Debug = 10;
int Wait = 1;
long Limit = 70;  // nr of tick to run

typedef union
{
    uint8_t r1;
    uint8_t r2;
    uint16_t rr;
} z80reg;

typedef struct z80status
{
    z80reg z_regs[9];
    uint16_t z_pc;
    uint16_t z_sp;
    uint8_t z_index[2];
    int8_t z_cycles;    // M cycles
    int8_t z_step;      // T steps
	int8_t z_opcode;
	int8_t z_operand;

    struct {
        int c:1;
        int n:1;
        int pv:1;
        int h:1;
        int z:1;
        int s:1;
        int f3:1;
        int f5:1;
    } flags;

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
#define ZOperand z80.z_operand

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
    if (Debug >= 1)
        printf("\nDebug Level = %d\n\n", Debug);
    _m1 = 1;
    _mreq = 1;
    _ireq = 1;
    _rd = 1;
    _wr = 1;
    _rfsh = 1;
    _halt = 1;
    _busack = 1;
}

int8_t add_func1 (int8_t a, int8_t b){
    int16_t result = a + b;
    if (result >= 255)
        z80.flags.f5 = 1;
    else
        z80.flags.f5 = 0;
    return (result & 0xFF);
}

int8_t addc_func1 (int8_t a, int8_t b){
    int16_t result = a + b + z80.flags.f5;
    if (result >= 255)
        z80.flags.f3 = 1;
    else
        z80.flags.f3 = 0;
    return (result & 0xFF);
}

int8_t add_func2 (int8_t a, int8_t b){
    int16_t result = a + b;
    z80.flags.n = 0;
    if (result >= 255)
        z80.flags.pv = 1;
    else
        z80.flags.pv = 0;
    z80.flags.h = 0;
    if (result == 0)
        z80.flags.z = 1;
    if ((result & 0xFF) & 0x80)
        z80.flags.s = 1;
    return (result & 0xFF);
}

int8_t sub_func2 (int8_t a, int8_t b){
    int16_t result = a - b;
    z80.flags.n = 1;
    if (result >= 255)
        z80.flags.pv = 1;
    else
        z80.flags.pv = 0;
    z80.flags.h = 0;
    if (result == 0)
        z80.flags.z = 1;
    if ((result & 0xFF) & 0x80)
        z80.flags.s = 1;
    return (result & 0xFF);
}


void emuZ80(void) {
    if (Debug >= 4)
        printf("\n  Opcode = %d, ", ZOpcode);
    if (Debug >= 5)
        printf("Step = %d, ", Step);
    if (Debug >= 6)
        printf("Clk = %d, ", _clk);
    if (Debug >= 7)
        printf("PC = %d -- ", PC);

    uint8_t temp8;
    uint16_t temp16;

    //Machine Cycle M1 - Step T1 - Opcode Fetch
    if (Step == 0) {
    	if (_clk == 1) {
    		address = PC;
		    _m1 = 0;
            if (Debug >= 8)
                printf("M1 - T1 - P_Clk");
	    }
        else { // (_clk == 0)
            _mreq = 0;
            _rd = 0;
            Step++;
            if (Debug >= 8)
                printf(" - N_Clk, ");
        }
        return;
	}

    //Machine Cycle M1 - Step T2 - Opcode Fetch
    if (Step == 1) {
    	if (_clk == 1) {
            if (Debug >= 8)
                printf("M1 - T2 - P_Clk ");
        }
        else if (_clk == 0 && _wait == 0){
            if (Debug >= 8)
                printf("- TW ");
        }
        else if (_clk == 0 && _wait == 1) {
            ZOpcode = data;
            Step++;
            if (Debug >= 8)
                printf("- N_Clk, ");
        }
        return;
	}

    //Machine Cycle M1 - Step T3 - Opcode Fetch - Refresh + Instruction decoding
    if (Step == 2) {
    	if (_clk == 1) {
    		address = (PC | R);
            _mreq = 1;
            _rd = 1;
            _m1 = 1;
            _rfsh = 0;
            if (Debug >= 8)
                printf("M1 - T3 - P_Clk ");
        }
        else { // (_clk == 0)
            // Decode instruction T4
            switch(ZOpcode) {
                case 0x00: // NOP - no operation
                    if (Debug >= 1)
                        printf("\nNOP");
                    break;
                case 0x01: // LD BC, nn - loads nn into BC
                    if (Debug >= 1)
                        printf("\nLD BC, nn");
                    break;
                case 0x02: // LD (BC), A - Stores A in memory location pointed by BC
                    if (Debug >= 1)
                        printf("\nLD (BC), A");
                    break;
                case 0x03: // INC BC - Adds one to BC
                    if (Debug >= 1)
                        printf("\nINC BC");
                    break;
                case 0x04: // INC B - Adds one to B
                    if (Debug >= 1)
                        printf("\nINC B");
                    break;
                case 0x05: // DEC B - Subtract one from B
                    if (Debug >= 1)
                        printf("\nDEC B");
                    break;
                case 0x06: // LD B, n - Loads n into B
                    if (Debug >= 1)
                        printf("\nLD B, n");
                    break;
                case 0x07: // RLCA - The contents of A are rotated left one bit position. The contents of bit 7 are copied to the carry flag and bit 0
                    if (Debug >= 1)
                        printf("\nRLCA");
                    break;
                case 0x08: // EX AF, AF' - Exchanges the 16-bit contents of AF and AF'
                    if (Debug >= 1)
                        printf("\nEX AF, AF'");
                    break;
                case 0x09: // ADD HL, BC - The value of BC is added to HL
                    if (Debug >= 1)
                        printf("\nADD HL, BC");
                    break;
                default:
                    break;
            }
            _mreq = 0;
            Step++;
            if (Debug >= 8)
                printf(" - N_Clk, ");
        return;
        }
    }

    //Machine Cycle M1 - Step T4 - Opcode Fetch - Refresh + Instruction execution
    if (Step == 3) {
    	if (_clk == 1) {
            PC++;
            if (Debug >= 8)
                printf("M1 - T4 - P_Clk");
        }
        else { // (_clk == 0)
            _mreq = 1;
            Step++;
            Cycles++;
            // Decode instruction T4
            switch(ZOpcode) {
                case 0x00: // NOP - no operation
                    Step = 0;
                    break;
                case 0x01: // LD BC, nn - loads nn into BC
                case 0x02: // LD (BC), A - Stores A in memory location pointed by BC
                case 0x03: // INC BC - Adds one to BC
                case 0x06: // LD B, n - Loads n into B
                case 0x09: // ADD HL, BC - The value of BC is added to HL
                    break;
                case 0x04: // INC B - Adds one to B
                    B = add_func2(B,1);
                    Step = 0;
                    Cycles++;
                    if (Debug >= 8)
                        printf(" ==> B = 0x%X",B);
                    break;
                case 0x05: // DEC B - Subtract one from B
                    B = sub_func2(B,1);
                    Step = 0;
                    Cycles++;
                    if (Debug >= 8)
                        printf(" ==> B = 0x%X",B);
                    break;
                case 0x07: // RLCA - The contents of A are rotated left one bit position. The contents of bit 7 are copied to the carry flag and bit 0
                    if (A & 0x80)
                        z80.flags.c = 1;
                    else
                        z80.flags.c = 0;
                    A = A << 1;
                    if (z80.flags.c)
                        A = A & 1;
                    else
                        A = A & 0;
                    z80.flags.n = 0;
                    z80.flags.h = 0;
                    Step = 0;
                    Cycles++;
                    if (Debug >= 8)
                        printf(" ==> A = 0x%X",A);
                    break;
                case 0x08: // EX AF, AF' - Exchanges the 16-bit contents of AF and AF'
                    temp16 = AF1;
                    AF1 = AF;
                    AF = temp16;
                    if (Debug >= 8)
                        printf(" ==> A = 0x%X, F = 0x%X, AF = 0x%X, A' = 0x%X, F' = 0x%X, AF' = 0x%X",A,F,AF,A1,F1,AF1);
                    break;
                default:
                    break;
            }
            if (Debug >= 8)
                printf(" - N_Clk, ");
        }
        return;
    }

    //Machine Cycle M2 - Step T1 - Memory Read/Write Cycle
    if (Step == 4) {
        // Decode instruction T5
        switch(ZOpcode) {
            case 0x01: // LD BC, nn - loads nn into BC
            case 0x06: // LD B, n - Loads n into B
            case 0x09: // ADD HL, BC - The value of BC is added to HL
                if (_clk == 1) {
                    address = PC;
                    if (Debug >= 8)
                        printf("M2 - T1 - P_Clk");
                }
                else { // (_clk == 0)
                    _mreq = 0;
                    _rd = 0;
                    Step++;
                    if (Debug >= 8)
                        printf(" - N_Clk, ");
                }
                break;
            case 0x02: // LD (BC), A - Stores A in memory location pointed by BC
                if (_clk == 1) {
                    address = ((B << 8) | C);     //202h //514 dec
                    if (Debug >= 8)
                        printf(" M2 - T1 - P_Clk");
                }
                else { // (_clk == 0)
                    data = A;
                    _mreq = 0;
                    Step++;
                    if (Debug >= 8)
                        printf(" - N_Clk, ");
                }
                break;
            case 0x03: // INC BC - Adds one to BC
                if (_clk == 1) {
                    C = add_func1(C,1);
                    if (Debug >= 8)
                        printf(" M2 - T1 - P_Clk");
                }
                else { // (_clk == 0)
                    Step++;
                    if (Debug >= 8)
                        printf(" - N_Clk ==> C = 0x%X, ",C);
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
            case 0x06: // LD B, n - Loads n into B
            case 0x09: // ADD HL, BC - The value of BC is added to HL
                if (_clk == 1) {
                    if (Debug >= 8)
                        printf("M2 - T2 - P_Clk");
                }
                else if (_clk == 0 && _wait == 0){
                    if (Debug >= 8)
                        printf("- TW ");
                }
                else if (_clk == 0 && _wait == 1) {
                    Step++;
                    if (Debug >= 8)
                        printf("- N_Clk, ");
                }
                break;
            case 0x02: // LD (BC), A - Stores A in memory location pointed by BC
                if (_clk == 1) {
                    if (Debug >= 8)
                        printf(" M2 - T2 - P_Clk");
                }
                else if (_clk == 0 && _wait == 0){
                    if (Debug >= 8)
                        printf("- TW ");
                }
                else if (_clk == 0 && _wait == 1) {
                    _wr = 0;
                    Step++;
                    if (Debug >= 8)
                        printf(" - N_Clk, ");
                }
                break;
            case 0x03: // INC BC - Adds one to BC
                if (_clk == 1) {
                    B = addc_func1(B,0);
                    if (Debug >= 8)
                        printf(" M2 - T2 - P_Clk");
                }
                else { // (_clk == 0)
                    Step = 0;
                    Cycles++;
                    if (Debug >= 8)
                        printf(" - N_Clk ==> B = 0x%X, BC = 0x%X",B,BC);
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
                    if (Debug >= 8)
                        printf("M2 - T3 - P_Clk");
                }
                else { // (_clk == 0)
                    C = data;
                    _mreq = 1;
                    _rd = 1;
                    PC++;
                    Step++;
                    Cycles++;
                    if (Debug >= 8)
                        printf(" - N_Clk ==> C = 0x%X",C);
                }
                break;
            case 0x02: // LD (BC), A - Stores A in memory location pointed by BC
                if (_clk == 1) {
                    if (Debug >= 8)
                        printf("M2 - T3 - P_Clk");
                    }
                    else { // (_clk == 0)
                        PC++;
                        _mreq = 1;
                        _wr = 1;
                        Step = 0;
                        Cycles++;
                        if (Debug >= 8)
                            printf(" - N_Clk ==> (0x%X) <= 0x%X ",address,A);
                    }
                    break;
            case 0x06: // LD B, n - Loads n into B
                if (_clk == 1) {
                    if (Debug >= 8)
                        printf("M2 - T3 - P_Clk");
                }
                else { // (_clk == 0)
                    B = data;
                    _mreq = 1;
                    _rd = 1;
                    PC++;
                    Step = 0;
                    Cycles++;
                    if (Debug >= 8)
                        printf(" - N_Clk ==> B = 0x%X",B);
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
                    if (Debug >= 8)
                        printf("M3 - T1 - P_Clk");
                }
                else { // (_clk == 0)
                    _mreq = 0;
                    _rd = 0;
                    Step++;
                    if (Debug >= 8)
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
            if (Debug >= 8)
                printf("M3 - T2 - P_Clk ");
        }
        else if (_clk == 0 && _wait == 0){
            if (Debug >= 3)
                printf("- TW ");
        }
        else if (_clk == 0 && _wait == 1) {
            Step++;
            if (Debug >= 8)
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
                    if (Debug >= 8)
                        printf("M3 - T3 - P_Clk");
                }
                else { // (_clk == 0)
                    B = data;
                    _mreq = 1;
                    _rd = 1;
                    PC++;
                    Step = 0;
                    Cycles++;
                    if (Debug >= 8)
                        printf(" - N_Clk ==> B = 0x%X, C = 0x%X ===> BC = 0x%X",B,C,BC);
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

    printf("\nZ80 Emulator\n");

    char *codeFile = "ROM.bin";
    FILE *fd;
    long filelen;
    long counter = 0;

    uint8_t ram[32768];
    uint8_t vram[32768];
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

    // set input control signals
	_wait = 1;	// in
	_int = 1;	// in
	_nmi = 1;	// in
 	_reset = 1;	// in
 	_busrq = 1;	// in
 	_clk = 0;	// in

    resetZ80();

    while(counter < Limit){

        // read from ROM Memory
        if (_mreq == 0 && _rd == 0 && address <= 32768){
            data = rom[address];
            if (Debug >= 9)
                printf("\n\tRead data=%d from ROM address=%d\n",data,address);
        }

        // write to RAM Memory
        if (_mreq == 0 && _wr == 0 && address > 32768){
            ram[address] = data;
            if (Debug >= 9)
                printf("\n\tWrite data=%d to RAM address=%d\n",data,address);
        }

        // read from RAM Memory
        if (_mreq == 0 && _rd == 0 && address > 32768){
            data = ram[address];
            if (Debug >= 9)
                printf("\n\tRead data=%d from RAM address=%d\n",data,address);
        }

        // write to VRAM Memory
        if (_mreq == 0 && _wr == 0 && address <= 32768){
            vram[address] = data;
            if (Debug >= 9)
                printf("\n\tWrite data=%d from VRAM address=%d\n",data,address);
        }

        _clk = 1;
        emuZ80();

        _clk = 0;
        emuZ80();

        //printf("\tdata=%d, address=%d, WR=%d, RD=%d",data,address,_wr,_rd);
        counter++;
    }
    //TODO: ability to set the clock speed

    return 0;
}

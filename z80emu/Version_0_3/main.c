#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/*
    This is a Hardware Level Emulator for the Zilog Z80 Processor.

    This emulator is created with the purpose of testing the functioning of the code
    to create a Z80 emulator that will run on a Raspberry Pi PICO.

    Copyright (c) 2024 Linca Marius Gheorghe

    Version 0.3 - immediate value loading into register implemented
                - immediate value loading into register pair implemented
*/

int Debug = 15;
int Wait = 1;
long Limit = 130;  // nr of tick to run

typedef struct z80status
{
    int8_t z_a;         // Accumulator
    int8_t z_a1;         // Accumulator'

    // registers B and C
    union {
        struct {
            int8_t c;
            int8_t b;
        };
        int16_t bc;
    };

    // registers B' and C'
    union {
        struct {
            int8_t c1;
            int8_t b1;
        };
        int16_t bc1;
    };

    // registers D and E
    union {
        struct {
            int8_t e;
            int8_t d;
        };
        int16_t de;
    };

    // registers D' and E'
    union {
        struct {
            int8_t e1;
            int8_t d1;
        };
        int16_t de1;
    };

    //  registers H and L
    union {
        struct {
            int8_t l;
            int8_t h;
        };
        int16_t hl;
    };

    //  registers H' and L'
    union {
        struct {
            int8_t l1;
            int8_t h1;
        };
        int16_t hl1;
    };

    //  registers I and R
    union {
        struct {
            int8_t r;
            int8_t i;
        };
        int16_t ir;
    };

    //  stack pointer
    union {
        struct {
            int8_t z_spl;
            int8_t z_sph;
        };
        int16_t z_sp;
    };

    //  instruction opcode
    union {
        struct {
            uint8_t z_opcode_l;
            uint8_t z_opcode_h;
        };
        uint16_t z_opcode;
    };

    uint16_t z_pc;
    uint16_t z_ix;
    uint16_t z_iy;
    int8_t z_cycles;    // M cycles
    int8_t z_step;      // T steps
	int8_t z_operand;

    int8_t iff1;        // Interrupt flip flops
    int8_t iff2;
    int8_t im;          // Interrupt mode

    union {
        struct { // flags
            unsigned c:1;             // Carry
            unsigned n:1;             // Add/Subtract
            unsigned pv:1;            // Parity/Overflow
            unsigned f3:1;
            unsigned h:1;             // Half Carry
            unsigned f5:1;
            unsigned z:1;            // Zero
            unsigned s:1;            // Sign
        };
        unsigned char flags;
    } z_f;

    union {
        struct { // flags
            unsigned c:1;             // Carry
            unsigned n:1;             // Add/Subtract
            unsigned pv:1;            // Parity/Overflow
            unsigned f3:1;
            unsigned h:1;             // Half Carry
            unsigned f5:1;
            unsigned z:1;            // Zero
            unsigned s:1;            // Sign
        };
        unsigned char flags;
    } z_f1;

    int8_t z_temp8;

} z80status;

z80status z80;

#define A z80.z_a
#define A1 z80.z_a1
#define F z80.z_f.flags
#define F1 z80.z_f1.flags
#define B z80.b
#define B1 z80.b1
#define C z80.c
#define C1 z80.c1
#define BC z80.bc
#define BC1 z80.bc1
#define D z80.d
#define D1 z80.d1
#define E z80.e
#define E1 z80.e1
#define DE z80.de
#define DE1 z80.de1
#define H z80.h
#define H1 z80.h1
#define L z80.l
#define L1 z80.l1
#define HL z80.hl
#define HL1 z80.hl1
#define I z80.i
#define R z80.r
#define IR z80.ir

#define PC z80.z_pc
#define SPL z80.z_spl
#define SPH z80.z_sph
#define SP z80.z_sp
#define IX z80.z_ix
#define IY z80.z_iy
#define Cycles z80.z_cycles
#define Step z80.z_step
#define ZOpcode z80.z_opcode
#define ZOpcodeL z80.z_opcode_l
#define ZOpcodeH z80.z_opcode_h
#define ZOperand z80.z_operand
#define ZTemp8 z80.z_temp8

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
    ZOpcode = 0;
    _m1 = 1;
    _mreq = 1;
    _ireq = 1;
    _rd = 1;
    _wr = 1;
    _rfsh = 1;
    _halt = 1;
    _busack = 1;
}

void emuZ80(void) {
    if (Debug >= 4)
        printf("\n  Opcode = 0x%04X, ",ZOpcode);
    if (Debug >= 5)
        printf("Step = %d, ",Step);
    if (Debug >= 6)
        printf("Clk = %d, ", _clk);
    if (Debug >= 7)
        printf("PC = 0x%X -- ", PC);

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
            ZOpcodeL = data;
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
                        printf("\n\nNOP");
                    break;
                case 0x01: // LD BC, nn - loads nn into BC
                    if (Debug >= 1)
                        printf("\n\nLD BC, nn");
                    break;
                case 0x06: // LD B, n - Loads n into B
                    if (Debug >= 1)
                        printf("\n\nLD B, n");
                    break;
                case 0x0E: // LD C, n - Loads n into C
                    if (Debug >= 1)
                        printf("\n\nLD C, n");
                    break;
                case 0x11: // LD DE, nn - loads nn into DE
                    if (Debug >= 1)
                        printf("\n\nLD DE, nn");
                    break;
                case 0x16: // LD D, n - Loads n into D
                    if (Debug >= 1)
                        printf("\n\nLD D, n");
                    break;
                case 0x1E: // LD E, n - Loads n into E
                    if (Debug >= 1)
                        printf("\n\nLD E, n");
                    break;
                case 0x21: // LD HL, nn - loads nn into HL
                    if (Debug >= 1)
                        printf("\n\nLD HL, nn");
                    break;
                case 0x26: // LD H, n - Loads n into H
                    if (Debug >= 1)
                        printf("\n\nLD H, n");
                    break;
                case 0x2E: // LD L, n - Loads n into L
                    if (Debug >= 1)
                        printf("\n\nLD L, n");
                    break;
                case 0x31: // LD SP, nn - Loads nn into SP
                    if (Debug >= 1)
                        printf("\n\nLD SP, nn");
                    break;
                case 0x3E: // LD A, n - Loads n into A
                    if (Debug >= 1)
                        printf("\n\nLD A, n");
                    break;
                case 0xDD: // DD - set IX instructions prefix
                    if (Debug >= 1)
                        printf("\n\n(DD) ");
                    break;
                case 0xFD: // FD - set IY instructions prefix
                    if (Debug >= 1)
                        printf("\n\n(FD) ");
                    break;
                case 0xDD21: // LD IX, nn - loads nn into IX
                    if (Debug >= 1)
                        printf("\n\nLD IX, nn");
                    break;
                case 0xFD21: // LD IY, nn - loads nn into IY
                    if (Debug >= 1)
                        printf("\n\nLD IY, nn");
                    break;
                default:
                    break;
            }
            _mreq = 0;
            Step++;
            if (Debug >= 8)
                printf(" - N_Clk, ");
        }
        return;
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
                case 0x06: // LD B, n - Loads n into B
                case 0x0E: // LD C, n - Loads n into C
                case 0x11: // LD DE, nn - loads nn into DE
                case 0x16: // LD D, n - Loads n into D
                case 0x1E: // LD E, n - Loads n into E
                case 0x21: // LD HL, nn - loads nn into HL
                case 0x26: // LD H, n - Loads n into H
                case 0x2E: // LD L, n - Loads n into L
                case 0x31: // LD SP, nn - loads nn into SP
                case 0x3E: // LD A, n - Loads n into A
                case 0xDD21: // LD IX, nn - loads nn into IX
                case 0xFD21: // LD IY, nn - loads nn into IY
                    break;
                case 0xDD: // DD - set IX instructions prefix
                case 0xFD: // FD - set IY instructions prefix
                    ZOpcodeH = ZOpcodeL;
                    ZOpcodeL = 0;
                    Step = 0;
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
            case 0x0E: // LD C, n - Loads n into C
            case 0x11: // LD DE, nn - loads nn into DE
            case 0x16: // LD D, n - Loads n into D
            case 0x1E: // LD E, n - Loads n into E
            case 0x21: // LD HL, nn - loads nn into HL
            case 0x26: // LD H, n - Loads n into H
            case 0x2E: // LD L, n - Loads n into L
            case 0x31: // LD SP, nn - loads nn into SP
            case 0x3E: // LD A, n - Loads n into A
            case 0xDD21: // LD IX, nn - loads nn into IX
            case 0xFD21: // LD IY, nn - loads nn into IY
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
            case 0x0E: // LD C, n - Loads n into C
            case 0x11: // LD DE, nn - loads nn into DE
            case 0x16: // LD D, n - Loads n into D
            case 0x1E: // LD E, n - Loads n into E
            case 0x21: // LD HL, nn - loads nn into HL
            case 0x26: // LD H, n - Loads n into H
            case 0x2E: // LD L, n - Loads n into L
            case 0x31: // LD SP, nn - loads nn into SP
            case 0x3E: // LD A, n - Loads n into A
            case 0xDD21: // LD IX, nn - loads nn into IX
            case 0xFD21: // LD IY, nn - loads nn into IY
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
                        printf(" - N_Clk ==> C = 0x%02X",C);
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
                        printf(" - N_Clk ==> B = 0x%02X",B);
                }
                break;
            case 0x0E: // LD C, n - Loads n into C
                if (_clk == 1) {
                    if (Debug >= 8)
                        printf("M2 - T3 - P_Clk");
                }
                else { // (_clk == 0)
                    C = data;
                    _mreq = 1;
                    _rd = 1;
                    PC++;
                    Step = 0;
                    Cycles++;
                    if (Debug >= 8)
                        printf(" - N_Clk ==> C = 0x%02X",C);
                }
                break;
            case 0x11: // LD DE, nn - loads nn into DE
                if (_clk == 1) {
                    if (Debug >= 8)
                        printf("M2 - T3 - P_Clk");
                }
                else { // (_clk == 0)
                    E = data;
                    _mreq = 1;
                    _rd = 1;
                    PC++;
                    Step++;
                    Cycles++;
                    if (Debug >= 8)
                        printf(" - N_Clk ==> E = 0x%02X",E);
                }
                break;
            case 0x16: // LD D, n - Loads n into D
                if (_clk == 1) {
                    if (Debug >= 8)
                        printf("M2 - T3 - P_Clk");
                }
                else { // (_clk == 0)
                    D = data;
                    _mreq = 1;
                    _rd = 1;
                    PC++;
                    Step = 0;
                    Cycles++;
                    if (Debug >= 8)
                        printf(" - N_Clk ==> D = 0x%02X",D);
                }
                break;
            case 0x1E: // LD E, n - Loads n into E
                if (_clk == 1) {
                    if (Debug >= 8)
                        printf("M2 - T3 - P_Clk");
                }
                else { // (_clk == 0)
                    E = data;
                    _mreq = 1;
                    _rd = 1;
                    PC++;
                    Step = 0;
                    Cycles++;
                    if (Debug >= 8)
                        printf(" - N_Clk ==> E = 0x%02X",E);
                }
                break;
            case 0x21: // LD HL, nn - loads nn into HL
                if (_clk == 1) {
                    if (Debug >= 8)
                        printf("M2 - T3 - P_Clk");
                }
                else { // (_clk == 0)
                    L = data;
                    _mreq = 1;
                    _rd = 1;
                    PC++;
                    Step++;
                    Cycles++;
                    if (Debug >= 8)
                        printf(" - N_Clk ==> L = 0x%02X",L);
                }
                break;
            case 0x26: // LD H, n - Loads n into H
                if (_clk == 1) {
                    if (Debug >= 8)
                        printf("M2 - T3 - P_Clk");
                }
                else { // (_clk == 0)
                    H = data;
                    _mreq = 1;
                    _rd = 1;
                    PC++;
                    Step = 0;
                    Cycles++;
                    if (Debug >= 8)
                        printf(" - N_Clk ==> H = 0x%02X",H);
                }
                break;
            case 0x2E: // LD L, n - Loads n into L
                if (_clk == 1) {
                    if (Debug >= 8)
                        printf("M2 - T3 - P_Clk");
                }
                else { // (_clk == 0)
                    L = data;
                    _mreq = 1;
                    _rd = 1;
                    PC++;
                    Step = 0;
                    Cycles++;
                    if (Debug >= 8)
                        printf(" - N_Clk ==> L = 0x%02X",L);
                }
                break;
            case 0x31: // LD SP, nn - loads nn into SP
                if (_clk == 1) {
                    if (Debug >= 8)
                        printf("M2 - T3 - P_Clk");
                }
                else { // (_clk == 0)
                    SPL = data;
                    _mreq = 1;
                    _rd = 1;
                    PC++;
                    Step++;
                    Cycles++;
                    if (Debug >= 8)
                        printf(" - N_Clk ==> SPL = 0x%02X",SPL);
                }
                break;
            case 0x3E: // LD A, n - Loads n into A
                if (_clk == 1) {
                    if (Debug >= 8)
                        printf("M2 - T3 - P_Clk");
                }
                else { // (_clk == 0)
                    A = data;
                    _mreq = 1;
                    _rd = 1;
                    PC++;
                    Step = 0;
                    Cycles++;
                    if (Debug >= 8)
                        printf(" - N_Clk ==> A = 0x%02X",A);
                }
                break;
            case 0xDD21: // LD IX, nn - loads nn into IX
            case 0xFD21: // LD IY, nn - loads nn into IY
                if (_clk == 1) {
                    if (Debug >= 8)
                        printf("M2 - T3 - P_Clk");
                }
                else { // (_clk == 0)
                    ZTemp8 = data;
                    _mreq = 1;
                    _rd = 1;
                    PC++;
                    Step++;
                    Cycles++;
                    if (Debug >= 8)
                        printf(" - N_Clk ==> temp = 0x%02X",ZTemp8);
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
            case 0x11: // LD DE, nn - loads nn into DE
            case 0x21: // LD HL, nn - loads nn into HL
            case 0x31: // LD SP, nn - loads nn into SP
            case 0xDD21: // LD IX, nn - loads nn into IX
            case 0xFD21: // LD IY, nn - loads nn into IY
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
                        printf(" - N_Clk ==> B = 0x%02X, C = 0x%02X ===> BC = 0x%04X",B,C,BC);
                }
                break;
            case 0x11: // LD DE, nn - loads nn into DE
                if (_clk == 1) {
                    if (Debug >= 8)
                        printf("M3 - T3 - P_Clk");
                }
                else { // (_clk == 0)
                    D = data;
                    _mreq = 1;
                    _rd = 1;
                    PC++;
                    Step = 0;
                    Cycles++;
                    if (Debug >= 8)
                        printf(" - N_Clk ==> D = 0x%02X, E = 0x%02X ===> DE = 0x%04X",D,E,DE);
                }
                break;
            case 0x21: // LD HL, nn - loads nn into HL
                if (_clk == 1) {
                    if (Debug >= 8)
                        printf("M3 - T3 - P_Clk");
                }
                else { // (_clk == 0)
                    H = data;
                    _mreq = 1;
                    _rd = 1;
                    PC++;
                    Step = 0;
                    Cycles++;
                    if (Debug >= 8)
                        printf(" - N_Clk ==> H = 0x%02X, L = 0x%02X ===> HL = 0x%04X",H,L,HL);
                }
                break;
            case 0x31: // LD SP, nn - loads nn into SP
                if (_clk == 1) {
                    if (Debug >= 8)
                        printf("M3 - T3 - P_Clk");
                }
                else { // (_clk == 0)
                    SPH = data;
                    _mreq = 1;
                    _rd = 1;
                    PC++;
                    Step = 0;
                    Cycles++;
                    if (Debug >= 8)
                        printf(" - N_Clk ==> SPL = 0x%02X, SPH = 0x%02X ===> SP = 0x%04X",SPL,SPH,SP);
                }
                break;
            case 0xDD21: // LD IX, nn - loads nn into IX
                if (_clk == 1) {
                    if (Debug >= 8)
                        printf("M3 - T3 - P_Clk");
                }
                else { // (_clk == 0)
                    IX = (data << 8) + ZTemp8;
                    _mreq = 1;
                    _rd = 1;
                    PC++;
                    Step = 0;
                    Cycles++;
                    ZOpcode = 0;
                    if (Debug >= 8)
                        printf(" - N_Clk ===> IX = 0x%04X",IX);
                }
                break;
            case 0xFD21: // LD IY, nn - loads nn into IY
                if (_clk == 1) {
                    if (Debug >= 8)
                        printf("M3 - T3 - P_Clk");
                }
                else { // (_clk == 0)
                    IY = (data << 8) + ZTemp8;
                    _mreq = 1;
                    _rd = 1;
                    PC++;
                    Step = 0;
                    Cycles++;
                    ZOpcode = 0;
                    if (Debug >= 8)
                        printf(" - N_Clk ===> IY = 0x%04X",IY);
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
                printf("\n\tRead Data=0x%X from ROM Address=0x%X",data,address);
        }

        // write to RAM Memory
        if (_mreq == 0 && _wr == 0 && address > 32768){
            ram[address] = data;
            if (Debug >= 9)
                printf("\n\tWrite Data=0x%X to RAM Address=0x%X",data,address);
        }

        // read from RAM Memory
        if (_mreq == 0 && _rd == 0 && address > 32768){
            data = ram[address];
            if (Debug >= 9)
                printf("\n\tRead Data=0x%X from RAM Address=0x%X",data,address);
        }

        // write to VRAM Memory
        if (_mreq == 0 && _wr == 0 && address <= 32768){
            vram[address] = data;
            if (Debug >= 9)
                printf("\n\tWrite Data=0x%X from VRAM Address=0x%X",data,address);
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



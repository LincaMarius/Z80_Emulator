#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/*
    This is a Hardware Level Emulator for the Zilog Z80 Processor.

    This emulator is created with the purpose of testing the functioning of the code
    to create a Z80 emulator that will run on a Raspberry Pi PICO.

    Copyright (c) 2024 Linca Marius Gheorghe

    Version 0.1 - concept verification and implementation method verification
*/

int Debug = 10;
int WaitLevel = 0;
long InstrLimit = 2;  // nr of instructions to run
long ClkLimit = 50;

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
    }bc;

    // registers B' and C'
    union {
        struct {
            int8_t c1;
            int8_t b1;
        };
        int16_t bc1;
    }bc1;

// registers D and E
    union {
        struct {
            int8_t e;
            int8_t d;
        };
        int16_t de;
    }de;

    // registers D' and E'
    union {
        struct {
            int8_t e1;
            int8_t d1;
        };
        int16_t de1;
    }de1;

    //  registers H and L
    union {
        struct {
            int8_t l;
            int8_t h;
        };
        int16_t hl;
    }hl;

    //  registers H' and L'
    union {
        struct {
            int8_t l1;
            int8_t h1;
        };
        int16_t hl1;
    }hl1;

    //  registers I and R
    union {
        struct {
            uint8_t r;
            uint8_t i;
        };
        uint16_t ir;
    }ir;

    //  stack pointer
    union {
        struct {
            int8_t spl;
            int8_t sph;
        };
        int16_t sp;
    }sp;

    //  instruction opcode
    union {
        struct {
            uint8_t opcode_l;
            uint8_t opcode_h;
        };
        uint16_t opcode;
    }op;

    //  register IX
    union {
        struct {
            uint8_t ixl;
            uint8_t ixh;
        };
        uint16_t ix;
    }ix;

    //  register IY
    union {
        struct {
            uint8_t iyl;
            uint8_t iyh;
        };
        uint16_t iy;
    }iy;

    uint16_t z_pc;
    int8_t z_cycles;    // M cycles
    int8_t z_step;      // T steps
	int8_t z_operand;
	uint32_t max_cycles;
	uint32_t max_clocks;
	uint16_t max_instructions;

    int8_t iff1;        // Interrupt flip flops
    int8_t iff2;
    int8_t im;          // Interrupt mode
    int8_t halted;
    int8_t interruptActive;

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
    int16_t z_temp16;
    uint8_t clk11;          // 0 = low level; 1 = positive front; 2 = high level; 3 = negative front.

} z80status;

z80status z80;

#define A z80.z_a
#define A1 z80.z_a1
#define F z80.z_f.flags
#define F1 z80.z_f1.flags
#define B z80.bc.b
#define B1 z80.bc1.b1
#define C z80.bc.c
#define C1 z80.bc1.c1
#define BC z80.bc.bc
#define BC1 z80.bc1.bc1
#define D z80.de.d
#define D1 z80.de1.d1
#define E z80.de.e
#define E1 z80.de1.e1
#define DE z80.de.de
#define DE1 z80.de1.de1
#define H z80.hl.h
#define H1 z80.hl1.h1
#define L z80.hl.l
#define L1 z80.hl1.l1
#define HL z80.hl.hl
#define HL1 z80.hl1.hl1
#define I z80.ir.i
#define R z80.ir.r
#define IR z80.ir.ir
#define IFF1 z80.iff1
#define IFF2 z80.iff2
#define InterruptActive z80.interruptActive

#define PC z80.z_pc
#define SPH z80.sp.sph
#define SPL z80.sp.spl
#define SP z80.sp.sp
#define IX z80.ix.ix
#define IXH z80.ix.ixh
#define IXL z80.ix.ixl
#define IY z80.iy.iy
#define IYH z80.iy.iyh
#define IYL z80.iy.iyl
#define Cycles z80.z_cycles
#define Step z80.z_step
#define ZOpcode z80.op.opcode
#define ZOpcodeL z80.op.opcode_l
#define ZOpcodeH z80.op.opcode_h
#define ZOperand z80.z_operand
#define ZTemp8 z80.z_temp8
#define ZTemp16 z80.z_temp16
#define MaxCycles z80.max_cycles
#define MaxClocks z80.max_clocks
#define MaxInstrictions z80.max_instructions
#define Halted z80.halted

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
    Halted = 0;
    IFF1 = 0;
    IFF2 = 0;
    InterruptActive = 0;
    MaxCycles = 0;
    MaxClocks = 0;
    MaxInstrictions = 0;
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
            if (Halted == 1) {
                if (Debug >= 8)
                    printf("Halted -- M1 - T1 - P_Clk");
            }
            else {
                address = PC;
                _m1 = 0;
                _rfsh = 1;
                _rd = 1;
                if (Debug >= 8)
                    printf("M1 - T1 - P_Clk, address = 0x%04X",address);
            }
    	}
        else if (_clk == 0) {
            ZTemp8 = R;
            R++;
            if (R >= 0x80)
                R = 0;
            if (ZTemp8 >> 8)
                R = R | 0x80;
            else
                R = R & 0x7F;
            if (Debug >= 8)
                printf(" - N_Clk, R = 0x%02X",R);
            if (InterruptActive >= 1) {
                return;
            }
            else {
                _mreq = 0;
                _rd = 0;
            }
            Step++;
            MaxClocks++;
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
                printf("- TW, _wait = 0x%02X",_wait);
        }
        else if (_clk == 0 && _wait == 1) {
            ZOpcodeL = data;
            Step++;
            MaxClocks++;
            if (Debug >= 8)
                printf("- N_Clk, _wait = 0x%02X",_wait);
        }
        return;
	}

    //Machine Cycle M1 - Step T3 - Opcode Fetch - Refresh + Instruction decoding
    if (Step == 2) {
    	if (_clk == 1) {
    		address = IR;
            _mreq = 1;
            _rd = 1;
            _m1 = 1;
            _rfsh = 0;
            if (Debug >= 8)
                printf("M1 - T3 - P_Clk, address = 0x%04X",address);
        }
        else { // (_clk == 0)
            // Decode instruction T4
            switch(ZOpcode) {
                case 0x00: // NOP - no operation
                    if (Debug >= 1)
                        printf("\n\nNOP");
                    break;
                case 0x76: // HALT - Suspends CPU operation until an interrupt or reset occurs
                    if (Debug >= 1)
                        printf("\n\nHALT");
                    break;
                default:
                    break;
            }
            _mreq = 0;
            Step++;
            MaxClocks++;
            if (Debug >= 8)
                printf(" - N_Clk, ");
        }
        return;
    }

    //Machine Cycle M1 - Step T4 - Opcode Fetch - Refresh + Instruction execution
    if (Step == 3) {
    	if (_clk == 1) {
            if (Debug >= 8)
                printf("M1 - T4 - P_Clk");
        }
        else { // (_clk == 0)
            if (IFF1 == 1 && _busrq == 1){
                if (_int == 0) {
                    InterruptActive = 1;
                    Halted = 0;
                    _halt = 1;
                    printf("--> Interrupt");
                }
                else if (_nmi == 0){
                    InterruptActive = 2;
                    Halted = 0;
                    _halt = 1;
                    printf("--> NMI Interrupt");
                }
                else {
                    InterruptActive = 0;
                }
            }
            _mreq = 1;
            Step++;
            MaxClocks++;
            // Decode instruction T4
            switch(ZOpcode) {
                case 0x00: // NOP - no operation
                    Step = 0;
                    PC++;
                    MaxCycles++;
                    MaxInstrictions++;
                    break;
                case 0x76: // HALT - Suspends CPU operation until an interrupt or reset occurs
                    _halt = 0;
                    Halted = 1;
                    Step = 0;
                    MaxCycles++;
                    MaxInstrictions++;
                    if (Debug >= 8)
                        printf(" ==> HALT");
                    break;
                default:
                    break;
            }
            if (Debug >= 8)
                printf(" - N_Clk, ");
        }
        return;
    }
}






// main console program
int main(int argc, char *argv[]) {

    printf("\nZ80 Emulator\n");

    char *codeFile = "D:\\ROM.bin";
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

    uint8_t clk11;
    uint8_t clk22;

    while(1){

        // read from ROM Memory
        if ((_mreq == 0) && (_rd == 0) && (address <= 32768)){
            data = rom[address];
            if (Debug >= 9)
                printf("\n\tRead Data=0x%X from ROM Address=0x%X",data,address);
        }

        // write to RAM Memory
        else if (_mreq == 0 && _wr == 0 && address > 32768){
            ram[address] = data;
            if (Debug >= 9)
                printf("\n\tWrite Data=0x%X to RAM Address=0x%X",data,address);
        }

        // read from RAM Memory
        else if (_mreq == 0 && _rd == 0 && address > 32768){
            data = ram[address];
            if (Debug >= 9)
                printf("\n\tRead Data=0x%X from RAM Address=0x%X",data,address);
        }

        // write to VRAM Memory
        else if (_mreq == 0 && _wr == 0 && address <= 32768){
            vram[address] = data;
            if (Debug >= 9)
                printf("\n\tWrite Data=0x%X to VRAM Address=0x%X",data,address);
        }

        // refresh DRAM Memory
        else if (_mreq == 0 && _wr == 1 && _rd == 1 && _rfsh == 0){
            if (Debug >= 9)
                printf("\n\tRefresh DRAM Address=0x%X",address);
        }


        _clk = 1;
        emuZ80();

        _clk = 0;
        emuZ80();

        if (WaitLevel == 1) {
            int8_t FFD1 = 0;
            int8_t FFD2 = 0;
            if (_clk == 0) {
                FFD1 = _m1;
                }
            if (_clk == 0) {
                FFD2 = FFD1;
                if (FFD2 == 0)
                    FFD1 = 1;
            }
            _wait = FFD1;
        }

        if (WaitLevel == 2) {
            int8_t FFD3 = 0;
            int8_t FFD4 = 0;
            if (_clk == 0) {
                FFD3 = _mreq;
                if (FFD3 == 1)
                    FFD4 = 1;
                }
            if (_clk == 0) {
                FFD4 = FFD3;

            }
            _wait = FFD4 & ~FFD3;
        }

        counter++;
        if (MaxInstrictions >= InstrLimit || counter >= ClkLimit){
            if (Debug >= 1){
                printf("\n\nA=0x%02X, B=0x%02X, C=0x%02X, D=0x%02X, E=0x%02X, H=0x%02X, L=0x%02X, F=0x%02X",A,B,C,D,E,H,L,F);
                printf("\nA'=0x%02X, B'=0x%02X, C'=0x%02X, D'=0x%02X, E'=0x%02X, H'=0x%02X, L'=0x%02X, F'=0x%02X",A1,B1,C1,D1,E1,H1,L1,F1);
                printf("\nPC=0x%04X, SP=0x%04X, I=0x%02X, R=0x%02X, IX=0x%04X, IY=0x%04X",PC,SP,I,R,IX, IY);
                printf("\nMaxCycles M=0x%X(%d), MaxClocks T=0x%X(%d), MaxInstrictions=0x%X(%d)\n\n",MaxCycles,MaxCycles,MaxClocks,MaxClocks,MaxInstrictions,MaxInstrictions);
            }
        break;
        }
    }
    return 0;
    //TODO: ability to set the clock speed
}

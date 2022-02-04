#pragma once

#define DEBUG

#include <array>
#include <cstdint>
#include <vector>
#include <map>
#include <string>
#include <stdio.h>
#include <iostream>

class Bus; //Forward declaration of Bus

class SM83
{

public:
	//Constructor and deconstructor
	SM83();
	~SM83();

public:
	void connectBus(Bus* n) { bus = n; }; //link cpu to the bus

public:
	//Cpu core registers
	uint16_t pc = 0x0000; //program counter
	uint16_t sp = 0x0000; //Stack pointer

	uint8_t A = 0x00; //Accumulator
	uint8_t F = 0x00; //Flag register
	uint8_t B = 0x00; //High byte of BC
	uint8_t C = 0x00; //Low byte of BC
	uint8_t D = 0x00; //High byte of DE
	uint8_t E = 0x00; //Low byte of DE
	uint8_t H = 0x00; //High byte of HL
	uint8_t L = 0x00; //Low byte of HL

	uint16_t HL = 0x0000;
	uint16_t r16 = 0x0000;

	std::map<int, std::map < std::string, uint8_t* >> rp; //rp table
	std::map<int, std::map < std::string, uint8_t* >> rp2; //rp table
	std::map<int, uint8_t*> r; //r table

public:
	//Interrupt registers
	uint8_t IE = 0x00;
	uint8_t IF = 0x00;

public:
	//High RAM
	std::array<uint8_t, 127> HRAM = { 0x00 };

public:
	//External event functions
		void irqVBlank(); //Interupt request VBlank
		void irqLCDSTAT(); //Interupt request LCD STAT
		void irqTimer(); //Interupt request Timer
		void irqSerial(); //Interupt request Serial
		void irqJoypad(); //Interupt request JoyPad
		void irHandler();
		void clock(); //Do one clock tick
		bool complete(); //Helper function


public:
	//Flags
	enum FLAGS
	{
		z = (1 << 7), //Zero flag
		n = (1 << 6), //Substraction flag (BCD)
		h = (1 << 5), //Half Carry flag  (BCD)
		c = (1 << 4) //Carry flag
	};
	bool IME = false; // IME flag

private:
	//Get flag and set flag
	void setFlag(FLAGS f, bool v);
	uint8_t getFlag(FLAGS f);

public:
	//Linkage to communication bus
	Bus* bus = nullptr;
	uint8_t read(uint16_t address);
	void write(uint16_t address, uint8_t data);

public:
	//Variables and functions to facilitate emulation
	uint8_t opcode = 0x00;
	int cycles = 0;

private:
	//For opcode decoding
	uint8_t Y = 0x00;
	uint8_t X = 0x00;
	uint8_t Z = 0x00;
	uint8_t P = 0x00;
	uint8_t Q = 0x00;
	uint8_t n8 = 0x00;
	uint16_t n16 = 0x0000;
	int8_t e8 = 0x00;

	bool irqFlag = false; //Flag for interupt request. This is to know if interrupts have been requested

	void operate(uint8_t opcode);
	void operatePrefix(uint8_t opcode);

//cpu instructions. Returns the number of cycles to peform the instruction
private:
	//Load instructions
	int LDr8r8(uint8_t& r1, uint8_t& r2); //LD r8,r8
	int LDr8n8(uint8_t& r, uint8_t& n); //LD r8,n8
	int LDr16n16(uint8_t& lr, uint8_t& hr, uint16_t& n); //LD r16, n16
	int LDbHLbr8(uint8_t& r); //LD[HL], r8
	int LDrbHLbn8(uint8_t& n); //LD [HL],n8
	int LDr8bHLb(uint8_t& r); //LD r8,[HL]
	int LDbr16bA(uint16_t& r); //LD [r16],A
	int LDbn16bA(uint16_t& n); //LD [n16],A
	int LDHbn16nA(uint8_t& n); //LDH [n16],A
	int LDHbCbA(); //LDH [C],A
	int LDAbr16b(uint16_t& r); //LD A,[r16]
	int LDAbn16b(uint16_t& n); //LD A,[n16]
	int LDHAbn16b(uint8_t& n); //LDH A,[n16]
	int LDHAbCb(); //LDH A,[C]
	int LDbHLIbA(); //LD [HLI],A
	int LDbHLDbA(); //LD [HLD],A
	int LDAbHLDb(); //LD A,[HLD]
	int LDAabHLIb(); //LD A,[HLI]

	//8-bit Arithmetic and Logic Instructions
	int ADCAr8(uint8_t& r); //ADC A,r8
	int ADCAbHLb(); //ADC A,[HL]
	int ADCAn8(uint8_t& n8); //ADC A,n8
	int ADDAr8(uint8_t& r); //ADD A,r8
	int ADDAbHLb(); //ADD A,[HL]
	int ADDAn8(uint8_t& n8); //ADD A,n8
	int ANDAr8(uint8_t& r); //AND A,r8
	int ANDAbHLB(); //AND A,[HL]
	int ANDAn8(uint8_t& n8); //AND A,n8
	int CPAr8(uint8_t& r); //CP A,r8
	int CPAbHLb(); //CP A,[HL]
	int CPAn8(uint8_t& n8); //CP A,n8
	int DECr8(uint8_t& r); //DEC r8
	int DECbHLb(); //DEC [HL]
	int INCr8(uint8_t& r); //INC r8
	int INCbHLb(); //INC [HL]
	int ORAr8(uint8_t& r); //OR A,r8
	int ORAbHLb(); //OR A,[HL]
	int ORAn8(uint8_t n8); //OR A,n8
	int SBCAr8( uint8_t& r); //SBC A,r8
	int SBCAbHLb(); //SBC A,[HL]
	int SBCAn8(uint8_t n8); //SBC A,n8
	int SUBAr8(uint8_t& r); //SUB A,r8
	int SUBAbHLb(); //SUB A,[HL]
	int SUBAn8(uint8_t n8); //SUB A,n8
	int XORAr8(uint8_t& r); //XOR A,r8
	int XORAbHLb(); //XOR A,[HL]
	int XORAn8(uint8_t n8); //XOR A,n8

	//16-bit Arithmetic Instructions
	int ADDHLr16(uint16_t& r); //ADD HL,r16
	int DECr16(uint8_t& Hr, uint8_t& Lr); //DEC r16
	int INCr16(uint8_t& Hr, uint8_t& Lr); //INC r16

	//Bit Operations Instructions
	int BITu3r8(uint8_t& u3, uint8_t& r); //BIT u3,r8
	int BITu3bHLb(uint8_t& u3); //BIT u3,[HL]
	int RESu3r8(uint8_t& u3, uint8_t& r); //RES u3,r8
	int RESu3bHLb(uint8_t& u3); //RES u3,[HL]
	int SETu3r8(uint8_t& u3, uint8_t& r); //SET u3,r8
	int SETu3rbHLb(uint8_t& u3); //SET u3,[HL]
	int SWAPr8(uint8_t& r); //SWAP r8
	int SWAPbHLb(); //SWAP[HL]

	//Bit Shift Instructions
	int RLr8(uint8_t& r); //RL r8
	int RLbHLb(); //RL [HL]
	int RLA(); //RLA
	int RLCr8(uint8_t& r); //RLC r8
	int RLCbHLb(); //RLC [HL]
	int RLCA(); //RLCA
	int RRr8(uint8_t& r); //RR r8
	int RRbHLb(); //RR [HL]
	int RRA(); //RRA
	int RRCr8(uint8_t& r); //RRC r8
	int RRCbHLb(); //RRC [HL]
	int RRCA(); //RRCA
	int SLAr8(uint8_t& r); //SLA r8
	int SLAbHLb(); //SLA [HL]
	int SRAr8(uint8_t& r); // SRA r8
	int SRAbHLb(); //SRA [HL]
	int SRLr8(uint8_t& r); //SRL r8
	int SRLbHLb(); //SRL [HL]

	//Jumps and Subroutines
	int CALLn16(uint16_t& n16); //CALL n16
	int CALLccn16(uint16_t& n16, uint8_t& y) ; //CALL cc,n16
	int JPHL(); //JP HL
	int JRe8(int8_t e8); //JR e8
	int JRcce8(int8_t e8, uint8_t& y); //JR e8
	int JPn16(uint16_t& n16); //JP n16
	int JPccn16(uint16_t& n16, uint8_t& y); //JP cc,n16
	int RETcc(uint8_t& y); //RET cc
	int RET(); //RET
	int RETI(); //RET
	int RSTvec(uint8_t y); //RST vec

	//Stack Operations Instructions
	int ADDHLSP(); //ADD HL, SP
	int ADDSPe8(int8_t e8); //ADD SP,e8
	int DECSP(); //DEC SP
	int INCSP(); //INC SP
	int LDSPn16(uint16_t& n16); //LD SP,n16
	int LDbn16bSP(uint16_t& n16); //LD [n16],SP
	int LDHLspe8(int8_t e8); //LD HL,SP+e8
	int LDSPHL(); //LD SP,HL
	int POPr16(uint8_t& hByte, uint8_t& lByte); //POP r16
	int PUSHr16(uint8_t& hByte, uint8_t& lByte); //PUSH r16

	//Miscellaneous Instructions
	int CCF(); //CCF
	int CPL(); //CPL
	int DAA(); //DAA
	int DI(); //DI
	int EI(); //EI
	int NOP(); //NOP
	int STOP(); //STOP
	int SCF();
	void HALT(); //HALT
	//Flags to emulate Halt behaviour and bug
	bool HALTFlag = false;
	bool HALTBug = false; 
	uint16_t pcBug = 0x00;
	

#ifdef DEBUG
	public:
		//Disassembler
		std::map<uint8_t, std::string> instrMap;
		void initializeInstrMap();
#endif // DEBUG

};


	


#include "Disassembler.h"

Disassembler::Disassembler(int VectorSize)
{
	vInstrs.resize(VectorSize);
	vPC.resize(VectorSize);
	vData.resize(VectorSize);
}

Disassembler::~Disassembler()
{
}

void Disassembler::LoadCart(std::shared_ptr<Cartridge> cartridge)
{
	this->cart = cartridge;
}

void Disassembler::InitializeMaps()
{
	//Instrmap
	instrMap[0x00] = "NOP";
	instrMap[0x01] = "LD BC, d16";
	instrMap[0x02] = "LD (BC), A";
	instrMap[0x03] = "INC BC";
	instrMap[0x04] = "INC B";
	instrMap[0x05] = "DEC B";
	instrMap[0x06] = "LD B, d8";
	instrMap[0x07] = "RLCA";
	instrMap[0x08] = "LD (a16), SP";
	instrMap[0x09] = "ADD HL, BC";
	instrMap[0x0A] = "LD A, (BC)";
	instrMap[0x0B] = "DEC BC";
	instrMap[0x0C] = "INC C";
	instrMap[0x0D] = "DEC C";
	instrMap[0x0E] = "LD C, d8";
	instrMap[0x0F] = "RRCA";

	instrMap[0x10] = "STOP d8";
	instrMap[0x11] = "LD DE, d16";
	instrMap[0x12] = "LD (DE), A";
	instrMap[0x13] = "INC DE";
	instrMap[0x14] = "INC D";
	instrMap[0x15] = "DEC D";
	instrMap[0x16] = "LD D, d8";
	instrMap[0x17] = "RLA";
	instrMap[0x18] = "JR r8";
	instrMap[0x19] = "ADD HL, DE";
	instrMap[0x1A] = "LD A, (DE)";
	instrMap[0x1B] = "DEC DE";
	instrMap[0x1C] = "INC E";
	instrMap[0x1D] = "DEC E";
	instrMap[0x1E] = "LD E, d8";
	instrMap[0x1F] = "RRA";

	instrMap[0x20] = "JR NZ, r8";
	instrMap[0x21] = "LD HL, d16";
	instrMap[0x22] = "LD (HL+), A";
	instrMap[0x23] = "INC HL";
	instrMap[0x24] = "INC H";
	instrMap[0x25] = "DEC H";
	instrMap[0x26] = "LD H, d8";
	instrMap[0x27] = "DAA";
	instrMap[0x28] = "JR Z, r8";
	instrMap[0x29] = "ADD HL, HL";
	instrMap[0x2A] = "LD A, (HL+)";
	instrMap[0x2B] = "DEC HL";
	instrMap[0x2C] = "INC L";
	instrMap[0x2D] = "DEC L";
	instrMap[0x2E] = "LD L, d8";
	instrMap[0x2F] = "CPL";

	instrMap[0x30] = "JR NC, r8";
	instrMap[0x31] = "LD SP, d16";
	instrMap[0x32] = "LD (HL-), A";
	instrMap[0x33] = "INC SP";
	instrMap[0x34] = "INC (HL)";
	instrMap[0x35] = "DEC (HL)";
	instrMap[0x36] = "LD (HL), d8";
	instrMap[0x37] = "SCF";
	instrMap[0x38] = "JR C, r8";
	instrMap[0x39] = "ADD HL, SP";
	instrMap[0x3A] = "LD A, (HL-)";
	instrMap[0x3B] = "DEC SP";
	instrMap[0x3C] = "INC A";
	instrMap[0x3D] = "DEC A";
	instrMap[0x3E] = "LD A, d8";
	instrMap[0x3F] = "CCF";

	instrMap[0x40] = "LD B, B";
	instrMap[0x41] = "LD B, C";
	instrMap[0x42] = "LD B, D";
	instrMap[0x43] = "LD B, E";
	instrMap[0x44] = "LD B, H";
	instrMap[0x45] = "LD B, L";
	instrMap[0x46] = "LD B, (HL)";
	instrMap[0x47] = "LD B, A";
	instrMap[0x48] = "LD C, B";
	instrMap[0x49] = "LD C, C";
	instrMap[0x4A] = "LD C, D";
	instrMap[0x4B] = "LD C, E";
	instrMap[0x4C] = "LD C, H";
	instrMap[0x4D] = "LD C, L";
	instrMap[0x4E] = "LD C, (HL)";
	instrMap[0x4F] = "LD C, A";

	instrMap[0x50] = "LD D, B";
	instrMap[0x51] = "LD D, C";
	instrMap[0x52] = "LD D, D";
	instrMap[0x53] = "LD D, E";
	instrMap[0x54] = "LD D, H";
	instrMap[0x55] = "LD D, L";
	instrMap[0x56] = "LD D, (HL)";
	instrMap[0x57] = "LD D, A";
	instrMap[0x58] = "LD E, B";
	instrMap[0x59] = "LD E, C";
	instrMap[0x5A] = "LD E, D";
	instrMap[0x5B] = "LD E, E";
	instrMap[0x5C] = "LD E, H";
	instrMap[0x5D] = "LD E, L";
	instrMap[0x5E] = "LD E, (HL)";
	instrMap[0x5F] = "LD E, A";

	instrMap[0x60] = "LD H, B";
	instrMap[0x61] = "LD H, C";
	instrMap[0x62] = "LD H, D";
	instrMap[0x63] = "LD H, E";
	instrMap[0x64] = "LD H, H";
	instrMap[0x65] = "LD H, L";
	instrMap[0x66] = "LD H, (HL)";
	instrMap[0x67] = "LD H, A";
	instrMap[0x68] = "LD L, B";
	instrMap[0x69] = "LD L, C";
	instrMap[0x6A] = "LD L, D";
	instrMap[0x6B] = "LD L, E";
	instrMap[0x6C] = "LD L, H";
	instrMap[0x6D] = "LD L, L";
	instrMap[0x6E] = "LD L, (HL)";
	instrMap[0x6F] = "LD L, A";

	instrMap[0x70] = "LD (HL), B";
	instrMap[0x71] = "LD (HL), C";
	instrMap[0x72] = "LD (HL), D";
	instrMap[0x73] = "LD (HL), E";
	instrMap[0x74] = "LD (HL), H";
	instrMap[0x75] = "LD (HL), L";
	instrMap[0x76] = "HALT";
	instrMap[0x77] = "LD (HL), A";
	instrMap[0x78] = "LD A, B";
	instrMap[0x79] = "LD A, C";
	instrMap[0x7A] = "LD A, D";
	instrMap[0x7B] = "LD A, E";
	instrMap[0x7C] = "LD A, H";
	instrMap[0x7D] = "LD A, L";
	instrMap[0x7E] = "LD A, (HL)";
	instrMap[0x7F] = "LD A, A";

	instrMap[0x80] = "ADD A, B";
	instrMap[0x81] = "ADD A, C";
	instrMap[0x82] = "ADD A, D";
	instrMap[0x83] = "ADD A, E";
	instrMap[0x84] = "ADD A, H";
	instrMap[0x85] = "ADD A, L";
	instrMap[0x86] = "ADD A, (HL)";
	instrMap[0x87] = "ADD A, A";
	instrMap[0x88] = "ADC A, B";
	instrMap[0x89] = "ADC A, C";
	instrMap[0x8A] = "ADC A, D";
	instrMap[0x8B] = "ADC A, E";
	instrMap[0x8C] = "ADC A, H";
	instrMap[0x8D] = "ADC A, L";
	instrMap[0x8E] = "ADC A, (HL)";
	instrMap[0x8F] = "ADC A, A";

	instrMap[0x90] = "SUB B";
	instrMap[0x91] = "SUB C";
	instrMap[0x92] = "SUB D";
	instrMap[0x93] = "SUB E";
	instrMap[0x94] = "SUB H";
	instrMap[0x95] = "SUB L";
	instrMap[0x96] = "SUB (HL)";
	instrMap[0x97] = "SUB A";
	instrMap[0x98] = "SBC A, B";
	instrMap[0x99] = "SBC A, C";
	instrMap[0x9A] = "SBC A, D";
	instrMap[0x9B] = "SBC A, E";
	instrMap[0x9C] = "SBC A, H";
	instrMap[0x9D] = "SBC A, L";
	instrMap[0x9E] = "SBC A, (HL)";
	instrMap[0x9F] = "SBC A, A";

	instrMap[0xA0] = "AND B";
	instrMap[0xA1] = "AND C";
	instrMap[0xA2] = "AND D";
	instrMap[0xA3] = "AND E";
	instrMap[0xA4] = "AND H";
	instrMap[0xA5] = "AND L";
	instrMap[0xA6] = "AND (HL)";
	instrMap[0xA7] = "AND A";
	instrMap[0xA8] = "XOR B";
	instrMap[0xA9] = "XOR C";
	instrMap[0xAA] = "XOR D";
	instrMap[0xAB] = "XOR E";
	instrMap[0xAC] = "XOR H";
	instrMap[0xAD] = "XOR L";
	instrMap[0xAE] = "XOR (HL)";
	instrMap[0xAF] = "XOR A";

	instrMap[0xB0] = "OR B";
	instrMap[0xB1] = "OR C";
	instrMap[0xB2] = "OR D";
	instrMap[0xB3] = "OR E";
	instrMap[0xB4] = "OR H";
	instrMap[0xB5] = "OR L";
	instrMap[0xB6] = "OR (HL)";
	instrMap[0xB7] = "OR A";
	instrMap[0xB8] = "CP B";
	instrMap[0xB9] = "CP C";
	instrMap[0xBA] = "CP D";
	instrMap[0xBB] = "CP E";
	instrMap[0xBC] = "CP H";
	instrMap[0xBD] = "CP L";
	instrMap[0xBE] = "CP (HL)";
	instrMap[0xBF] = "CP A";

	instrMap[0xC0] = "RET NZ";
	instrMap[0xC1] = "POP BC";
	instrMap[0xC2] = "JP NZ, a16";
	instrMap[0xC3] = "JP a16";
	instrMap[0xC4] = "CALL NZ, a16";
	instrMap[0xC5] = "PUSH BC";
	instrMap[0xC6] = "ADD A, d8";
	instrMap[0xC7] = "RST 00H";
	instrMap[0xC8] = "RET Z";
	instrMap[0xC9] = "RET";
	instrMap[0xCA] = "JP Z, a16";
	instrMap[0xCB] = "PREFIX";
	instrMap[0xCC] = "CALL Z, a16";
	instrMap[0xCD] = "CALL a16";
	instrMap[0xCE] = "ADC A, d8";
	instrMap[0xCF] = "RST 08H";

	instrMap[0xD0] = "RET NC";
	instrMap[0xD1] = "POP DE";
	instrMap[0xD2] = "JP NC, a16";
	instrMap[0xD3] = "Illegal Opcode";
	instrMap[0xD4] = "CALL NC, a16";
	instrMap[0xD5] = "PUSH DE";
	instrMap[0xD6] = "SUB d8";
	instrMap[0xD7] = "RST 10H";
	instrMap[0xD8] = "RET C";
	instrMap[0xD9] = "RETI";
	instrMap[0xDA] = "JP C, a16";
	instrMap[0xDB] = "Illegal Opcode";
	instrMap[0xDC] = "CALL C, a16";
	instrMap[0xDD] = "Illegal Opcode";
	instrMap[0xDE] = "SBC A, d8";
	instrMap[0xDF] = "RST 18H";

	instrMap[0xE0] = "LDH (a8), A";
	instrMap[0xE1] = "POP HL";
	instrMap[0xE2] = "LD (C), A";
	instrMap[0xE3] = "Illegal Opcode";
	instrMap[0xE4] = "Illegal Opcode";
	instrMap[0xE5] = "PUSH HL";
	instrMap[0xE6] = "AND d8";
	instrMap[0xE7] = "RST 20H";
	instrMap[0xE8] = "ADD SP, r8";
	instrMap[0xE9] = "JP HL";
	instrMap[0xEA] = "LD (a16), A";
	instrMap[0xEB] = "Illegal Opcode";
	instrMap[0xEC] = "Illegal Opcode";
	instrMap[0xED] = "Illegal Opcode";
	instrMap[0xEE] = "XOR d8";
	instrMap[0xEF] = "RST 28H";

	instrMap[0xF0] = "LDH A, (a8)";
	instrMap[0xF1] = "POP AF";
	instrMap[0xF2] = "LD A, (C)";
	instrMap[0xF3] = "DI";
	instrMap[0xF4] = "Illegal Opcode";
	instrMap[0xF5] = "PUSH AF";
	instrMap[0xF6] = "OR d8";
	instrMap[0xF7] = "RST 30H";
	instrMap[0xF8] = "LD HL, SP + r8";
	instrMap[0xF9] = "LD SP, HL";
	instrMap[0xFA] = "LD A, (a16)";
	instrMap[0xFB] = "EI";
	instrMap[0xFC] = "Illegal Opcode";
	instrMap[0xFD] = "Illegal Opcode";
	instrMap[0xFE] = "CP d8";
	instrMap[0xFF] = "RST 38H";
}


//Read in pc and the subsequent 10 data from cart (ROM)
void Disassembler::Disassemble(uint16_t pc)
{
	for (int i = 0; i < vInstrs.size(); i++)
	{


		if (wait == 0)
		{
			//Find out how many bytes
			if (instrMap[Busptr->cpuRead(pc + i)].find("d8") != std::string::npos)
				bytes = 2;
			else if (instrMap[Busptr->cpuRead(pc + i)].find("d16") != std::string::npos)
				bytes = 3;
			else if (instrMap[Busptr->cpuRead(pc + i)].find("a8") != std::string::npos)
				bytes = 2;
			else if (instrMap[Busptr->cpuRead(pc + i)].find("a16") != std::string::npos)
				bytes = 3;
			else if (instrMap[Busptr->cpuRead(pc + i)].find("r8") != std::string::npos)
				bytes = 2;
			else
				bytes = 1;

			wait = bytes;
			vInstrs.at(i) = instrMap[Busptr->cpuRead(pc + i)];
			bytes--;
			for(int j = 0; j < bytes;j++)
			{
				if((i + j + 1) < vInstrs.size()) vInstrs.at(j + i + 1) = "Data";
			}
			
		}
		vData.at(i) = hex(Busptr->cpuRead(pc + i), 2);
		vPC.at(i) = hex(pc + i, 4);
		wait--;
		
	}

	wait = 0;
}

void Disassembler::ResizeVectors(int nSize)
{
	vInstrs.resize(nSize);
	vData.resize(nSize);
	vPC.resize(nSize);
}

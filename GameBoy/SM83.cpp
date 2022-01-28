#include "SM83.h"
#include "Bus.h"

//Constructor and deconstrctor
SM83::SM83()
{
	//Initialize rp table
	rp[0]["H"] = &B;
	rp[0]["L"] = &C;
	rp[1]["H"] = &D;
	rp[1]["L"] = &E;
	rp[2]["H"] = &H;
	rp[2]["L"] = &L;

	//Initialize rp2 table
	rp2[0]["H"] = &B;
	rp2[0]["L"] = &C;
	rp2[1]["H"] = &D;
	rp2[1]["L"] = &E;
	rp2[2]["H"] = &H;
	rp2[2]["L"] = &L;
	rp2[3]["H"] = &A;
	rp2[3]["L"] = &F;

	//Initialize r table
	r[0] = &B;
	r[1] = &C;
	r[2] = &D;
	r[3] = &E;
	r[4] = &H;
	r[5] = &L;
	r[7] = &A;

	//Reset vectors
	pc = 0x0100;
	sp = 0x0000;
	A = 0x00; //Accumulator
	F = 0x00; //Flag register
	B = 0x00; //High byte of BC
	C = 0x00; //Low byte of BC
	D = 0x00; //High byte of DE
	E = 0x00; //Low byte of DE
	H = 0x00; //High byte of HL
	L = 0x00; //Low byte of HL
}

SM83::~SM83()
{
}

//Interrupt requests
void SM83::irqVBlank()
{
	IF = IF | 0x01; 
}

void SM83::irqLCDSTAT()
{
	IF = IF | (0x01 << 1);
}

void SM83::irqTimer()
{
	IF = IF | (0x01 << 2);
}

void SM83::irqSerial()
{
	IF = IF | (0x01 << 3);
}

void SM83::irqJoypad()
{
	IF = IF | (0x01 << 4);
}

void SM83::irHandler()
{
	if (IME == true)
	{
		if (((IF & 0x01) > 1) && ((IE & 0x01) > 1)) //VBlank
		{
			IME = false;
			IF = IF & ~0x01;
			sp--;
			write(sp, (pc & 0xFF00) >> 8);
			sp--;
			write(sp, pc & 0x00FF);
			pc = 0x0040;
		}
		else if (((IF & (0x01 << 1)) > 1) && ((IE & (0x01 << 1)) > 1)) //LCD STAT
		{
			IME = false;
			IF = IF & ~(0x01 << 1);
			sp--;
			write(sp, (pc & 0xFF00) >> 8);
			sp--;
			write(sp, pc & 0x00FF);
			pc = 0x0048;
		}
		else if (((IF & (0x01 << 2)) > 1) && ((IE & (0x01 << 2)) > 1)) //Timer
		{
			IME = false;
			IF = IF & ~(0x01 << 2);
			sp--;
			write(sp, (pc & 0xFF00) >> 8);
			sp--;
			write(sp, pc & 0x00FF);
			pc = 0x0050;
		}
		else if (((IF & (0x01 << 3)) > 1) && ((IE & (0x01 << 3)) > 1)) //Serial
		{
			IME = false;
			IF = IF & ~(0x01 << 3);
			sp--;
			write(sp, (pc & 0xFF00) >> 8);
			sp--;
			write(sp, pc & 0x00FF);
			pc = 0x0058;
		}
		else if (((IF & (0x01 << 4)) > 1) && ((IE & (0x01 << 4)) > 1)) //Joypad
		{
			IME = false;
			IF = IF & ~(0x01 << 1);
			sp--;
			write(sp, (pc & 0xFF00) >> 8);
			sp--;
			write(sp, pc & 0x00FF);
			pc = 0x0060;
		}
		cycles += 5;
	}
}

//Do one cpu clock tick.
void SM83::clock()
{
	if (cycles == 0)
	{
		irHandler(); //Call irHandler first to check if we need to hand control to the interrupt handler

		opcode = read(pc);

		//If the opcode is prefixed by CB, the opcode is in a different bank of instructions
		if (opcode == 0xCB)
		{
			pc++;
			opcode = read(pc);
			operatePrefix(opcode);
		}
		else
			operate(opcode);

		cycles *= 4;

		cycles--; //Decrement one cycle
	}
	else 
		cycles--;
}

//Returns true when enough cycles have been ticked to do one instruction
bool SM83::complete()
{
	return cycles==0;
}

//set and get flag from F register
void SM83::setFlag(FLAGS f, bool v)
{
	if (v) F |= f;
	else F &= ~f;
}

uint8_t SM83::getFlag(FLAGS f)
{
	return ((F & f) > 1) ? 0x1 : 0x0;
	
}

//Read and write
uint8_t SM83::read(uint16_t address)
{
	return bus->cpuRead(address);
}

void SM83::write(uint16_t address, uint8_t data)
{
	bus->cpuWrite(address, data);
}

//Do one instruction
void SM83::operate(uint8_t opcode)
{
	pc++; //Increment program counter
	Z = opcode & 0x07;
	Y = (opcode & 0x38) >> 3;
	X = (opcode & 0xC0) >> 6;
	Q = (opcode & (0x01 << 3)) >> 3;
	P = (opcode & 0x30) >> 4;

	switch (X)
	{
	case 0: //x = 0
		switch (Z)
		{
		case 0: //x = 0, z = 0
			if (Y > 3)
			{
				e8 = read(pc);
				pc++;
				Y -= 4;
				cycles += JRcce8(e8, Y);
			}
			switch (Y)
			{
			case 0:
				cycles += NOP();
				break;
			case 1:
				n16 = (read(pc + 1) << 8) | read(pc);
				pc += 2;
				cycles += LDbn16bSP(n16);
				break;
			case 2:
				cycles += STOP();
				break;
			case 3:
				e8 = read(pc);
				pc++;
				cycles += JRe8(e8);
				break;
			}
			break;

		case 1: //x = 0, z = 1
			switch (Q)
			{
			case 0:
				if (P < 3) {
					n16 = (read(pc + 1) << 8) | read(pc);
					pc += 2;
					cycles += LDr16n16(*rp[P]["L"], *rp[P]["H"], n16);

				}
				else
				{
					n16 = (read(pc + 1) << 8) | read(pc);
					pc += 2;
					sp = n16;
					cycles += 3;
				}
				break;
			case 1:
				if (P < 3)
				{
					r16 = (*rp[P]["H"] << 8) | *rp[P]["L"];
					cycles += ADDHLr16(r16);
				}
				else
				{
					cycles += ADDHLSP();
				}
			}
			break;

		case 2: //x = 0, z = 2
			switch (Q)
			{
			case 0:
				switch (P)
				{
				case 0:
					r16 = (B << 8) | C;
					cycles += LDbr16bA(r16);
					break;
				case 1:
					r16 = (D << 8) | E;
					cycles += LDbr16bA(r16);
					break;
				case 2:
					cycles += LDbHLIbA();
					break;
				case 3:
					cycles += LDbHLDbA();
					break;
				}
				break;
			case 1:
				switch (P)
				{
				case 0:
					r16 = (B << 8) | C;
					cycles += LDAbr16b(r16);
					break;
				case 1:
					r16 = (D << 8) | E;
					cycles += LDAbr16b(r16);
					break;
				case 2:
					cycles += LDAabHLIb();
					break;
				case 3:
					cycles += LDAbHLDb();
					break;
				}
				break;
			}
			break;

		case 3: //x=0, z=3
			switch (Q)
			{
			case 0:
				cycles += INCr16(*rp[P]["H"], *rp[P]["L"]);
				break;
			case 1:
				cycles += DECr16(*rp[P]["H"], *rp[P]["L"]);
				break;
			}
			break;

		case 4: //x=0, z=4
			if (Y != 6)
				cycles += INCr8(*r[Y]);
			else
				cycles += INCbHLb();
			break;

		case 5: //x=0, z=5
			if (Y != 6)
				cycles += DECr8(*r[Y]);
			else
				cycles += DECbHLb();
			break;

		case 6: //x=0, z=6
			n8 = read(pc);
			pc++;
			if (Y != 6)
				cycles += LDr8n8(*r[Y], n8);
			else
				cycles += LDrbHLbn8(n8);
			break;

		case 7: //x=0, z=7
			switch (Y)
			{
			case 0:
				cycles += RLCA();
				break;
			case 1:
				cycles += RRCA();
				break;
			case 2:
				cycles += RLA();
				break;
			case 3:
				cycles += RRA();
				break;
			case 4:
				cycles += DAA();
				break;
			case 5:
				cycles += CPL();
				break;
			case 6:
				cycles += SCF();
				break;
			case 7:
				cycles += CCF();
				break;
			}
			break;
		}
		break;

	case 1: //x=1
		if ((Y != 6) && (Z != 6))
			cycles += LDr8r8(*r[Y], *r[Z]);
		else if ((Y == 6) && (Z != 6))
			cycles += LDbHLbr8(*r[Z]);
		else if ((Y != 6) && (Z == 6))
			cycles += LDr8bHLb(*r[Y]);
		else if ((Y == 6) && (Z == 6))
			HALT();
		break;

	case 2: //x=2
		switch (Y)
		{
		case 0:
			if (Z != 6)
				cycles += ADDAr8(*r[Z]);
			else
				cycles += ADDAbHLb();
			break;
		case 1:
			if (Z != 6)
				cycles += ADCAr8(*r[Z]);
			else
				cycles += ADCAbHLb();
			break;
		case 2:
			if (Z != 6)
				cycles += SUBAr8(*r[Z]);
			else
				cycles += SUBAbHLb();
			break;
		case 3:
			if (Z != 6)
				cycles += SBCAr8(*r[Z]);
			else
				cycles += SBCAbHLb();
			break;
		case 4:
			if (Z != 6)
				cycles += ANDAr8(*r[Z]);
			else
				cycles += ANDAbHLB();
			break;
		case 5:
			if (Z != 6)
				cycles += XORAr8(*r[Z]);
			else
				cycles += XORAbHLb();
			break;
		case 6:
			if (Z != 6)
				cycles += ORAr8(*r[Z]);
			else
				cycles += ORAbHLb();
			break;
		case 7:
			if (Z != 6)
				cycles += CPAr8(*r[Z]);
			else
				cycles += CPAbHLb();
			break;
		}
		break;

	case 3: //x=3
		switch (Z)
		{
		case 0: //x=3,z=0
			if (Y < 4)
				cycles += RETcc(Y);
			break;
			switch (Y)
			{
			case 4:
				n16 = (read(pc + 1) << 8) | read(pc);
				pc += 2;
				cycles += LDHbn16nA(n16);
				break;
			case 5:
				e8 = read(pc);
				pc++;
				cycles += ADDSPe8(e8);
				break;
			case 6:
				n16 = (read(pc + 1) << 8) | read(pc);
				pc += 2;
				cycles += LDHAbn16b(n16);
				break;
			case 7:
				e8 = read(pc);
				pc++;
				cycles += LDHLspe8(e8);
				break;
			}
			break;

		case 1: //x=3, z=1
			switch (Q)
			{
			case 0:
				cycles += POPr16(*rp2[P]["H"], *rp2[P]["L"]);
				break;
			case 1:
				switch (P)
				{
				case 0:
					cycles += RET();
					break;
				case 1:
					cycles += RETI();
					break;
				case 2:
					cycles += JPHL();
					break;
				case 3:
					cycles += LDSPHL();
					break;
				}
			}
			break;

		case 2: //x=3, z=2
			n16 = (read(pc + 1) << 8) | read(pc);
			pc += 2;
			if (Y < 4)
				cycles += JPccn16(n16, Y);
			switch (Y)
			{
			case 4:
				cycles += LDHbCbA();
				break;
			case 5:
				cycles += LDbn16bA(n16);
				break;
			case 6:
				cycles += LDHAbCb();
				break;
			case 7:
				cycles += LDAbn16b(n16);
				break;
			}
			break;

		case 3: //x=3, z=3
			n16 = (read(pc + 1) << 8) | read(pc);
			pc += 2;
			switch (Y)
			{
			case 0:
				cycles += JPn16(n16);
				break;
			case 6:
				cycles += DI();
				break;
			case 7:
				cycles += EI();
				break;
			}
			break;

		case 4: //x=3, z=4
			n16 = (read(pc + 1) << 8) | read(pc);
			pc += 2;
			if (Y < 4)
				cycles += CALLccn16(n16, Y);
			break;

		case 5: //x=3, z=5
			n16 = (read(pc + 1) << 8) | read(pc);
			pc += 2;
			switch (Q)
			{
			case 0:
				PUSHr16(*rp2[P]["H"], *rp2[P]["L"]);
				break;
			case 1:
				if (P == 0)
					cycles += CALLn16(n16);
				break;
			}
			break;

		case 6: //x=3, z=6
			n8 = read(pc);
			pc++;
			switch (Y)
			{
			case 0:
				cycles += ADDAn8(n8);
				break;
			case 1:
				cycles += ADCAn8(n8);
				break;
			case 2:
				cycles += SUBAn8(n8);
				break;
			case 3:
				cycles += SBCAn8(n8);
				break;
			case 4:
				cycles += ANDAn8(n8);
				break;
			case 5:
				cycles += XORAn8(n8);
				break;
			case 6:
				cycles += ORAn8(n8);
				break;
			case 7:
				cycles += CPAn8(n8);
				break;
			}
			break;

		case 7: //x=3, z=7
			cycles += RSTvec(Y);
			break;
		}

		default:
			std::cout << "Illegal opcode" << std::endl;
	}
}

//Operate instructions with CB prefix
void SM83::operatePrefix(uint8_t opcode)
{
	pc++; //Increment program counter
	Z = opcode & 0x07;
	Y = (opcode & 0x38) >> 3;
	X = (opcode & 0xC0) >> 6;
	Q = (opcode & (0x01 << 3)) >> 3;
	P = (opcode & 0x30) >> 4;

	switch (X)
	{
	case 0:
		switch (Y)
		{
		case 0:
			if (Z != 6)
				cycles += RLCr8(*r[Z]);
			else
				cycles += RLCbHLb();
			break;
		case 1:
			if (Z != 6)
				cycles += RRCr8(*r[Z]);
			else
				cycles += RRCbHLb();
			break;
		case 2:
			if (Z != 6)
				cycles += RLr8(*r[Z]);
			else
				cycles += RLbHLb();
			break;
		case 3:
			if (Z != 6)
				cycles += RRr8(*r[Z]);
			else
				cycles += RRbHLb();
			break;
		case 4:
			if (Z != 6)
				cycles += SLAr8(*r[Z]);
			else
				cycles += SLAbHLb();
			break;
		case 5:
			if (Z != 6)
				cycles += SRAr8(*r[Z]);
			else
				cycles += SRAbHLb();
			break;
		case 6:
			if (Z != 6)
				cycles += SWAPr8(*r[Z]);
			else
				cycles += SWAPbHLb();
			break;
		case 7:
			if (Z != 6)
				cycles += SRLr8(*r[Z]);
			else
				cycles += SRLbHLb();
			break;
		}
		break;

	case 1:
		if (Z != 6)
			cycles += BITu3r8(Y, *r[Z]);
		else
			cycles += BITu3bHLb(Y);
		break;

	case 2:
		if (Z != 6)
			cycles += RESu3r8(Y, *r[Z]);
		else
			cycles += RESu3bHLb(Y);
		break;

	case 3:
		if (Z != 6)
			cycles += SETu3r8(Y, *r[Z]);
		else
			cycles += SETu3rbHLb(Y);
		break;

	default:
		std::cout << "Illegal opcode" << std::endl;
	}

}

//--------------------------------------------------------------------------------------------------------------------------//
//-----------------------------------------cpu instruction function definitions here:---------------------------------------//
//--------------------------------------------------------------------------------------------------------------------------//

//Load instructions:

//Load (copy) value in register on the right into register on the left.
int SM83::LDr8r8(uint8_t& r1, uint8_t& r2)
{
	r1 = r2;
	return 1;
}

//Load value n8 into register r8.
int SM83::LDr8n8(uint8_t& r, uint8_t& n)
{
	r = n;
	return 2;
}

//Load value n16 into register r16.
int SM83::LDr16n16(uint8_t& lr,uint8_t& hr, uint16_t& n)
{
	lr = n & 0x00FF;
	hr = (n & 0xFF00) >> 8;
	return 3;
}

//Store value in register r8 into the byte pointed to by register HL.
int SM83::LDbHLbr8(uint8_t& r)
{
	write((H << 8) | L, r);
	return 2;
}

//Store value n8 into the byte pointed to by register HL.
int SM83::LDrbHLbn8(uint8_t& n)
{
	write((H << 8) | L, n);
	return 3;
}

//Load value into register r8 from the byte pointed to by register HL.
int SM83::LDr8bHLb(uint8_t& r)
{
	r = read((H << 8) | L);
	return 2;
}

//Store value in register A into the byte pointed to by register r16.
int SM83::LDbr16bA(uint16_t& r)
{
	A = read(r);
	return 2;
}

//Store value in register A into the byte at address n16.
int SM83::LDbn16bA(uint16_t& n)
{
	write(n, A);
	return 4;
}

//Store value in register A into the byte at address n16, provided the address is between $FF00 and $FFFF.
int SM83::LDHbn16nA(uint16_t& n)
{
	if (0xFF00 >= n && n <= 0xFFFF) write(n, A);
	return 3;
}

//Store value in register A into the byte at address $FF00+C.
int SM83::LDHbCbA()
{
	write(0xFF00 + C, A);
	return 2;
}

//Load value in register A from the byte pointed to by register r16.
int SM83::LDAbr16b(uint16_t& r)
{
	A = read(r);
	return 2;
}

//Load value in register A from the byte at address n16.
int SM83::LDAbn16b(uint16_t& n)
{
	A = read(n);
	return 4;
}

//Load value in register A from the byte at address n16, provided the address is between $FF00 and $FFFF.
int SM83::LDHAbn16b(uint16_t& n)
{
	if(0xFF00 >= n && n <=0xFFFF) A = read(n);
	return 3;
}

//Load value in register A from the byte at address $FF00+c.
int SM83::LDHAbCb()
{
	A = read(0xFF00 + C);
	return 2;
}

//Store value in register A into the byte pointed by HL and increment HL afterwards.
int SM83::LDbHLIbA()
{
	HL = (H << 8) | L;
	write(HL, A);
	L++;
	if(L == 0x00) H++;
	return 2;
}

//Store value in register A into the byte pointed by HL and decrement HL afterwards.
int SM83::LDbHLDbA()
{
	HL = (H << 8) | L;
	write(HL, A);
	L--;
	if (L == 0xFF) H--;
	return 2;
}

//Load value into register A from the byte pointed by HL and decrement HL afterwards.
int SM83::LDAbHLDb()
{
	HL = (H << 8) | L;
	A = read(HL);
	L--;
	if (L == 0xFF) H--;
	return 2;
}

//Load value into register A from the byte pointed by HL and increment HL afterwards.
int SM83::LDAabHLIb()
{
	HL = (H << 8) | L;
	A = read(HL);
	L++;
	if (L == 0x00) H++;
	return 2;
}

//8-bit Arithmetic and Logic Instructions:

//Add the value in r8 plus the carry flag to A.
int SM83::ADCAr8(uint8_t& r)
{
	if ((A & 0x0F) > (0xF - ((r + getFlag(c)) & 0x0F))) setFlag(h, 1);
	if(A > (0xFF - (r + getFlag(c)))) setFlag(c,1);
	setFlag(n, 0);
	A += (r + getFlag(c));
	if (A == 0x00) setFlag(z, 1);
	return 1;
}

//Add the byte pointed to by HL plus the carry flag to A.
int SM83::ADCAbHLb()
{
	HL = (H << 8) | L;
	if ((A & 0x0F) > (0xF - ((read(HL) + getFlag(c)) & 0x0F))) setFlag(h, 1);
	if (A > (0xFF - (read(HL) + getFlag(c)))) setFlag(c, 1);
	setFlag(n, 0);
	A += (read(HL) + getFlag(c));
	if (A == 0x00) setFlag(z, 1);
	return 2;
}

//Add the value n8 plus the carry flag to A.
int SM83::ADCAn8(uint8_t& n8)
{
	if ((A & 0x0F) > (0xF - ((n8 + getFlag(c)) & 0x0F))) setFlag(h, 1);
	if (A > (0xFF - (n8 + getFlag(c)))) setFlag(c, 1);
	setFlag(n, 0);
	A += (n + getFlag(c));
	if (A == 0x00) setFlag(z, 1);
	return 2;
}

//Add the value in r8 to A.
int SM83::ADDAr8(uint8_t& r)
{
	if ((A & 0x0F) > (0xF - (r & 0x0F))) setFlag(h, 1);
	if (A > (0xFF - r)) setFlag(c, 1);
	setFlag(n, 0);
	A += r;
	if (A == 0x00) setFlag(z, 1);
	return 1;
}

//Add the byte pointed to by HL to A.
int SM83::ADDAbHLb()
{
	HL = (H << 8) | L;
	if ((A & 0x0F) > (0xF - (read(HL) & 0x0F))) setFlag(h, 1);
	if (A > (0xFF - read(HL))) setFlag(c, 1);
	setFlag(n, 0);
	A += read(HL);
	if (A == 0x00) setFlag(z, 1);
 	return 2;
}

//Add the value n8 to A.
int SM83::ADDAn8(uint8_t& n8)
{
	if ((A & 0x0F) > (0xF - (n8 & 0x0F))) setFlag(h, 1);
	if (A > (0xFF - n8)) setFlag(c, 1);
	setFlag(n, 0);
	A += n8;
	if (A == 0x00) setFlag(z, 1);
	return 2;
}

//Bitwise AND between the value in r8 and A.
int SM83::ANDAr8(uint8_t& r)
{
	setFlag(h, 1);
	setFlag(n, 0);
	A = A & r;
	if (A == 0x00) setFlag(z, 1);
	return 1;
}

//Bitwise AND between the byte pointed to by HL and A.
int SM83::ANDAbHLB()
{
	HL = (H << 8) | L;
	setFlag(h, 1);
	setFlag(n, 0);
	A = A & read(HL);
	if (A == 0x00) setFlag(z, 1);
	return 2;
}

//Bitwise AND between the value in n8 and A.
int SM83::ANDAn8(uint8_t& n8)
{
	setFlag(h, 1);
	setFlag(n, 0);
	A = A & n8;
	if (A == 0x00) setFlag(z, 1);
	return 2;
}

//Subtract the value in r8 from A and set flags accordingly, but don't store the result. This is useful for Comparing values.
int SM83::CPAr8(uint8_t& r)
{
	if ((A - r) == 0x00) setFlag(z, 1);
	setFlag(n, 1);
	if((A & 0x0F) < (r & 0x0F)) setFlag(h,1);
	if (A < r) setFlag(c, 1);
	return 1;
}

//Subtract the byte pointed to by HL from A and set flags accordingly, but don't store the result.
int SM83::CPAbHLb()
{
	HL = (H << 8) | L;
	if ((A - read(HL)) == 0x00) setFlag(z, 1);
	setFlag(n, 1);
	if ((A & 0x0F) < (read(HL) & 0x0F)) setFlag(h, 1);
	if (A < read(HL)) setFlag(c, 1);
	return 2;
}

//Subtract the value n8 from A and set flags accordingly, but don't store the result.
int SM83::CPAn8(uint8_t& n8)
{
	if ((A - n8) == 0x00) setFlag(z, 1);
	setFlag(n, 1);
	if ((A & 0x0F) < (n8 & 0x0F)) setFlag(h, 1);
	if (A < n8) setFlag(c, 1);
	return 2;
}

//Decrement value in register r8 by 1.
int SM83::DECr8(uint8_t& r)
{
	if ((r & 0x0F) < 0x01) setFlag(h, 1);
	setFlag(n, 1);
	r--;
	if (r == 0x00) setFlag(z, 0);
	return 1;
}

//Decrement the byte pointed to by HL by 1.
int SM83::DECbHLb()
{
	HL = (H << 8) | L;
	if ((read(HL) & 0x0F) < 0x01) setFlag(h, 1);
	setFlag(n, 1);
	if (read(HL) - 1 == 0x00) setFlag(z, 0);
	write(HL, read(HL) - 1);
	return 3;
}

int SM83::INCr8(uint8_t& r)
{
	if ((r & 0x0F) > (0xF - 0x01)) setFlag(h, 1);
	setFlag(n, 0);
	r++;
	if (r == 0x00) setFlag(z, 1);
	return 1;
}

//Increment the byte pointed to by HL by 1.
int SM83::INCbHLb()
{
	HL = (H << 8) | L;
	if ((read(HL) & 0x0F) > (0xF - 0x01)) setFlag(h, 1);
	setFlag(n, 0);
	if((read(HL) + 1) == 0x00) setFlag(z, 1);
	write(HL, read(HL) + 1);
	return 3;
}

//Store into A the bitwise OR of the value in r8 and A.
int SM83::ORAr8(uint8_t& r)
{
	A = A | r;
	if (A == 0x00) setFlag(z, 1);
	setFlag(n, 0);
	setFlag(h, 0);
	setFlag(c, 0);
	return 1;
}

//Store into A the bitwise OR of the byte pointed to by HL and A.
int SM83::ORAbHLb()
{
	HL = (H << 8) | L;
	A = A | read(HL);
	if (A == 0x00) setFlag(z, 1);
	setFlag(n, 0);
	setFlag(h, 0);
	setFlag(c, 0);
	return 2;
}

//Store into A the bitwise OR of n8 and A.
int SM83::ORAn8(uint8_t n8)
{
	A = A | n8;
	if (A == 0x00) setFlag(z, 1);
	setFlag(n, 0);
	setFlag(h, 0);
	setFlag(c, 0);
	return 2;
}

//Subtract the value in r8 and the carry flag from A.
int SM83::SBCAr8(uint8_t& r)
{
	if(A < (r + getFlag(c))) setFlag(c,1);
	if((A & 0x0F) < ((r + getFlag(c)) & 0x0F)) setFlag(h, 1);
	A -= (r + getFlag(c));
	if (A == 0x00) setFlag(z, 1);
	setFlag(n, 1);
	return 1;
}

int SM83::SBCAbHLb()
{
	HL = (H << 8) | L;
	if (A < (read(HL) + getFlag(c))) setFlag(c, 1);
	if ((A & 0x0F) < ((read(HL) + getFlag(c)) & 0x0F)) setFlag(h, 1);
	A -= (read(HL) + getFlag(c));
	if (A == 0x00) setFlag(z, 1);
	setFlag(n, 1);
	return 2;
}

//Subtract the value n8 and the carry flag from A.
int SM83::SBCAn8(uint8_t n8)
{
	if (A < (n8 + getFlag(c))) setFlag(c, 1);
	if ((A & 0x0F) < ((n8 + getFlag(c)) & 0x0F)) setFlag(h, 1);
	A -= (n8 + getFlag(c));
	if (A == 0x00) setFlag(z, 1);
	setFlag(n, 1);
	return 2;
}

//Subtract the value in r8 from A.
int SM83::SUBAr8(uint8_t& r)
{
	if (A < r) setFlag(c, 1);
	if((A & 0x0F) < (r & 0x0F)) setFlag(h, 1);
	A -= r;
	if (A == 0x00) setFlag(z, 1);
	setFlag(n, 1);
	return 1;
}

//Subtract the byte pointed to by HL from A.
int SM83::SUBAbHLb()
{
	HL = (H << 8) | L;
	if (A < read(HL)) setFlag(c, 1);
	if ((A & 0x0F) < (read(HL) & 0x0F)) setFlag(h, 1);
	A -= read(HL);
	if (A == 0x00) setFlag(z, 1);
	setFlag(n, 1);
	return 2;
}

//Subtract the value n8 from A.
int SM83::SUBAn8(uint8_t n8)
{
	if (A < n8) setFlag(c, 1);
	if ((A & 0x0F) < (n8 & 0x0F)) setFlag(h, 1);
	A -= n8;
	if (A == 0x00) setFlag(z, 1);
	setFlag(n, 1);
	return 2;
}

//Bitwise XOR between the value in r8 and A.
int SM83::XORAr8(uint8_t& r)
{
	A = A ^ r;
	if (A == 0x00) setFlag(z, 1);
	setFlag(n, 0);
	setFlag(h, 0);
	setFlag(c, 0);
	return 1;
}

//Bitwise XOR between the byte pointed to by HL and A.
int SM83::XORAbHLb()
{
	HL = (H << 8) | L;
	A = A ^ read(HL);
	if (A == 0x00) setFlag(z, 1);
	setFlag(n, 0);
	setFlag(h, 0);
	setFlag(c, 0);
	return 2;
}

//Bitwise XOR between the value in n8 and A.
int SM83::XORAn8(uint8_t n8)
{
	A = A ^ n8;
	if (A == 0x00) setFlag(z, 1);
	setFlag(n, 0);
	setFlag(h, 0);
	setFlag(c, 0);
	return 2;
}


//16-bit Arithmetic Instructions

//Add the value in r16 to HL.
int SM83::ADDHLr16(uint16_t& r)
{
	HL = (H << 8) | L;
	if (HL > (0xFFFF - r)) setFlag(c, 1);
	if ((HL & 0x0FFF) > (0x0FFF - (r & 0x0FFF))) setFlag(h, 1);
	setFlag(n, 0);
	HL += r;
	H = (HL & 0xFF00) >> 8;
	L = HL & 0x00FF;
	
	return 2;
}

//Decrement value in register r16 by 1.
int SM83::DECr16(uint8_t& Hr, uint8_t& Lr)
{
	Lr--;
	if (Lr == 0xFF) Hr--;
	return 2;
}

//Increment value in register r16 by 1.
int SM83::INCr16(uint8_t& Hr, uint8_t& Lr)
{
	Lr++;
	if (Lr == 0x00) Hr++;
	return 2;
}

//Bit Operations Instructions

//Test bit u3 in register r8, set the zero flag if bit not set.
int SM83::BITu3r8(uint8_t& u3, uint8_t& r)
{
	if ((r & (0x1 << u3)) == 0) setFlag(z, 0);
	setFlag(h, 1);
	setFlag(n, 0);
	return 2;
}

int SM83::BITu3bHLb(uint8_t& u3)
{
	HL = (H << 8) | L;
	if ((read(HL) & (0x1 << u3)) == 0) setFlag(z, 0);
	setFlag(h, 1);
	setFlag(n, 0);
	return 3;
}

int SM83::RESu3r8(uint8_t& u3, uint8_t& r)
{
	r = r & ~(0x1 << u3);
	return 2;
}

int SM83::RESu3bHLb(uint8_t& u3)
{
	HL = (H << 8) | L;
	write(HL, read(HL) & ~(0x1 << u3));
	return 4;
}

//Set bit u3 in register r8 to 1. Bit 0 is the rightmost one, bit 7 the leftmost one.
int SM83::SETu3r8(uint8_t& u3, uint8_t& r)
{
	r = r | (0x1 << u3);
	return 2;
}

//Set bit u3 in the byte pointed by HL to 1. Bit 0 is the rightmost one, bit 7 the leftmost one.
int SM83::SETu3rbHLb(uint8_t& u3)
{
	HL = (H << 8) | L;
	write(HL, read(HL) | (0x1 << u3));
	return 4;
}

int SM83::SWAPr8(uint8_t& r)
{
	
	r = ((r & 0x0F) << 4) | ((r & 0xF0) >> 4);
	if (r == 0x00) setFlag(z, 1);
	setFlag(n, 0);
	setFlag(h, 0);
	setFlag(c, 0);
	return 2;
}

//Swap the upper 4 bits in the byte pointed by HL and the lower 4 ones.
int SM83::SWAPbHLb()
{
	HL = (H << 8) | L;
	write(HL, ((read(HL) & 0x0F) << 4) | ((read(HL) & 0xF0) >> 4));
	if (read(HL) == 0x00) setFlag(z, 1);
	setFlag(n, 0);
	setFlag(h, 0);
	setFlag(c, 0);
	return 4;
}

//Bit Shift Instructions

//Rotate bits in register r8 left through carry.
int SM83::RLr8(uint8_t& r)
{
	uint8_t lowestBit = getFlag(c);
	setFlag(c, (r & 0x80) >> 7);
	r = r << 1;
	r = r | lowestBit;
	if (r == 0x00) setFlag(z, 1);
	setFlag(n, 0);
	setFlag(h, 0);
	return 2;
}

//Rotate the byte pointed to by HL left through carry.
int SM83::RLbHLb()
{
	HL = (H << 8) | L;
	uint8_t lowestBit = getFlag(c);
	setFlag(c, (read(HL) & 0x80) >> 7);
	write(HL,(read(HL) << 1) | lowestBit);
	if (read(HL) == 0x00) setFlag(z, 1);
	setFlag(n, 0);
	setFlag(h, 0);
	return 4;
}

//Rotate register A left through carry.
int SM83::RLA()
{
	uint8_t lowestBit = getFlag(c);
	setFlag(c, (A & 0x80) >> 7);
	A = A << 1;
	A = A | lowestBit;
	setFlag(z, 0);
	setFlag(n, 0);
	setFlag(h, 0);
	return 1;
}

//Rotate register r8 left.
int SM83::RLCr8(uint8_t& r)
{
	setFlag(c, (r & 0x80) >> 7);
	r = (r << 1) | (r >> 7);
	if (r == 0x00) setFlag(z, 1);
	setFlag(n, 0);
	setFlag(h, 0);
	return 2;
}

//Rotate the byte pointed to by HL left.
int SM83::RLCbHLb()
{
	HL = (H << 8) | L;
	setFlag(c, (read(HL) & 0x80) >> 7);
	write(HL,(read(HL) << 1) | (read(HL) >> 7));
	if (read(HL) == 0x00) setFlag(z, 1);
	setFlag(n, 0);
	setFlag(h, 0);
	return 4;
}

//Rotate register A left.
int SM83::RLCA()
{
	setFlag(c, (A & 0x80) >> 7);
	A = (A << 1) | (A >> 7);
	if (A == 0x00) setFlag(z, 1);
	setFlag(n, 0);
	setFlag(h, 0);
	return 1;
}

//Rotate register r8 right through carry.
int SM83::RRr8(uint8_t& r)
{
	uint8_t highestBit = getFlag(c);
	setFlag(c, r & 0x01);
	r = r >> 1;
	r = r | (highestBit << 7);
	if (r == 0x00) setFlag(z, 1);
	setFlag(n, 0);
	setFlag(h, 0);
	return 2;
}

//Rotate the byte pointed to by HL right through carry.
int SM83::RRbHLb()
{
	HL = (H << 8) | L;
	uint8_t highestBit = getFlag(c);
	setFlag(c, read(HL) & 0x01);
	write(HL, (read(HL) >> 1) | (highestBit << 7));
	if (read(HL) == 0x00) setFlag(z, 1);
	setFlag(n, 0);
	setFlag(h, 0);
	return 4;
}

//Rotate register A right through carry.
int SM83::RRA()
{
	uint8_t highestBit = getFlag(c);
	setFlag(c, A & 0x01);
	A = A >> 1;
	A = A | (highestBit << 7);
	setFlag(z, 0);
	setFlag(n, 0);
	setFlag(h, 0);
	return 1;
}

//Rotate register r8 right.
int SM83::RRCr8(uint8_t& r)
{
	setFlag(c, r & 0x01);
	r = (r >> 1) | (r << 7);
	if (r == 0x00) setFlag(z, 1);
	setFlag(n, 0);
	setFlag(h, 0);
	return 2;
}

//Rotate the byte pointed to by HL right.
int SM83::RRCbHLb()
{
	HL = (H << 8) | L;
	setFlag(c, read(HL) & 0x01);
	write(HL, (read(HL) >> 1) | (read(HL) << 7));
	if (read(HL) == 0x00) setFlag(z, 1);
	setFlag(n, 0);
	setFlag(h, 0);
	return 4;
}

//Rotate register A right.
int SM83::RRCA()
{
	setFlag(c, A & 0x01);
	A = (A >> 1) | (A << 7);
	setFlag(z, 0);
	setFlag(n, 0);
	setFlag(h, 0);
	return 1;
}

//Shift Left Arithmetically register r8.
int SM83::SLAr8(uint8_t& r)
{
	setFlag(c, (r & 0x80) >> 7);
	r = (r << 1);
	if (r == 0x00) setFlag(z, 1);
	setFlag(n, 0);
	setFlag(h, 0);
	return 2;
}

//Shift Left Arithmetically the byte pointed to by HL.
int SM83::SLAbHLb()
{
	HL = (H << 8) | L;
	setFlag(c, (read(HL) & 0x80) >> 7);
	write(HL, (read(HL) << 1));
	if (read(HL) == 0x00) setFlag(z, 1);
	setFlag(n, 0);
	setFlag(h, 0);
	return 4;
}

//Shift Right Arithmetically register r8.
int SM83::SRAr8(uint8_t& r)
{
	setFlag(c, r & 0x01);
	r = (r >> 1) | (r & 0x80);
	if (r == 0x00) setFlag(z, 1);
	setFlag(n, 0);
	setFlag(h, 0);
	return 2;
}

//Shift Right Arithmetically the byte pointed to by HL.
int SM83::SRAbHLb()
{
	HL = (H << 8) | L;
	setFlag(c, read(HL) & 0x01);
	write(HL,(read(HL) >> 1) | (read(HL) & 0x80));
	if (read(HL) == 0x00) setFlag(z, 1);
	setFlag(n, 0);
	setFlag(h, 0);
	return 4;
}

//Shift Right Logically register r8.
int SM83::SRLr8(uint8_t& r)
{
	setFlag(c, r & 0x01);
	r = (r >> 1); 
	if (r == 0x00) setFlag(z, 1);
	setFlag(n, 0);
	setFlag(h, 0);
	return 2;
}

//Shift Right Logically the byte pointed to by HL.
int SM83::SRLbHLb()
{
	HL = (H << 8) | L;
	setFlag(c, read(HL) & 0x01);
	write(HL, (read(HL) >> 1));
	if (read(HL) == 0x00) setFlag(z, 1);
	setFlag(n, 0);
	setFlag(h, 0);
	return 4;
}

//Jumps and Subroutines

//Call address n16. This pushes the address of the instruction after the CALL on the stack,
//such that RET can pop it later; then, it executes an implicit JP n16.
int SM83::CALLn16(uint16_t& n16)
{
	sp--;
	write(sp, (pc & 0xFF00) >> 8);
	sp--;
	write(sp, pc & 0x00FF); 
	pc = n16;
	return 6;
}

//Call address n16 if condition cc is met.
int SM83::CALLccn16(uint16_t& n16, uint8_t& y)
{
	switch (y)
	{
	case 0:
		if (getFlag(z) == 0)
		{
			sp--;
			write(sp, (pc & 0xFF00) >> 8);
			sp--;
			write(sp, pc & 0x00FF);
			pc = n16;
			return 6;
		}
		else return 3;
	case 1:
		if (getFlag(z) == 1)
		{
			sp--;
			write(sp, (pc & 0xFF00) >> 8);
			sp--;
			write(sp, pc & 0x00FF);
			pc = n16;
			return 6;
		}
		else return 3;
	case 2:
		if (getFlag(c) == 0)
		{
			sp--;
			write(sp, (pc & 0xFF00) >> 8);
			sp--;
			write(sp, pc & 0x00FF);
			pc = n16;
			return 6;
		}
		else return 3;
	case 3:
		if (getFlag(c) == 1)
		{
			sp--;
			write(sp, (pc & 0xFF00) >> 8);
			sp--;
			write(sp, pc & 0x00FF);
			pc = n16;
			return 6;
		}
		else return 3;
	}
}

//Jump to address in HL; effectively, load PC with value in register HL.
int SM83::JPHL()
{
	HL = (H << 8) | L;
	pc = HL;
	return 1;
}

//Relative Jump by adding e8 to the address of the instruction following the JR.
//To clarify, an operand of 0 is equivalent to no jumping.
int SM83::JRe8(int8_t e8)
{
	pc += e8;
	return 3;
}

//Relative Jump by adding e8 to the current address if condition cc is met.
int SM83::JRcce8(int8_t e8, uint8_t &y)
{
	switch (y)
	{
	case 0:
		if (getFlag(z) == 0)
		{
			pc += e8;
			return 3;
		}
		else return 2;
	case 1:
		if (getFlag(z) == 1)
		{
			pc += e8;
			return 3;
		}
		else return 2;
	case 2:
		if (getFlag(c) == 0)
		{
			pc += e8;
			return 3;
		}
		else return 2;
	case 3:
		if (getFlag(c) == 1)
		{
			pc += e8;
			return 3;
		}
		else return 2;
	}
}

//Jump to address n16; effectively, store n16 into PC.
int SM83::JPn16(uint16_t& n16)
{
	pc = n16;
	return 4;
}

//Jump to address n16 if condition cc is met.
int SM83::JPccn16(uint16_t& n16, uint8_t& y)
{
	switch (y)
	{
	case 0:
		if (getFlag(z) == 0)
		{
			pc = n16;
			return 4;
		}
		else return 3;
	case 1:
		if (getFlag(z) == 1)
		{
			pc = n16;
			return 4;
		}
		else return 3;
	case 2:
		if (getFlag(c) == 0)
		{
			pc = n16;
			return 4;
		}
		else return 3;
	case 3:
		if (getFlag(c) == 1)
		{
			pc = n16;
			return 4;
		}
		else return 3;
	}
}

//Return from subroutine if condition cc is met.
int SM83::RETcc(uint8_t& y)
{
	switch (y)
	{
	case 0:
		if (getFlag(z) == 0)
		{
			pc = (read(sp + 1) << 8) | read(sp);
			sp += 2;
			return 5;
		}
		else return 2;
	case 1:
		if (getFlag(z) == 1)
		{
			pc = (read(sp + 1) << 8) | read(sp);
			sp += 2;
			return 5;
		}
		else return 2;
	case 2:
		if (getFlag(c) == 0)
		{
			pc = (read(sp + 1) << 8) | read(sp);
			sp += 2;
			return 5;
		}
		else return 2;
	case 3:
		if (getFlag(c) == 1)
		{
			pc = (read(sp + 1) << 8) | read(sp);
			sp += 2;
			return 5;
		}
		else return 2;
	}
}

//Return from subroutine.
int SM83::RET()
{
	pc = (read(sp + 1) << 8) | read(sp);
	sp += 2;
	return 4;
}

//Return from subroutine and enable interrupts.
int SM83::RETI()
{
	EI();
	RET();
	return 4;
}

//Call address vec.
int SM83::RSTvec(uint8_t y)
{
	sp--;
	write(sp, (pc & 0xFF00) >> 8);
	sp--;
	write(sp, pc & 0x00FF);
	pc = y*8;
	return 4;
}

//Stack Operations Instructions

//Add the value in SP to HL.
int SM83::ADDHLSP()
{
	HL = (H << 8) | L;
	if (HL > (0xFFFF - sp)) setFlag(c, 1);
	if ((HL & 0x0FFF) > (0x0FFF - (sp & 0x0FFF))) setFlag(h, 1);
	setFlag(n, 0);
	HL += sp;
	H = (HL & 0xFF00) >> 8;
	L = HL & 0x00FF;
	return 2;
}

//Add the signed value e8 to SP.
int SM83::ADDSPe8(int8_t e8)
{
	if(((sp & 0x000F) > (0x0F - (e8 & 0x0F)))) setFlag(h,1);
	if((sp & 0x00FF) > (0xFF - e8)) setFlag(c, 1);
	setFlag(n, 0);
	setFlag(z, 0);
	sp += e8;
	return 4;
}

//Decrement value in register SP by 1.
int SM83::DECSP()
{
	sp--;
	return 2;
}

//Increment value in register SP by 1.
int SM83::INCSP()
{
	sp++;
	return 2;
}

//Load value n16 into register SP.
int SM83::LDSPn16(uint16_t& n16)
{
	sp = n16;
	return 3;
}

//Store SP & $FF at address n16 and SP >> 8 at address n16 + 1.
int SM83::LDbn16bSP(uint16_t& n16)
{
	write(n16, sp & 0x00FF);
	write(n16 + 1, sp >> 8);
	return 5;
}

//Add the signed value e8 to SP and store the result in HL.
int SM83::LDHLspe8(int8_t e8)
{
	HL = (H << 8) | L;
	if (((sp & 0x000F) > (0x0F - (e8 & 0x0F)))) setFlag(h, 1);
	if ((sp & 0x00FF) > (0xFF - e8)) setFlag(c, 1);
	setFlag(z, 0);
	setFlag(n, 0);
	HL = sp + e8;
	return 3;
}

//Load register HL into register SP.
int SM83::LDSPHL()
{
	HL = (H << 8) | L;
	sp = HL;
	return 2;
}

//Pop register r16 from the stack.
int SM83::POPr16(uint8_t& hByte, uint8_t& lByte)
{
	lByte = read(sp);
	hByte = read(sp + 1);
	sp += 2;
	return 3;
}

//Push register r16 into the stack.
int SM83::PUSHr16(uint8_t& hByte, uint8_t& lByte)
{
	sp--;
	write(sp, hByte);
	sp--;
	write(sp, lByte);
	return 4;
}


//Miscellaneous Instructions

//Complement Carry Flag.
int SM83::CCF()
{
	F = F ^ (0x1 << 4); 
	return 1;
}

//CompLement accumulator (A = ~A).
int SM83::CPL()
{
	A = ~A;
	setFlag(n, 1);
	setFlag(h, 1);
	return 1;
}

//Decimal Adjust Accumulator to get a correct BCD representation after an arithmetic instruction.
int SM83::DAA()
{
	if (getFlag(n) == 0) //Addition
	{
		if (getFlag(h) == 1 || ((A & 0x0F) > 0x9)) 
		{
			A += 0x06;
		}
		if (getFlag(c) == 1 || ((A & 0xF0) > 0x90)) 
		{
			A += 0x60;
			setFlag(c, 1);
		}
		else setFlag(c, 0);

	}
	else //Subtraction
	{
		if (getFlag(h) == 1 || ((A & 0x0F) > 0x9))
		{
			A -= 0x06;
		}
		if (getFlag(c) == 1 || ((A & 0xF0) > 0x90))
		{
			A -= 0x60;
			setFlag(c, 1);
		}
		else setFlag(c, 0);
	}

	if(A == 0x00) setFlag(z, 1);
	setFlag(h, 0);


	return 1;
}

//Disable Interrupts by clearing the IME flag.
int SM83::DI()
{
	IME = 0;
	return 1;
}

//Enable Interrupts
int SM83::EI()
{
	IME = 1; 
	return 1;
}

//No operation
int SM83::NOP() {
	return 1;
}

//Unused for now
int SM83::STOP() {
	return 0;
}

//Set Carry Flag.
int SM83::SCF()
{
	setFlag(c, 1);
	return 1;
}

//Do nothing for now
void SM83::HALT()
{
}

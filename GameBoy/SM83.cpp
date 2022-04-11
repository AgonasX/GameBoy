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
		if (((IF & 0x01) > 0) && ((IE & 0x01) > 0)) //VBlank
		{
			IME = false;
			IF = IF & ~0x01;
			sp--;
			write(sp, (pc & 0xFF00) >> 8);
			sp--;
			write(sp, pc & 0x00FF);
			pc = 0x0040;
			cycles = 5;
		}
		else if (((IF & (0x01 << 1)) > 0) && ((IE & (0x01 << 1)) > 0)) //LCD STAT
		{
			IME = false;
			IF = IF & ~(0x01 << 1);
			sp--;
			write(sp, (pc & 0xFF00) >> 8);
			sp--;
			write(sp, pc & 0x00FF);
			pc = 0x0048;
			cycles = 5;
		}
		else if (((IF & (0x01 << 2)) > 0) && ((IE & (0x01 << 2)) > 0)) //Timer
		{
			IME = false;
			IF = IF & ~(0x01 << 2);
			sp--;
			write(sp, (pc & 0xFF00) >> 8);
			sp--;
			write(sp, pc & 0x00FF);
			pc = 0x0050;
			cycles = 5;
		}
		else if (((IF & (0x01 << 3)) > 0) && ((IE & (0x01 << 3)) > 0)) //Serial
		{
			IME = false;
			IF = IF & ~(0x01 << 3);
			sp--;
			write(sp, (pc & 0xFF00) >> 8);
			sp--;
			write(sp, pc & 0x00FF);
			pc = 0x0058;
			cycles = 5;
		}
		else if (((IF & (0x01 << 4)) > 0) && ((IE & (0x01 << 4)) > 0)) //Joypad
		{
			IME = false;
			IF = IF & ~(0x01 << 1);
			sp--;
			write(sp, (pc & 0xFF00) >> 8);
			sp--;
			write(sp, pc & 0x00FF);
			pc = 0x0060;
			cycles = 5;
		}
	}
}

//Do one cpu clock tick.
void SM83::clock()
{
	if (cycles <= 0)
	{
		//Handle HALT instruction first
		if (HALTFlag == true)
		{
			cycles--;
			if ((IE & IF) == 0)
			{
				cycles = 0; //No waiting
				return; 
			}
			else
			{
				cycles = 0;
				HALTFlag = false;
			}
		}

		irHandler(); //Call irHandler first to check if we need to hand control to the interrupt handler

		opcode = read(pc);

		//std::cout << "program counter: " << (std::hex) << " 0x" << (int)pc << std::endl;
		//std::cout << "A = " << (std::hex) << " 0x" << (int)A << std::endl;
		

		//If the opcode is prefixed by CB, the opcode is in a different bank of instructions
		if (opcode == 0xCB)
		{
			pc++;
			opcode = read(pc);
			operatePrefix(opcode);
			//std::cout << "PREFIX!!!" << std::endl;
			//std::cout <<"opcode: " << (std::hex) << " 0x" << (int)opcode << std::endl;
		}
		else
			operate(opcode);

		cycles *= 4; //Get T-cycles from M-cycles

		cycles--; //Decrement one T-cycle
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
	//HALT bug, pc fails to be incremented
	if (HALTBug == true)
		pcBug = pc;

	pc++; //Increment program counter
	Z = opcode & 0x07;
	Y = (opcode & 0x38) >> 3;
	X = (opcode & 0xC0) >> 6;
	Q = (opcode & (0x01 << 3)) >> 3;
	P = (opcode & 0x30) >> 4;

	//std::cout << "Opcode:" << (std::hex) << " 0x" << (int)opcode << std::endl;
	//std::cout << "SP:" << (std::hex) << " 0x" << (int)sp << std::endl;
	//std::cout << "x = " << (std::hex) << " 0x" << (int)X << std::endl;
	//std::cout << "Y = " << (std::hex) << " 0x" << (int)Y << std::endl;
	//std::cout << "Z = " << (std::hex) << " 0x" << (int)Z << std::endl;

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
				//Y -= 4;
				cycles += JRcce8(e8, Y - 4);
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
					//std::cout << (std::hex) <<"n16: " << "0x" << (int)n16 << std::endl;
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
				if (P == 3)
				{
					sp++;
					cycles += 2;
				}
				else
					cycles += INCr16(*rp[P]["H"], *rp[P]["L"]);
				break;
			case 1:
				if (P == 3)
				{
					sp--;
					cycles += 2;
				}
				else
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
			switch (Y)
			{
			case 4:
				n8 = read(pc);
				pc++;
				cycles += LDHbn16nA(n8);
				break;
			case 5:
				e8 = read(pc);
				pc++;
				cycles += ADDSPe8(e8);
				break;
			case 6:
				n8 = read(pc);
				pc++;
				cycles += LDHAbn16b(n8);
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
				if (P == 3)
					cycles += POPAF();
				else
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
			if (Y < 4)
			{
				n16 = (read(pc + 1) << 8) | read(pc);
				pc += 2;
				cycles += JPccn16(n16, Y);
			}
			switch (Y)
			{
			case 4:
				cycles += LDHbCbA();
				break;
			case 5:
				n16 = (read(pc + 1) << 8) | read(pc);
				pc += 2;
				cycles += LDbn16bA(n16);
				break;
			case 6:
				cycles += LDHAbCb();
				break;
			case 7:
				n16 = (read(pc + 1) << 8) | read(pc);
				pc += 2;
				cycles += LDAbn16b(n16);
				break;
			}
			break;

		case 3: //x=3, z=3
			switch (Y)
			{
			case 0:
				n16 = (read(pc + 1) << 8) | read(pc);
				//std::cout << "Jump Adress = " << (std::hex) << " 0x" << (int)n16 << std::endl;
				pc += 2;
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
			switch (Q)
			{
			case 0:
				if (P == 3) 
					cycles += PUSHAF();
				else
					cycles += PUSHr16(*rp2[P]["H"], *rp2[P]["L"]);
				break;
			case 1:
				n16 = (read(pc + 1) << 8) | read(pc);
				pc += 2;
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
		break;

		default:
			std::cout << "Illegal opcode:" << (std::hex) << " 0x" << (int)opcode << std::endl;
			break;
			
	}

	//HALT bug
	if (HALTBug == true)
	{
		pc = pcBug;
		HALTBug = false;
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
		std::cout << "Illegal opcode:" << (std::hex) << " 0x" << (int)opcode << std::endl;
		break;
	}



	/*
	//HALT bug
	if (HALTBug == true)
	{
		pc = pcBug;
		HALTBug = false;
	}
	*/
}

//--------------------------------------------------------------------------------------------------------------------------//
//-----------------------------------------CPU instruction function definitions here:---------------------------------------//
//--------------------------------------------------------------------------------------------------------------------------//

//Load instructions:

//Load (copy) value in register on the right into register on the left.
int SM83::LDr8r8(uint8_t& r1, uint8_t& r2)
{
	r1 = r2;
	return 1;
}

//Load value n8 into register r8.
int SM83::LDr8n8(uint8_t& r, uint8_t& n8)
{
	r = n8;
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
int SM83::LDrbHLbn8(uint8_t& n8)
{
	write((H << 8) | L, n8);
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
	write(r, A);
	return 2;
}

//Store value in register A into the byte at address n16.
int SM83::LDbn16bA(uint16_t& n)
{
	write(n, A);
	return 4;
}

//Store value in register A into the byte at address n16, provided the address is between $FF00 and $FFFF.
int SM83::LDHbn16nA(uint8_t& n)
{
	write(0xFF00 + n, A);
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
int SM83::LDHAbn16b(uint8_t& n)
{
	A = read(0xFF00 + n);
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
int SM83::ADCAr8(uint8_t r)
{
	if (getFlag(c) > 0)
	{
		A += 1;
		if (A == 0x00) setFlag(c, 1);
		else setFlag(c, 0);

		if ((A & 0x0F) == 0x00) setFlag(h, 1);
		else setFlag(h, 0);
	}

	else
	{
		setFlag(c, 0);
		setFlag(h, 0);
	}

	//Use temp variables with larger bits to avoid overflow
	uint16_t A_temp = A + r;

	//Find overflow from bit 7 and 3 from adding carry
	uint16_t no_carry_sum = A ^ r;
	uint16_t carry_into = A_temp ^ no_carry_sum;

	if ((carry_into & 0x10) > 0) setFlag(h, 1);


	if ((carry_into & 0x100) > 0) setFlag(c, 1);


	A = A + r;

	setFlag(n, 0);

	if (A == 0x00) setFlag(z, 1);
	else setFlag(z, 0);
	return 1;
}

//Add the byte pointed to by HL plus the carry flag to A.
int SM83::ADCAbHLb()
{
	HL = (H << 8) | L;
	
	if (getFlag(c) > 0)
	{
		A += 1;
		if (A == 0x00) setFlag(c, 1);
		else setFlag(c, 0);

		if ((A & 0x0F) == 0x00) setFlag(h, 1);
		else setFlag(h, 0);
	}

	else
	{
		setFlag(c, 0);
		setFlag(h, 0);
	}

	//Use temp variables with larger bits to avoid overflow
	uint8_t data = read(HL);
	uint16_t A_temp = A + data;

	//Find overflow from bit 7 and 3 from adding carry
	uint16_t no_carry_sum = A ^ data;
	uint16_t carry_into = A_temp ^ no_carry_sum;

	if ((carry_into & 0x10) > 0) setFlag(h, 1);


	if ((carry_into & 0x100) > 0) setFlag(c, 1);


	A = A + data;

	setFlag(n, 0);

	if (A == 0x00) setFlag(z, 1);
	else setFlag(z, 0);
	return 2;
}

//Add the value n8 plus the carry flag to A.
int SM83::ADCAn8(uint8_t& n8)
{
	if (getFlag(c) > 0)
	{
		A += 1;
		if (A == 0x00) setFlag(c, 1);
		else setFlag(c, 0);

		if ((A & 0x0F) == 0x00) setFlag(h, 1);
		else setFlag(h, 0);
	}

	else
	{
		setFlag(c, 0);
		setFlag(h, 0);
	}

	//Use temp variables with larger bits to avoid overflow
	uint16_t A_temp = A + n8;

	//Find overflow from bit 7 and 3 from adding carry
	uint16_t no_carry_sum = A ^ n8;
	uint16_t carry_into = A_temp ^ no_carry_sum;

	if ((carry_into & 0x10) > 0) setFlag(h, 1);
	

	if ((carry_into & 0x100) > 0) setFlag(c, 1);
	

	A = A + n8;

	setFlag(n, 0);
	
	if (A == 0x00) setFlag(z, 1);
	else setFlag(z, 0);
	return 2;
}

//Add the value in r8 to A.
int SM83::ADDAr8(uint8_t& r)
{
	
	if ((A & 0x0F) > (0x0F - (r & 0x0F))) setFlag(h, 1);
	else setFlag(h, 0);
	
	if (A > (0xFF - r)) setFlag(c, 1);
	else setFlag(c, 0);

	setFlag(n, 0);
	A += r;
	if (A == 0x00) setFlag(z, 1);
	else setFlag(z, 0);
	return 1;
}

//Add the byte pointed to by HL to A.
int SM83::ADDAbHLb()
{
	HL = (H << 8) | L;
	if ((A & 0x0F) > (0x0F - (read(HL) & 0x0F))) setFlag(h, 1);
	else setFlag(h, 0);

	if (A > (0xFF - read(HL))) setFlag(c, 1);
	else setFlag(c, 0);

	setFlag(n, 0);
	A += read(HL);
	if (A == 0x00) setFlag(z, 1);
	else setFlag(z, 0);
 	return 2;
}

//Add the value n8 to A.
int SM83::ADDAn8(uint8_t& n8)
{
	if ((A & 0x0F) > (0x0F - (n8 & 0x0F))) setFlag(h, 1);
	else setFlag(h, 0);

	if (A > (0xFF - n8)) setFlag(c, 1);
	else setFlag(c, 0);

	setFlag(n, 0);

	A += n8;
	if (A == 0x00) setFlag(z, 1);
	else setFlag(z, 0);

	return 2;
}

//Bitwise AND between the value in r8 and A.
int SM83::ANDAr8(uint8_t& r)
{
	setFlag(h, 1);
	setFlag(n, 0);
	setFlag(c, 0);
	A = A & r;
	if (A == 0x00) setFlag(z, 1);
	else setFlag(z, 0);
	return 1;
}

//Bitwise AND between the byte pointed to by HL and A.
int SM83::ANDAbHLB()
{
	HL = (H << 8) | L;
	setFlag(h, 1);
	setFlag(n, 0);
	setFlag(c, 0);
	A = A & read(HL);
	if (A == 0x00) setFlag(z, 1);
	else setFlag(z, 0);
	return 2;
}

//Bitwise AND between the value in n8 and A.
int SM83::ANDAn8(uint8_t& n8)
{
	setFlag(h, 1);
	setFlag(n, 0);
	setFlag(c, 0);
	A = A & n8;
	if (A == 0x00) setFlag(z, 1);
	else setFlag(z, 0);
	return 2;
}

//Subtract the value in r8 from A and set flags accordingly, but don't store the result. This is useful for Comparing values.
int SM83::CPAr8(uint8_t& r)
{
	if ((A - r) == 0x00) setFlag(z, 1);
	else setFlag(z, 0);
	setFlag(n, 1);

	if((A & 0x0F) < (r & 0x0F)) setFlag(h,1);
	else setFlag(h, 0);

	if (A < r) setFlag(c, 1);
	else setFlag(c, 0);
	return 1;
}

//Subtract the byte pointed to by HL from A and set flags accordingly, but don't store the result.
int SM83::CPAbHLb()
{
	HL = (H << 8) | L;
	if ((A - read(HL)) == 0x00) setFlag(z, 1);
	else setFlag(z, 0);

	setFlag(n, 1);
	if ((A & 0x0F) < (read(HL) & 0x0F)) setFlag(h, 1);
	else setFlag(h, 0);

	if (A < read(HL)) setFlag(c, 1);
	else setFlag(c, 0);
	return 2;
}

//Subtract the value n8 from A and set flags accordingly, but don't store the result.
int SM83::CPAn8(uint8_t& n8)
{
	if ((A - n8) == 0x00) setFlag(z, 1);
	else setFlag(z, 0);

	setFlag(n, 1);
	if ((A & 0x0F) < (n8 & 0x0F)) setFlag(h, 1);
	else setFlag(h, 0);

	if (A < n8) setFlag(c, 1);
	else setFlag(c, 0);
	return 2;
}

//Decrement value in register r8 by 1.
int SM83::DECr8(uint8_t& r)
{
	if ((r & 0x0F) < 0x01) setFlag(h, 1);
	else setFlag(h, 0);

	setFlag(n, 1);
	r--;
	if (r == 0x00) setFlag(z, 1);
	else setFlag(z, 0);
	return 1;
}

//Decrement the byte pointed to by HL by 1.
int SM83::DECbHLb()
{
	HL = (H << 8) | L;

	if ((read(HL) & 0x0F) < 0x01) setFlag(h, 1);
	else setFlag(h, 0);

	setFlag(n, 1);
	if (read(HL) - 1 == 0x00) setFlag(z, 1);
	else setFlag(z, 0);
	write(HL, read(HL) - 1);
	return 3;
}

int SM83::INCr8(uint8_t& r)
{
	if ((r & 0x0F) > (0x0F - 0x01)) setFlag(h, 1);
	else setFlag(h, 0);

	setFlag(n, 0);
	r++;
	if (r == 0x00) setFlag(z, 1);
	else setFlag(z, 0);

	//std::cout << (std::hex) << "0x" << (int)r << std::endl;

	return 1;
}

//Increment the byte pointed to by HL by 1.
int SM83::INCbHLb()
{
	HL = (H << 8) | L;

	uint8_t byte_temp = read(HL);
	byte_temp++;

	if ((read(HL) & 0x0F) > (0x0F - 0x01)) setFlag(h, 1);
	else setFlag(h, 0);

	setFlag(n, 0);
	if (byte_temp == 0x00)
	{
		setFlag(z, 1);
	}
	else setFlag(z, 0);
	write(HL, byte_temp);

	return 3;
}

//Store into A the bitwise OR of the value in r8 and A.
int SM83::ORAr8(uint8_t& r)
{
	A = A | r;
	if (A == 0x00) setFlag(z, 1);
	else setFlag(z, 0);
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
	else setFlag(z, 0);
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
	else setFlag(z, 0);
	setFlag(n, 0);
	setFlag(h, 0);
	setFlag(c, 0);
	return 2;
}

//Subtract the value in r8 and the carry flag from A.
int SM83::SBCAr8(uint8_t r)
{

	if (getFlag(c) > 0)
	{
		A -= 1;
		if (A == 0xFF) setFlag(c, 1);
		else setFlag(c, 0);

		if ((A & 0x0F) == 0x0F) setFlag(h, 1);
		else setFlag(h, 0);
	}

	else
	{
		setFlag(c, 0);
		setFlag(h, 0);
	}

	//Use temp variables with more bits to avoid overflow
	uint16_t A_temp = A - r;

	//Find borrow from bit 8 and 4
	uint16_t no_borrow_subtract = A ^ r;
	uint16_t borrow_from = A_temp ^ no_borrow_subtract;

	if ((borrow_from & 0x10) > 0) setFlag(h, 1);


	if ((borrow_from & 0x100) > 0) setFlag(c, 1);


	A = A - r;

	setFlag(n, 1);

	if (A == 0x00) setFlag(z, 1);
	else setFlag(z, 0);
	return 1;
}

int SM83::SBCAbHLb()
{
	HL = (H << 8) | L;

	if (getFlag(c) > 0)
	{
		A -= 1;
		if (A == 0xFF) setFlag(c, 1);
		else setFlag(c, 0);

		if ((A & 0x0F) == 0x0F) setFlag(h, 1);
		else setFlag(h, 0);
	}

	else
	{
		setFlag(c, 0);
		setFlag(h, 0);
	}

	//Use temp variables with larger bits to avoid overflow
	uint8_t data = read(HL);
	uint16_t A_temp = A - data;

	//Find borrow from bit 8 and 4
	uint16_t no_borrow_subtract = A ^ data;
	uint16_t borrow_from = A_temp ^ no_borrow_subtract;

	if ((borrow_from & 0x10) > 0) setFlag(h, 1);


	if ((borrow_from & 0x100) > 0) setFlag(c, 1);


	A = A - data;

	setFlag(n, 1);

	if (A == 0x00) setFlag(z, 1);
	else setFlag(z, 0);
	return 2;
}

//Subtract the value n8 and the carry flag from A.
int SM83::SBCAn8(uint8_t n8)
{

	if (getFlag(c) > 0)
	{
		A -= 1;
		if (A == 0xFF) setFlag(c, 1);
		else setFlag(c, 0);

		if ((A & 0x0F) == 0x0F) setFlag(h, 1);
		else setFlag(h, 0);
	}

	else
	{
		setFlag(c, 0);
		setFlag(h, 0);
	}

	//Use temp variables with larger bits to avoid overflow
	uint16_t A_temp = A - n8;

	//Find borrow from bit 8 and 4
	uint16_t no_borrow_subtract = A ^ n8;
	uint16_t borrow_from = A_temp ^ no_borrow_subtract;

	if ((borrow_from & 0x10) > 0) setFlag(h, 1);


	if ((borrow_from & 0x100) > 0) setFlag(c, 1);


	A = A - n8;

	setFlag(n, 1);

	if (A == 0x00) setFlag(z, 1);
	else setFlag(z, 0);
	return 2;
}

//Subtract the value in r8 from A.
int SM83::SUBAr8(uint8_t& r)
{
	if (A < r) setFlag(c, 1);
	else setFlag(c, 0);

	if((A & 0x0F) < (r & 0x0F)) setFlag(h, 1);
	else setFlag(h, 0);

	A -= r;
	if (A == 0x00) setFlag(z, 1);
	else setFlag(z, 0);
	setFlag(n, 1);
	return 1;
}

//Subtract the byte pointed to by HL from A.
int SM83::SUBAbHLb()
{
	HL = (H << 8) | L;
	if (A < read(HL)) setFlag(c, 1);
	else setFlag(c, 0);

	if ((A & 0x0F) < (read(HL) & 0x0F)) setFlag(h, 1);
	else setFlag(h, 0);

	A -= read(HL);
	if (A == 0x00) setFlag(z, 1);
	else setFlag(z, 0);
	setFlag(n, 1);
	return 2;
}

//Subtract the value n8 from A.
int SM83::SUBAn8(uint8_t n8)
{
	if (A < n8) setFlag(c, 1);
	else setFlag(c, 0);

	if ((A & 0x0F) < (n8 & 0x0F)) setFlag(h, 1);
	else setFlag(h, 0);

	A -= n8;

	if (A == 0x00) setFlag(z, 1);
	else setFlag(z, 0);
	setFlag(n, 1);
	return 2;
}

//Bitwise XOR between the value in r8 and A.
int SM83::XORAr8(uint8_t& r)
{
	A = A ^ r;
	if (A == 0x00) setFlag(z, 1);
	else setFlag(z, 0);
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
	else setFlag(z, 0);
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
	else setFlag(z, 0);
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
	else setFlag(c, 0);

	if ((HL & 0x0FFF) > (0x0FFF - (r & 0x0FFF))) setFlag(h, 1);
	else setFlag(h, 0);

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
	if ((r & (0x1 << u3)) == 0) setFlag(z, 1);
	else setFlag(z, 0);
	setFlag(h, 1);
	setFlag(n, 0);
	return 2;
}

int SM83::BITu3bHLb(uint8_t& u3)
{
	HL = (H << 8) | L;
	if ((read(HL) & (0x1 << u3)) == 0) setFlag(z, 1);
	else setFlag(z, 0);
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
	else setFlag(z, 0);
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
	else setFlag(z, 0);
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
	else setFlag(z, 0);
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
	else setFlag(z, 0);
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
	else setFlag(z, 0);
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
	else setFlag(z, 0);
	setFlag(n, 0);
	setFlag(h, 0);
	return 4;
}

//Rotate register A left.
int SM83::RLCA()
{
	setFlag(c, (A & 0x80) >> 7);
	A = (A << 1) | (A >> 7);
	setFlag(z, 0);
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
	else setFlag(z, 0);
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
	else setFlag(z, 0);
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
	else setFlag(z, 0);
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
	else setFlag(z, 0);
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
	else setFlag(z, 0);
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
	else setFlag(z, 0);
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
	else setFlag(z, 0);
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
	else setFlag(z, 0);
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
	else setFlag(z, 0);
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
	else setFlag(z, 0);
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
int SM83::JRcce8(int8_t e8, uint8_t y)
{
	switch (y)
	{
	case 0:
		if (getFlag(z) == 0)
		{
			pc += e8;
			return 3;
		}
		else 
			return 2;
		break;
	case 1:
		if (getFlag(z) == 1)
		{
			pc += e8;
			return 3;
		}
		else
			return 2;
		break;
	case 2:
		if (getFlag(c) == 0)
		{
			pc += e8;
			return 3;
		}
		else 
			return 2;
		break;
	case 3:
		if (getFlag(c) == 1)
		{
			pc += e8;
			return 3;
		}
		else
			return 2;
		break;
	} 
}

//Jump to address n16; effectively, store n16 into PC.
int SM83::JPn16(uint16_t& n16)
{
	pc = n16;
	return 3; //Jump is 3 cycles? Other sources report 4. TODO: check if this is correct.
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
			//std::cout << "Jump Adress = " << (std::hex) << " 0x" << (int)n16 << std::endl;
			return 4;
		}
		else 
			return 3;
	case 1:
		if (getFlag(z) == 1)
		{
			pc = n16;
			return 4;
		}
		else
			return 3;
	case 2:
		if (getFlag(c) == 0)
		{
			pc = n16;
			return 4;
		}
		else 
			return 3;
	case 3:
		if (getFlag(c) == 1)
		{
			pc = n16;
			return 4;
		}
		else 
			return 3;
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
	else setFlag(c, 0);

	if ((HL & 0x0FFF) > (0x0FFF - (sp & 0x0FFF))) setFlag(h, 1);
	else setFlag(h, 0);

	setFlag(n, 0);
	HL += sp;
	H = (HL & 0xFF00) >> 8;
	L = HL & 0x00FF;
	return 2;
}

//Add the signed value e8 to SP.
int SM83::ADDSPe8(int8_t e8)
{
	if(((sp & 0x000F) > (0x0F - ((uint8_t)e8 & 0x0F)))) setFlag(h,1);
	else setFlag(h, 0);

	if((sp & 0x00FF) > (0xFF - (uint8_t)e8)) setFlag(c, 1);
	else setFlag(c, 0);

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
	if (((sp & 0x000F) > (0x0F - ((uint8_t)e8 & 0x0F)))) setFlag(h, 1);
	else setFlag(h, 0);

	if ((sp & 0x00FF) > (0xFF - (uint8_t)e8)) setFlag(c, 1);
	else setFlag(c, 0);

	setFlag(z, 0);
	setFlag(n, 0);
	HL = sp + e8;

	H = (HL & 0xFF00) >> 8;
	L = (HL & 0x00FF);
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

int SM83::POPAF()
{
	F = read(sp) & 0xF0;
	A = read(sp + 1);
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

int SM83::PUSHAF()
{
	sp--;
	write(sp, A);
	sp--;
	write(sp, F & 0xF0);
	return 4;
}


//Miscellaneous Instructions

//Complement Carry Flag.
int SM83::CCF()
{
	F = F ^ (0x1 << 4); 
	setFlag(h, 0);
	setFlag(n, 0);
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

		if ((getFlag(c) == 1) || (A > 0x99))
		{
			A += 0x60;
			setFlag(c, 1);
		}

		if ((getFlag(h) == 1)  || ((A & 0x0F) > 0x9)) 
		{
			A += 0x06;
		}
	}
	else //Subtraction
	{
		if (getFlag(c) == 1)
		{
			A -= 0x60;
		}

		if (getFlag(h) == 1)
		{
			A -= 0x06;
		}
	}

	if(A == 0x00) setFlag(z, 1);
	else setFlag(z, 0);
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
	setFlag(n, 0);
	setFlag(h, 0);
	return 1;
}

//Enter CPU low-power consumption mode until an interrupt occurs. 
//The exact behavior of this instruction depends on the state of the IME flag.
void SM83::HALT()
{

	HALTFlag = true;

	//HALT bug
	if ((IE & IF) > 0 && IME == false)
		HALTBug = true;
	
	cycles = 0;
}

#ifdef DEBUG

void SM83::initializeInstrMap()
{
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

#endif // DEBUG



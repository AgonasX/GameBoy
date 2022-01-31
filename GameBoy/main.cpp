#define DEBUG

#include "Bus.h"
#include "SM83.h"
#include "SM83_PPU.h"
#include "Cartridge.h"

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"


class GameBoy : public olc::PixelGameEngine
{

private:
	//The GameBoy
	Bus GB;
	std::shared_ptr<Cartridge> cartridge;

	//Emulation stuff
	bool bRunEmulator = false;
	bool bStepEmulation = false;
	bool bRuntoBreak = false;
	bool bRun2500 = false;

#ifdef DEBUG
public:
	void drawInstr();
	std::string hex(uint32_t n, uint8_t d)
	{
		std::string s(d, '0');
		for (int i = d - 1; i >= 0; i--, n >>= 4)
			s[i] = "0123456789ABCDEF"[n & 0xF];
		return s;
	};
	std::string bin(uint32_t n, uint8_t b)
	{
		std::string s(b, '0');
		for (int i = b - 1; i >= 0; i--, n >>= 1)
			s[i] = "01"[n & 0x1];
		return s;
	};
private:
	uint8_t opcode = 0x00;
	uint16_t pc = 0x00;
	uint16_t a8 = 0x00;
	uint16_t a16 = 0x0000;
#endif // DEBUG



public:
	GameBoy()
	{
		// Name your application
		sAppName = "GameBoy";

	}

public:
	bool OnUserCreate() override	
	{
		// Called once at the start, so create things here
		
		//Initialize Gameboy and cartridge
		cartridge = std::make_shared<Cartridge>("Roms/cpu_instrs.gb");
		GB.loadCartridge(cartridge);

		//DEBUG struff
		#ifdef DEBUG
			GB.cpu.initializeInstrMap();
		#endif // DEBUG

		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		Clear(olc::BLACK);

			//DEBUG struff
	#ifdef DEBUG
			drawInstr();
	#endif // DEBUG


		//User input:
			if (GetKey(olc::Key::P).bHeld) { bRunEmulator = false; bRun2500 = false; } //Pause emulation
		if (GetKey(olc::Key::R).bHeld) { bRunEmulator = true; bRun2500 = false; } //Run emulation
		if (GetKey(olc::Key::T).bHeld) { bRunEmulator = false; bRun2500 = true; } //Run emulation
		if (GetKey(olc::Key::B).bPressed) bRuntoBreak = true;
		if (GetKey(olc::Key::SPACE).bPressed) bStepEmulation = true;

		//Run enough clock ticks for one instruction at a time:
		if (bRunEmulator)
		{
			do
			{
				GB.clock();
			} while (!GB.cpu.complete());
				
		}

		//Step instructions
		if (!bRunEmulator)
		{
			if (bStepEmulation)
			{
				do
				{
					GB.clock();
				} while (!GB.cpu.complete());
				bStepEmulation = false;
			}
		}


		//Run 2500 instructions at a time
		int instr = 1000;
		if (bRun2500)
		{
			while (instr > 0)
			{
				do
				{
					GB.clock();
				} while (!GB.cpu.complete());
				instr--;
			}
		}
	
		//Run to break
		if (bRuntoBreak)
		{
			do
			{
				GB.clock();
			} while (GB.cpu.pc != 0x481C);
			bRuntoBreak = false;
		}


		std::array<uint32_t, 24576>& tileData = GB.ppu.getTileTable();

		//Draw tile data in VRAM 
		for (int x = 0; x < 128; x++)
			for (int y = 0; y < ScreenHeight(); y++)
				Draw(x, y, olc::Pixel(tileData.at(x + y*128)));

		
		return true;
	}
};

int main()
{
	GameBoy emulator;
	if (emulator.Construct(128 + 300,192, 2, 2))
		emulator.Start();
	return 0;
}

//DEBUG struff
#ifdef DEBUG
void GameBoy::drawInstr()
{
	pc = GB.cpu.pc;
	opcode = GB.cpu.read(pc);
	if (opcode == 0xE0 || opcode == 0xF0 || opcode == 0x08 || opcode == 0xEA || opcode == 0xFA)
	{
		a8 = 0xFF00 + GB.cpu.read(pc + 1);
		a16 = (GB.cpu.read((pc + 2)) << 8) | (GB.cpu.read((pc + 1)));
	}
	
	//Instructions
	DrawString(128 + 3, 10, "Instr:");
	DrawString(128 + 3, 20, GB.cpu.instrMap[opcode]);

	//Program Counter
	DrawString(128 + 3, 30, "PC: $" + hex(pc, 4));

	//Registers
	DrawString(128 + 3, 40, "A: $" + hex(GB.cpu.A, 2));
	DrawString(128 + 3, 50, "F: $" + hex(GB.cpu.F, 2));
	DrawString(128 + 3, 60, "B: $" + hex(GB.cpu.B, 2));
	DrawString(128 + 3, 70, "C: $" + hex(GB.cpu.C, 2));
	DrawString(128 + 3, 80, "D: $" + hex(GB.cpu.D, 2));
	DrawString(128 + 3, 90, "E: $" + hex(GB.cpu.E, 2));
	DrawString(128 + 3, 100, "H: $" + hex(GB.cpu.H, 2));
	DrawString(128 + 3, 110, "L: $" + hex(GB.cpu.L, 2));

	//SP
	DrawString(128 + 3, 120, "SP: $" + hex(GB.cpu.sp, 4));

	//Values
	DrawString(128 + 3, 130, "PC+2: $" + hex(GB.cpu.read(pc + 2), 2));
	DrawString(128 + 3, 140, "PC+1: $" + hex(GB.cpu.read(pc + 1), 2));

	//Memory values
	DrawString(128 + 100, 10, "(AF): $" + hex(GB.cpu.read((GB.cpu.A<<8)| GB.cpu.F), 2));
	DrawString(128 + 100, 20, "(BC): $" + hex(GB.cpu.read((GB.cpu.B << 8) | GB.cpu.C), 2));
	DrawString(128 + 100, 30, "(DE): $" + hex(GB.cpu.read((GB.cpu.D << 8) | GB.cpu.E), 2));
	DrawString(128 + 100, 40, "(HL): $" + hex(GB.cpu.read((GB.cpu.H << 8) | GB.cpu.L), 2));
	DrawString(128 + 100, 50, "(a16): $" + hex(GB.cpu.read(a16), 2));
	DrawString(128 + 200, 50, "a16: $" + hex(a16, 4));
	DrawString(128 + 100, 60, "(a8): $" + hex(GB.cpu.read(a8), 2));
	DrawString(128 + 200, 60, "a8: $" + hex(a8, 4));

	//Interrupts
	DrawString(128 + 125, 70, "IME", GB.cpu.IME ? olc::GREEN : olc::RED);
	DrawString(128 + 100, 80, "IE: " + bin(GB.cpu.IE,4));
	DrawString(128 + 100, 90, "IF: " + bin(GB.cpu.IF,4));

	//Flags:
	DrawString(128 + 100, 100, "F: " + bin(GB.cpu.F, 8));

	//Stack
	DrawString(128 + 100, 110, "[SP] High: $" + hex(GB.cpu.read(GB.cpu.sp + 1), 2));
	DrawString(128 + 100, 120, "[SP] Low: $" + hex(GB.cpu.read(GB.cpu.sp), 2));


}
#endif // DEBUG


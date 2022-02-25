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
	bool bRun60fps = false;
	double fResidualTime = 0;

	//Draw pixels
	void DrawTileData(int X,int Y);
	void DrawTileMap0(int X, int Y);
	void DrawTileMap1(int X, int Y);
	void DrawLCDScreen(int X, int Y);

#ifdef DEBUG
public:
	void drawInstr(int X, int Y);

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
		cartridge = std::make_shared<Cartridge>("Roms/dmg-acid2.gb");
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
			drawInstr(288, 0);
			DrawTileData(160, 0);
			DrawTileMap0(0, 192);
			DrawTileMap1(256, 192);
	#endif // DEBUG


		//User input:
			if (GetKey(olc::Key::P).bHeld) { bRunEmulator = false; bRun2500 = false; bRun60fps = false; } //Pause emulation
		if (GetKey(olc::Key::R).bHeld) { bRunEmulator = true; bRun2500 = false; } //Run emulation
		if (GetKey(olc::Key::T).bHeld) { bRunEmulator = false; bRun2500 = true; } //Run emulation
		if (GetKey(olc::Key::B).bPressed) bRuntoBreak = true;
		if (GetKey(olc::Key::SPACE).bPressed) bStepEmulation = true;
		if (GetKey(olc::Key::ENTER).bPressed) bRun60fps = true;

		//Interrupts
		if (GetKey(olc::Key::S).bPressed) GB.cpu.irqLCDSTAT();

		//Run enough clock ticks for one instruction at a time:
		if (bRunEmulator)
		{
			do
			{
				GB.clock();
			} while (!GB.cpu.complete());
			//Also clock out remaining ticks for other devices connected to the bus
			do
			{
				GB.clock();
			} while (GB.cpu.complete());
				
		}

		// Run at 60 fps
		if (bRun60fps)
		{
			if (fResidualTime <= 0)
			{
				fResidualTime += 1 / 60.f - fElapsedTime;

				do
				{
					GB.clock();
				} while (!GB.ppu.frameComplete());
			}
			else
				fResidualTime -= fElapsedTime;
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

				//Also clock out remaining ticks for other devices connected to the bus
				do
				{
					GB.clock();
				} while (GB.cpu.complete());
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
			} while (GB.cpu.pc != 0xC000);
			//Also clock out remaining ticks for other devices connected to the bus
			do
			{
				GB.clock();
			} while (GB.cpu.complete());
			
			bRuntoBreak = false;
		}

		
		
		DrawLCDScreen(0, 0);

		return true;
	}
};

int main()
{
	GameBoy emulator;
	//Debug
#ifndef DEBUG
	if (emulator.Construct(160, 144, 3, 3))
		emulator.Start();
#else
	if (emulator.Construct(160 + 128 + 300, 192 + 256, 1, 1))
		emulator.Start();
#endif		
	return 0;
}


void GameBoy::DrawTileData(int X, int Y)
{
	std::array<uint32_t, 24576>& tileData = GB.ppu.getTileTable();

	//Draw tile data in VRAM 
	for (int x = 0; x < 128; x++)
		for (int y = 0; y < 192; y++)
			Draw(x + X, y + Y, olc::Pixel(tileData.at(x + y * 128)));

	//BG and Window tile area:
	if (GB.ppu.LCDC.BGAndWindowTileDataArea == 1)
	{
		DrawRect(0 + X, 0+ Y, 127,	128, olc::BLACK);
	}

	else
	{
		DrawRect(0 + X, 64 + Y, 127, 128, olc::BLACK);
	}
}

void GameBoy::DrawTileMap0(int X, int Y)
{
	std::array<uint32_t, 65536>& tileData = GB.ppu.getTileMap0Data();

	//Draw tile map 1
	for (int x = 0; x < 256; x++)
		for (int y = 0; y < 256; y++)
			Draw(x + X, y + Y, olc::Pixel(tileData.at(x + y * 256)));
}

void GameBoy::DrawTileMap1(int X, int Y)
{
	std::array<uint32_t, 65536>& tileData = GB.ppu.getTileMap1Data();

	//Draw tile map 1
	for (int x = 0; x < 256; x++)
		for (int y = 0; y < 256; y++)
			Draw(x + X, y + Y, olc::Pixel(tileData.at(x + y * 256)));

}

void GameBoy::DrawLCDScreen(int X, int Y)
{
	// Draw LCD screen
	for (int x = 0; x < 160; x++)
		for (int y = 0; y < 144; y++)
		{
			Draw(x + X, y + Y, olc::Pixel(GB.ppu.LCDscreen.at(x + y * 160)));
		}
}

//DEBUG struff
#ifdef DEBUG
void GameBoy::drawInstr(int X, int Y)
{
	pc = GB.cpu.pc;
	opcode = GB.cpu.read(pc);
	if (opcode == 0xE0 || opcode == 0xF0 || opcode == 0x08 || opcode == 0xEA || opcode == 0xFA)
	{
		a8 = 0xFF00 + GB.cpu.read(pc + 1);
		a16 = (GB.cpu.read((pc + 2)) << 8) | (GB.cpu.read((pc + 1)));
	}
	
	//Instructions
	DrawString(X + 3, Y + 10, "Instr:");
	DrawString(X + 3, Y + 20, GB.cpu.instrMap[opcode]);

	//Program Counter
	DrawString(X + 3, Y + 30, "PC: $" + hex(pc, 4));

	//Registers
	DrawString(X + 3, Y + 40, "A: $" + hex(GB.cpu.A, 2));
	DrawString(X + 3, Y + 50, "F: $" + hex(GB.cpu.F, 2));
	DrawString(X + 3, Y + 60, "B: $" + hex(GB.cpu.B, 2));
	DrawString(X + 3, Y + 70, "C: $" + hex(GB.cpu.C, 2));
	DrawString(X + 3, Y + 80, "D: $" + hex(GB.cpu.D, 2));
	DrawString(X + 3, Y + 90, "E: $" + hex(GB.cpu.E, 2));
	DrawString(X + 3, Y + 100, "H: $" + hex(GB.cpu.H, 2));
	DrawString(X + 3, Y + 110, "L: $" + hex(GB.cpu.L, 2));

	//SP
	DrawString(X + 3, Y + 120, "SP: $" + hex(GB.cpu.sp, 4));

	//Values
	DrawString(X + 3, Y + 130, "PC+2: $" + hex(GB.cpu.read(pc + 2), 2));
	DrawString(X + 3, Y + 140, "PC+1: $" + hex(GB.cpu.read(pc + 1), 2));

	//Memory values
	DrawString(X + 100, Y + 10, "(AF): $" + hex(GB.cpu.read((GB.cpu.A<<8)| GB.cpu.F), 2));
	DrawString(X + 100, Y + 20, "(BC): $" + hex(GB.cpu.read((GB.cpu.B << 8) | GB.cpu.C), 2));
	DrawString(X + 100, Y + 30, "(DE): $" + hex(GB.cpu.read((GB.cpu.D << 8) | GB.cpu.E), 2));
	DrawString(X + 100, Y + 40, "(HL): $" + hex(GB.cpu.read((GB.cpu.H << 8) | GB.cpu.L), 2));
	DrawString(X + 100, Y + 50, "(a16): $" + hex(GB.cpu.read(a16), 2));
	DrawString(X + 200, Y + 50, "a16: $" + hex(a16, 4));
	DrawString(X + 100, Y + 60, "(a8): $" + hex(GB.cpu.read(a8), 2));
	DrawString(X + 200, Y + 60, "a8: $" + hex(a8, 4));

	//Interrupts
	DrawString(X + 125, Y + 70, "IME", GB.cpu.IME ? olc::GREEN : olc::RED);
	DrawString(X + 100, Y + 80, "IE: " + bin(GB.cpu.IE,4));
	DrawString(X + 100, Y + 90, "IF: " + bin(GB.cpu.IF,4));

	//PPU
	DrawString(X + 125, Y + 130, "PPU Enable", GB.ppu.LCDC.PPUEnable == 1 ? olc::GREEN : olc::RED);
	DrawString(X + 100, Y + 140, "BG/Window Area:" + hex(GB.ppu.LCDC.BGAndWindowTileDataArea, 2));
	DrawString(X + 100, Y + 150, "BG Area:" + hex(GB.ppu.LCDC.BGTileMapArea,2));
	DrawString(X + 100, Y + 160, "Window Area:" + hex(GB.ppu.LCDC.WindowsTileMapArea, 2));
	DrawString(X + 100, Y + 170, "STAT = " + bin(GB.ppu.STAT.reg, 8));
	


	//Flags:
	DrawString(X + 100, Y + 100, "F: " + bin(GB.cpu.F, 8));

	//Stack
	DrawString(X + 100, Y + 110, "[SP] High: $" + hex(GB.cpu.read(GB.cpu.sp + 1), 2));
	DrawString(X + 100, Y + 120, "[SP] Low: $" + hex(GB.cpu.read(GB.cpu.sp), 2));


}
#endif // DEBUG


#include "Bus.h"
#include "SM83.h"
#include "SM83_PPU.h"
#include "Cartridge.h"

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

class GameBoy : public olc::PixelGameEngine
{

private:
	Bus GB;
	std::shared_ptr<Cartridge> cartridge;

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
		cartridge = std::make_shared<Cartridge>("cpu_instrs.gb");
		GB.loadCartridge(cartridge);

		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{

		//Run as fast as possible:
		GB.cpu.clock();

		std::array<uint32_t, 24576>& tileData = GB.ppu.getTileTable();

		// Called once per frame, draws random coloured pixels
		for (int x = 0; x < ScreenWidth(); x++)
			for (int y = 0; y < ScreenHeight(); y++)
				Draw(x, y, olc::Pixel(tileData.at(x + y*256)));
		return true;
	}
};

int main()
{
	GameBoy emulator;
	if (emulator.Construct(256,96, 4, 4))
		emulator.Start();
	return 0;
}
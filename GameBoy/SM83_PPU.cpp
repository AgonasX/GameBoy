#include "SM83_PPU.h"

SM83_PPU::SM83_PPU()
{
}

SM83_PPU::~SM83_PPU()
{
}

void SM83_PPU::ppuWrite(uint16_t address, uint8_t data)
{
	if (0x8000 >= address && address <= 0x9FFF)
	{
		VRAM.at(address - 0x8000) = data;
	}
}

uint8_t SM83_PPU::ppuRead(uint16_t address)
{
	uint8_t data = 0x00;
	if (0x8000 >= address && address <= 0x9FFF)
	{
		data = VRAM.at(address - 0x8000);
	}

	return data;
}

void SM83_PPU::cpuWrite(uint16_t address, uint8_t data)
{
	//CPU does not have access to VRAM during mode 3
	if (0x8000 >= address && address <= 0x9FFF)
	{
		if (STAT.modeFlag != 3)
			ppuWrite(address, data);
	}

}

uint8_t SM83_PPU::cpuRead(uint16_t address)
{
	uint8_t data = 0x00;

	//CPU does not have access to VRAM during mode 3
	if (0x8000 >= address && address <= 0x9FFF)
	{
		if (STAT.modeFlag == 3)
			data = 0xFF;

		else
			data = ppuRead(address);
	}

	return data;
}

void SM83_PPU::clock()
{
}

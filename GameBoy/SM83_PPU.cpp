#include "SM83_PPU.h"

SM83_PPU::SM83_PPU()
{
	//Reset registers
	STAT.reg = 0x00;
	STAT.modeFlag = 3; //Hacked in for drawing tile data
	LCDC.reg = 0x00;

	//Palett colors (abgr)
	palettes.at(0) = 0xFF0FBC9B;
	palettes.at(1) = 0xFF0FAC8B;
	palettes.at(2) = 0xFF306230;
	palettes.at(3) = 0xFF0F380F;

	BGP.reg = 0x00; //Hacked in for testing

}

SM83_PPU::~SM83_PPU()
{
}

void SM83_PPU::ppuWrite(uint16_t address, uint8_t data)
{
	//VRAM
	if (0x8000 >= address && address <= 0x9FFF)
	{
		VRAM.at(address - 0x8000) = data;
	}

	//Palett registers
	if (address == 0xFF47) //BG palett data
		BGP.reg = data;

	//LCD 
	if (address == 0xFF40) //LCD Control (LCDC)
		LCDC.reg = data;
	if (address == 0xFF41) //LCD status register (STAT)
		STAT.reg = data; 
}

uint8_t SM83_PPU::ppuRead(uint16_t address)
{
	uint8_t data = 0x00;
	//VRAM
	if (0x8000 >= address && address <= 0x9FFF)
	{
		data = VRAM.at(address - 0x8000);
	}

	//Palett registers
	if (address == 0xFF47) //BG palett data
		data = BGP.reg;

	//LCD 
	if (address == 0xFF40) //LCD Control (LCDC)
		data = LCDC.reg;
	if (address == 0xFF41) //LCD status register (STAT)
		data = STAT.reg ;

	return data;
}

void SM83_PPU::cpuWrite(uint16_t address, uint8_t data)
{
	//VRAM
	if (0x8000 >= address && address <= 0x9FFF)
	{
		//CPU have full access to VRAM and OPM if PPU disabled
		if (LCDC.PPUEnable == 0) {
			ppuWrite(address, data);
		}

		else
		{
		//CPU does not have access to VRAM during mode 3
			if (STAT.modeFlag != 3)
				ppuWrite(address, data);
			
		}
	}

	//Registers and miscellaneous
	else
		ppuWrite(address, data);

}

uint8_t SM83_PPU::cpuRead(uint16_t address)
{
	uint8_t data = 0x00;
	//VRAM
	if (0x8000 >= address && address <= 0x9FFF)
	{
		//CPU have full access to VRAM and OPM if PPU disabled
		if (LCDC.PPUEnable == 0) {
			data = ppuRead(address);
		}

		else
		{
			//CPU does not have access to VRAM during mode 3
			if (STAT.modeFlag == 3)
				data = 0xFF;
			else
				data = ppuRead(address);

		}
	}

	//Registers and miscellaneous
	else
		data = ppuRead(address);

	return data;
}

void SM83_PPU::clock()
{
}

//For debugging
std::array<uint32_t, 24576>& SM83_PPU::getTileTable()
{
	//Loop through 32*12 tiles = 32*12*8*8 = 24576 pixels
	for (int nTileY = 0; nTileY < 12; nTileY++)
	{
		for (int nTileX = 0; nTileX < 32; nTileX++)
		{
			//Loop trough pixel in the tile
			for (int y = 0; y < 8; y++)
			{
				for (int x = 0; x < 8; x++)
				{
					//Get Colorindex and the corresponding rgb value. The palettes are decided by BGP register.
					RowLowByte = VRAM.at(2 * y + (nTileX + 32 * nTileY) * 16);
					RowHighByte = VRAM.at((2 * y + 1) + (nTileX + 32 * nTileY) * 16);
					lowBit = (RowLowByte & (0x1 << (7 - x))) >> (7 - x);
					highBit = (RowHighByte & (0x1 << (7 - x))) >> (6 - x);
					colorIndex = highBit | lowBit;
					argb = palettes.at((BGP.reg & (0x03 << 2 * colorIndex)) >> 2 * colorIndex);

					tileData.at((nTileX * 8 + 8 * 32 * 8 * nTileY) + (x + 32 * 8 * y)) = argb;
				}
			}
		}
	}

	return tileData;
}
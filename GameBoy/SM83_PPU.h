#pragma once
#include <array>
#include <cstdint>
class SM83_PPU
{
public:
	SM83_PPU();
	~SM83_PPU();


private:
	//PPU read and write.
	void ppuWrite(uint16_t address, uint8_t data);
	uint8_t ppuRead(uint16_t address);

public:
	//CPU read and write
	void cpuWrite(uint16_t address, uint8_t data);
	uint8_t cpuRead(uint16_t address);


public:
	//Clock cpu
	void clock();

public:
	//PPU registers

	//LCDC register
	union
	{
		struct
		{
			uint8_t PPUEnable : 1;
			uint8_t WindowsTileMapArea : 1;
			uint8_t WindowEnable : 1;
			uint8_t BGAndWindowTileDataArea : 1;
			uint8_t BGTileMapArea : 1;
			uint8_t OBJSize : 1;
			uint8_t OBJEnable : 1;
			uint8_t BGAndWindowPriority : 1;
		};
		uint8_t reg;
	} LCDC;

	//LCD status register
	union
	{
		struct
		{
			uint8_t modeFlag : 2;
			uint8_t LYFlag : 2;
			uint8_t HBLank : 1;
			uint8_t VBlank : 1;
			uint8_t OAM : 1;
			uint8_t LY : 1;
			uint8_t unused : 1;
		};
		uint8_t reg;
	} STAT;

	//Palett registers

	//Background palett register
	union
	{
		struct
		{
			uint8_t index0 : 2;
			uint8_t index1 : 2;
			uint8_t index2 : 2;
			uint8_t index3 : 2;
		};
		uint8_t reg;
	} BGP;

	

public:
	//For debugging
	std::array<uint32_t, 24576> tileData = { 0x00000000 };
	std::array<uint32_t, 24576>& getTileTable();
	uint32_t argb = 0x00000000;
	uint8_t RowLowByte = 0x00;
	uint8_t RowHighByte = 0x00;
	uint8_t lowBit = 0x00;
	uint8_t highBit = 0x00;
	uint8_t colorIndex = 0x00;
			
private:
	//VRAM
	std::array<uint8_t, 8192> VRAM = { 0x00 };

private:
	//other variables and functions to faciliate emulation
	std::array<uint32_t, 4> palettes = { 0x00 };



};


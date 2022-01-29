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
	uint8_t LCDC = 0x00;
	union
	{
		struct
		{
			uint8_t LY : 1;
			uint8_t OAM : 1;
			uint8_t VBlank : 1;
			uint8_t HBLank : 1;
			uint8_t LYFlag : 2;
			uint8_t modeFlag : 2;
		};
		uint8_t reg;
	} STAT;
			
private:
	//VRAM
	std::array<uint8_t, 8192> VRAM = { 0x00 };

private:
	//other variables and functions to faciliate emulation


};


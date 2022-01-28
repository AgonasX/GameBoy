#pragma once
#include <array>
#include "SM83.h"
#include "SM83_PPU.h"
#include "Cartridge.h"

class Bus
{

public: 
	//Constructor and deconstructor
	Bus();
	~Bus();


public:
	//Devices on the bus
	std::shared_ptr<Cartridge> cart; //Cartridge
	std::array<uint8_t, 16383> WRAM = { 0x00 };
	SM83 cpu; //CPU
	SM83_PPU ppu; //PPU

public:
	//Connect cartridge to the bus
	void loadCartridge(const std::shared_ptr<Cartridge> cartridge);

public:
	//Read and write
	 uint8_t cpuRead(uint16_t address);
	 bool cpuWrite(uint16_t address, uint8_t data);
};


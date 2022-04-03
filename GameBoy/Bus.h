#pragma once
#include <array>
#include "SM83.h"
#include "SM83_PPU.h"
#include "Cartridge.h"
#include "Timer.h"

class Bus
{

public: 
	//Constructor and deconstructor
	Bus();
	~Bus();


public:
	//Devices on the bus
	std::shared_ptr<Cartridge> cart; //Cartridge
	std::array<uint8_t, 8192> WRAM = { 0x00 }; //8192kB Work RAM
	SM83 cpu; //CPU
	SM83_PPU ppu; //PPU
	Timer timer; //Timer

private:
	//OAM DMA transfer
	uint8_t DMAreg = 0x00; //0xFF46 DMA register
	bool bDMATransfer = false;
	int DMACycles = 0;
	void DMA(uint8_t data);

public:
	//Connect cartridge to the bus
	void loadCartridge(const std::shared_ptr<Cartridge> cartridge);

public:
	//Read and write
	 uint8_t cpuRead(uint16_t address);
	 bool cpuWrite(uint16_t address, uint8_t data);

public:
	//The master clock
	void clock();
	uint8_t clockTicks = 0x00;
	uint8_t TimerTicks = 0x00;
};


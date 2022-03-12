#include "Bus.h"

Bus::Bus()
{
	cpu.connectBus(this);
	ppu.connectBus(this);
}

Bus::~Bus()
{
}

void Bus::DMA(uint8_t data)
{
	DMACycles--;
	if (DMACycles == 0)
	{
		uint16_t Source = data << 8;
		for (uint8_t i = 0x00; i < 0xA0; i++)
		{
			ppu.ppuWrite(0xFE + i, cpuRead(Source + i));
		}

		bDMATransfer = false;
	}
}

void Bus::loadCartridge(const std::shared_ptr<Cartridge> cartridge)
{
	this->cart = cartridge;
	std::cout << "LOOOAD cart" << std::endl;
	std::cout << cart << std::endl;
	
}

uint8_t Bus::cpuRead(uint16_t address)
{
	uint8_t data = 0x00;

	//Cartridge
	if (0x0000 <= address && address <= 0x7FFF)
		data = cart->cpuRead(address);
	
	if (0xA000 <= address && address <= 0xBFFF) //Cartridge RAM
		data = cart->cpuRead(address);

	//PPU VRAM
	if (0x8000 <= address && address <= 0x9FFF)
		data = ppu.cpuRead(address);

	//OAM
	if (0xFE00 <= address && address <= 0xFE9F)
		data = ppu.cpuRead(address);

	//LCD registers
	if (0xFF40 <= address && address <= 0xFF4B)
		data = ppu.cpuRead(address);

	//WRAM
	if (0xC000 <= address && address <= 0xDFFF)
		data = WRAM.at((address - 0xC000));
	if (0xE000 <= address && address <= 0xFDFF)
		data = WRAM.at(((address - 0x2000) - 0xC000)); //Mirror of 0xC000-0xDDFF

	//Interrupt enabler register
	if (address == 0xFFFF)
		data = cpu.IE;
	
	//Interrupt flag register
	if (address == 0xFF0F)
		data = cpu.IF;

	//Direct Memory Access
	if (address == 0xFF46)
		data = DMAreg;

	//Block access when in DMA transfer except for HRAM
	if (bDMATransfer)
		data = 0x00;

	//HRAM
	if (0xFF80 <= address && address <= 0xFFEE)
		data = cpu.HRAM.at(address - 0xFF80);
		
	return data;
}
bool Bus::cpuWrite(uint16_t address, uint8_t data)
{
	if (!bDMATransfer) //Block access when in DMA transfer except for HRAM
	{

		//Cartridge
		if (0x0000 <= address && address <= 0x7FFF)
			cart->cpuWrite(address, data);

		//External Cartridge RAM
		if (0xA000 <= address && address <= 0xBFFF)
			cart->cpuWrite(address, data);


		//PPU VRAM
		if (0x8000 <= address && address <= 0x9FFF)
			ppu.cpuWrite(address, data);

		//OAM
		if (0xFE00 <= address && address <= 0xFE9F)
			ppu.cpuWrite(address, data);

		//LCD registers
		if (0xFF40 <= address && address <= 0xFF4B)
			ppu.cpuWrite(address, data);

		//WRAM
		if (0xC000 <= address && address <= 0xDFFF)
			WRAM.at((address - 0xC000)) = data;
		if (0xE000 <= address && address <= 0xFDFF)
			WRAM.at(((address - 0x2000) - 0xC000)) = data; //Mirror of 0xC000-0xDDFF

		//Interrupt enabler register
		if (address == 0xFFFF)
			cpu.IE = data;

		//Interrupt flag register
		if (address == 0xFF0F)
			cpu.IF = data;

		//Direct Memory Access Transfer
		if (address == 0xFF46)
		{
			std::cout << "DMA time!!!"<<std::endl;
			DMAreg = data;
			bDMATransfer = true;
			DMACycles = 640;
		}
	}

	else
	{
		//HRAM
		if (0xFF80 <= address && address <= 0xFFEE)
			cpu.HRAM.at(address - 0xFF80) = data;
	}

	return true;
}

//Clock the CPU and PPU 
void Bus::clock()
{
	if((clockTicks & 0x1) == 0) cpu.clock(); //Clock cpu every two ticks

	//Clock PPU only when enabled
	//ppu.LCDC.PPUEnable = 1; //Force ppu clock
	if(ppu.LCDC.PPUEnable == 1) ppu.clock(); //Clock ppu every tick

	//DMA transfer
	if (bDMATransfer) DMA(DMAreg);

	clockTicks++;
}



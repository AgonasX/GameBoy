#include "Bus.h"

Bus::Bus()
{
	cpu.connectBus(this);
	ppu.connectBus(this);
}

Bus::~Bus()
{
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
	//std::cout << "cart null?" << std::endl;
	//std::cout << cart << std::endl;

	//Cartridge
	if (0x0000 <= address && address <= 0x7FFF)
		data = cart->cpuRead(address);
	
	if (0xA000 <= address && address <= 0xBFFF) //Cartridge RAM
		data = cart->cpuRead(address);

	//PPU VRAM and OPM
	if (0x8000 <= address && address <= 0x9FFF)
		data = ppu.cpuRead(address);

	//LCD registers
	if (0xFF40 <= address && address <= 0xFF4B)
		data = ppu.cpuRead(address);


	//WRAM
	if (0xC000 <= address && address <= 0xFDFF)
		data = WRAM.at((address - 0xC000) & 0x1FFF); //0xE000 to 0xFDFF mirrors 0xC000 to 0xDFFF
	
	//HRAM
	if (0xFF80 <= address && address <= 0xFFEE)
		data = cpu.HRAM.at(address - 0xFF80);

	//Interrupt enabler register
	if (address == 0xFFFF)
		data = cpu.IE;
	
	
	//Interrupt flag register
	if (address == 0xFF0F)
		data = cpu.IF;
		
	return data;
}
bool Bus::cpuWrite(uint16_t address, uint8_t data)
{
	//Cartridge
	if (0x0000 <= address && address <= 0x7FFF)
		cart->cpuWrite(address, data);

	
	if (0xA000 <= address && address <= 0xBFFF)
		cart->cpuWrite(address, data);

	//PPU VRAM and OPM
	if (0x8000 <= address && address <= 0x9FFF)
		ppu.cpuWrite(address, data);

	
	//LCD registers
	if (0xFF40 <= address && address <= 0xFF4B)
		ppu.cpuWrite(address, data);


	//WRAM
	if (0xC000 <= address && address <= 0xFDFF)
		WRAM.at((address - 0xC000) & 0x1FFF) = data; //0xE000 to 0xFDFF mirrors 0xC000 to 0xDFFF


	//HRAM
	if (0xFF80 <= address && address <= 0xFFEE)
	{
		std::cout << "AAAA" << std::endl;
		cpu.HRAM.at(address - 0xFF80) = data;
	}

	//Interrupt enabler register
	if (address == 0xFFFF)
		cpu.IE = data;

	//Interrupt flag register
	if (address == 0xFF0F)
		cpu.IF = data;
		
	return true;
}

//Clock the CPU and PPU 
void Bus::clock()
{
	if((clockTicks & 0x1) == 1) cpu.clock(); //Clock cpu every two ticks
	ppu.clock(); //Clock ppu every tick
	clockTicks++;
}



#include "Bus.h"

Bus::Bus()
{
	cpu.connectBus(this);
}

Bus::~Bus()
{
}

void Bus::loadCartridge(const std::shared_ptr<Cartridge> cartridge)
{
	this->cart = cartridge;
}

uint8_t Bus::cpuRead(uint16_t address)
{
	uint8_t data = 0x00;
	//Cartridge
	if (0x0000 <= address && address <= 0x3FFF)
		data = cart->cpuRead(address);

	if (0xA000 <= address && address <= 0xBFFF)
		data = cart->cpuRead(address);

	//PPU VRAM and OPM
	if (0x8000 <= address && address <= 0x9FFF)
		data = ppu.cpuRead(address);

	//WRAM
	if (0xC000 <= address && address <= 0xFDFF)
		data = WRAM.at((address & 0xDFFF) - 0xC000); //0xE000 to 0xFDFF mirrors 0xC000 to 0xDFFF

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
	if (0x0000 <= address && address <= 0x3FFF)
		cart->cpuWrite(address, data);

	if (0xA000 <= address && address <= 0xBFFF)
		cart->cpuWrite(address, data);

	//PPU VRAM and OPM
	if (0x8000 <= address && address <= 0x9FFF)
		ppu.cpuWrite(address, data);

	//WRAM
	if (0xC000 <= address && address <= 0xFDFF)
		WRAM.at((address - 0xC000) & 0x1FFF) = data; //0xE000 to 0xFDFF mirrors 0xC000 to 0xDFFF

	//Interrupt enabler register
	if (address == 0xFFFF)
		cpu.IE = data;

	//Interrupt flag register
	if (address == 0xFF0F)
		cpu.IF = data;
	return true;
}



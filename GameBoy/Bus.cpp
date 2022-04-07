#include "Bus.h"

Bus::Bus()
{
	cpu.connectBus(this);
	ppu.connectBus(this);
	timer.ConnectToBus(this);
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

	//Boot
	if (0x0000 <= address && address <= 0x00FF && bBootROM)
		data = Boot.at(address);
	
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

	//Timer and Divider registers
	if (0xFF04 <= address && address <= 0xFF07)
		data = timer.cpuRead(address);

	//Block access when in DMA transfer except for HRAM
	if (bDMATransfer)
		data = 0x00;

	//HRAM
	if (0xFF80 <= address && address <= 0xFFFE)
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

		//Timer and Divider registers
		if (0xFF04 <= address && address <= 0xFF07)
			timer.cpuWrite(address, data);

		//Disable DMG Boot ROM
		if (address == 0xFF50)
			bBootROM = data > 0x00 ? 0 : 1;

		//Direct Memory Access Transfer
		if (address == 0xFF46)
		{
			std::cout << "DMA time!!!"<<std::endl;
			DMAreg = data;
			bDMATransfer = true;
			DMACycles = 640;
		}
	}

	//HRAM
	if (0xFF80 <= address && address <= 0xFFFE)
		cpu.HRAM.at(address - 0xFF80) = data;
	

	return true;
}

//Clock the CPU and PPU 
void Bus::clock()
{
	cpu.clock(); //Clock cpu 

	//Clock PPU only when enabled
	//ppu.LCDC.PPUEnable = 1; //Force ppu clock
	//if(ppu.LCDC.PPUEnable == 1) ppu.clock(); //Clock ppu every tick
	ppu.clock();

	//DMA transfer
	if (bDMATransfer) DMA(DMAreg);

	//Timers and dividers
	if ((clockTicks % 256) == 0) timer.ClockDIV();
	if (timer.TAC.TimerEnable == 1)
	{
		if ((TimerTicks % timer.ClockSelect[timer.TAC.InputClockSelect]) == 0) timer.ClockTimer();
	
		TimerTicks++;
	}
	else
		TimerTicks = 0;

	clockTicks++;
}



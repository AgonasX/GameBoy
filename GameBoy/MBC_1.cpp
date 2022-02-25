#include "MBC_1.h"
#include <iostream>

MBC_1::MBC_1(std::shared_ptr<std::vector<uint8_t>> pMem, std::shared_ptr<std::vector<uint8_t>> pram)
{
	pPGRMemory = pMem;
	pRAM = pram;
}

MBC_1::~MBC_1()
{
}

//TODO: implement Banking modes
uint8_t MBC_1::MBCRead(uint16_t address)
{
	uint8_t data = 0x00;

	//Bank 0x00
	if (0x0000 <= address && address <= 0x3FFF) 
		data = pPGRMemory->at(address);

	//ROM bank 0x01-0x7F
	if (0x4000 <= address && address <= 0x7FFF)
	{
		ROMBankNumber &= 0x1F;
		if (ROMBankNumber == 0x00 || ROMBankNumber == 0x20 || ROMBankNumber == 0x40 || ROMBankNumber == 0x60)
			ROMBankNumber++;
		data = pPGRMemory->at(address + (ROMBankNumber - 1) * 0x4000);
	}

	//RAM 
	if (0xA000 <= address && address <= 0xBFFF)
	{
		if ((RAMEnable & 0x0F) == 0x0A) //Ram must be enabled first
		{
			RAMBankNumber &= 0x3;
			data = pRAM->at((address - 0xA000) + RAMBankNumber * 0x1FFF);
		}
	}

	return data;
}

void MBC_1::MBCWrite(uint16_t address, uint8_t data)
{

	//RAM enable
	if (0x0000 <= address && address <= 0x1FFF)
	{
		WriteToRamEnable = true;
		RAMEnable = data;
		std::cout << "RAM enabled?:" << (std::hex) << (int)RAMEnable << std::endl;
		
	}

	//ROM bank number
	if (0x2000 <= address && address <= 0x3FFF) 
		ROMBankNumber = data;

	//RAM bank number
	if (0x4000 <= address && address <= 0x5FFF) 
		RAMBankNumber = data;

	//Banking mode select
	if (0x6000 <= address && address <= 0x7FFF) 
		BankingModeSelect = data;

	//RAM
	if (0xA000 <= address && address <= 0xBFFF)
	{
		if ((RAMEnable & 0x0F) == 0x0A) //Ram must be enabled first
		{
			RAMBankNumber &= 0x3;
			pRAM->at((address - 0xA000) + RAMBankNumber * 0x1FFF) = data;
		}
	}
}

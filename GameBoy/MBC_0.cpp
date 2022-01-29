#include "MBC_0.h"
#include <iostream>

MBC_0::MBC_0(std::shared_ptr<std::vector<uint8_t>> pMem, std::shared_ptr<std::vector<uint8_t>> pram)
{
	pPGRMemory = pMem;
	pRAM = pram;
}

MBC_0::~MBC_0()
{
}

uint8_t MBC_0::MBCRead(uint16_t address)
{
	uint8_t data = 0x00;
	//if (0x00 <= address && address <= 0x7FFF)
	//{
		data = pPGRMemory->at(address);
	//}
	return data;
}

void MBC_0::MBCWrite(uint16_t address, uint8_t data)
{

	//No RAM, no writes

	pPGRMemory->at(address) = data; //Hacked in for testing
	//std::cout << "DATA Mem: " << (std::hex) << "0x" << (int)pPGRMemory->at(address) << std::endl;
	//std::cout << "ADDRESS: " << (std::hex) << "0x" << (int)address << std::endl;
	//std::cout << "DATA: " << (std::hex) << "0x" << (int)data << std::endl;
}



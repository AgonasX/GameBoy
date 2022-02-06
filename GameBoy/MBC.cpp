#include "MBC.h"

MBC::MBC()
{
}

MBC::~MBC()
{
}

uint8_t MBC::MBCRead(uint16_t address)
{
	return 0;
}

void MBC::MBCWrite(uint16_t address, uint8_t data)
{
}

void MBC::setRamBanks(uint8_t Banks)
{
	RAMBanks = Banks;
}

void MBC::setMemoryBanks(uint8_t Banks)
{
	MemoryBanks = Banks;
}




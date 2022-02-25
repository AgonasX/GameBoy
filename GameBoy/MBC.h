#pragma once
#include <cstdint>
#include <vector>
#include <memory>

class MBC
{
public:
	MBC();
	~MBC();

public:
	//MBC read and write
	virtual uint8_t MBCRead(uint16_t address);
	virtual void MBCWrite(uint16_t address, uint8_t data);

protected:
	//RAM and Memory
	std::shared_ptr<std::vector<uint8_t>> pPGRMemory;
	std::shared_ptr<std::vector<uint8_t>> pRAM;

protected:
	//Number of banks
	uint8_t RAMBanks = 0;
	uint8_t MemoryBanks = 0;

public:
	//Set number of banks
	void setRamBanks(uint8_t Banks);
	void setMemoryBanks(uint8_t Banks);

public:
	//Utility bool for testing
	bool WriteToRamEnable = false;
};


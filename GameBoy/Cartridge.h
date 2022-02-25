#pragma once
#include <string.h>
#include <fstream>
#include <vector>
#include <stdio.h>
#include <iostream>

#include "MBC.h"
#include "MBC_0.h"
#include "MBC_1.h"

class Cartridge
{
public:
	Cartridge(std::string fileName);
	~Cartridge();
	
private:
	//Program memory and RAM
	std::shared_ptr<std::vector<uint8_t>> vPGRMemory;
	std::shared_ptr<std::vector<uint8_t>> vRAM;

	/*
private:
	//Cartridge header
	struct
	{
		char* EntryPoint[4];
		char* NintendoLogo[48];
		char* Title[16];
		char* NewLicenseeCode[2];
		char* SBGFlag[1];
		char* CartridgeType[1];
		char* RomSize[1];
		char* RamSize[1];
	}header;
	*/

private:
	//ROM size and cartridge type
	uint32_t RAMsize = 0;
	uint32_t cartridgeType = 0x00; 
	

public: //Public for debug
	//MBC
	std::unique_ptr<MBC> pMBC;
	uint16_t mappedAddress = 0x00;


public:
	//CPU read and write
	uint8_t cpuRead(uint16_t address);
	void cpuWrite(uint16_t address, uint8_t data);


};


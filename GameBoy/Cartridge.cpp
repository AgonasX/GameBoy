#include "Cartridge.h"

//Load program memory and resize PGRMemory and RAM appropiately 
Cartridge::Cartridge(std::string fileName)
{
	vPGRMemory = std::make_shared <std::vector<uint8_t >>();
	vRAM = std::make_shared <std::vector<uint8_t >>();

	filename = fileName;

	std::ifstream inFile((fileName + ".gb").c_str(), std::ios::in | std::ios::binary | std::ios::ate);
	if (inFile.is_open())
	{
		std::streampos size;
		size = inFile.tellg();
		vPGRMemory->resize(size);
		inFile.seekg(0, std::ios::beg);
		inFile.read((char*)vPGRMemory->data(), size);

		//Get RAM size and cartridge type
		RAMsize = vPGRMemory->at(0x0149);
		cartridgeType = vPGRMemory->at(0x0147);
		
		switch (RAMsize)
		{
		case 0x00: //No RAM
			RAMsize = 0;
			break;
		case 0x2:
			RAMsize = 0x2000;
			break;
		case 0x3:
			RAMsize = 0x8000;
			break;
		case 0x4:
			RAMsize = 0x20000;
			break;
		case 0x5:
			RAMsize = 0x10000;
			break;
		}

		std::cout << "Cartridge type:" << (std::hex) << (int)cartridgeType << std::endl;
		//std::cout << "SIZE: " << size << std::endl;
		std::cout << "RAM SIZE: " << RAMsize << std::endl;

		vRAM->resize(RAMsize); 

	}
	else
		std::cout << "Error: could not open file: " << fileName.c_str() << std::endl;

	inFile.close();

	//MBCs
	switch (cartridgeType)
	{
	case 0x00: //No MBC
		pMBC = std::make_unique<MBC_0>(vPGRMemory, vRAM);
		break;
	case 0x01: //MBC1
		pMBC = std::make_unique<MBC_1>(vPGRMemory, vRAM);
		pMBC->setRamBanks(RAMsize >> 13); //Divide by 8KB to get number of RAM banks
		pMBC->setMemoryBanks(0x2 << cartridgeType); //Memory banks are powers of 2
		break;
	case 0x03: //MBC1 + RAM + battery
		pMBC = std::make_unique<MBC_1>(vPGRMemory, vRAM);
		pMBC->setRamBanks(RAMsize >> 13); //Divide by 8KB to get number of RAM banks
		pMBC->setMemoryBanks(0x2 << cartridgeType); //Memory banks are powers of 2
		pMBC->SetBatteryStatus(true);
		pMBC->LoadCartRAM(filename + ".sav");
		break;
	}

	//pMBC = std::make_unique<MBC_0>(vPGRMemory, vRAM); //Always no MBC for testing 
}

	Cartridge::~Cartridge()
	{
	}

	uint8_t Cartridge::cpuRead(uint16_t address)
	{
		return pMBC->MBCRead(address);
		
	}

	void Cartridge::cpuWrite(uint16_t address, uint8_t data)
	{
		pMBC->MBCWrite(address, data);
	}

	void Cartridge::SaveRAM()
	{
		pMBC->WriteCartRAM();
	}



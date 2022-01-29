#include "Cartridge.h"

//Load program memory and set the PGRMemory and RAM appropiately 
Cartridge::Cartridge(std::string& fileName)
{
	vPGRMemory = std::make_shared <std::vector<uint8_t >>();
	vRAM = std::make_shared <std::vector<uint8_t >>();

	std::streampos size;
	std::ifstream inFile(fileName.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
	if (inFile.is_open())
	{
		inFile.tellg();
		vPGRMemory->resize(size);
		inFile.seekg(0, std::ios::beg);
		inFile.read((char*)vPGRMemory->data(), size);

		//Get RAM size and cartridge type
		RAMsize = vPGRMemory->at(0x0149);
		cartridgeType = vPGRMemory->at(0x0147);

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
	}


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



#pragma once
#include "Cartridge.h"
#include <string>
#include <vector>
#include <map>
#include "Bus.h"

class Disassembler
{
public:
	Disassembler(int nVectorSize = 10); //Constructor
	~Disassembler(); //Deconstructor

private:
	//Cartridge (ROM)
	std::shared_ptr<Cartridge> cart;
	Bus* Busptr = nullptr;

public:
	void LoadCart(std::shared_ptr<Cartridge> cartridge);
	void ConnectBus(Bus* bus)
	{
		Busptr = bus;
	}

private:
	//Instruction maps
	std::map<uint8_t, std::string> instrMap;
	std::map<uint8_t, int> instrMapBytes;
	int bytes = 0;
	int wait = 0;

private:
	//Uint8_t to hex
	std::string hex(uint32_t n, uint8_t d)
	{
		std::string s(d, '0');
		for (int i = d - 1; i >= 0; i--, n >>= 4)
			s[i] = "0123456789ABCDEF"[n & 0xF];
		return s;
	};

public:
	//Vectors of strings
	std::vector<std::string> vInstrs;
	std::vector<std::string> vPC;
	std::vector<std::string> vData;

public:
	//Public functions
	void InitializeMaps();
	void Disassemble(uint16_t pc);
	void ResizeVectors(int nSize);


};


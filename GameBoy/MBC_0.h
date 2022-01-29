#pragma once
#include "MBC.h"
class MBC_0 :
    public MBC
{
public:
    MBC_0(const std::shared_ptr<std::vector<uint8_t>> pPGRMemory, const std::shared_ptr<std::vector<uint8_t>> pRAM);
    ~MBC_0();

public:
    //Mappe address to correct bank
    virtual uint8_t MBCRead(uint16_t address);
    virtual void MBCWrite(uint16_t address, uint8_t data);
};


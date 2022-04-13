#pragma once
#include "MBC.h"
#include <fstream>

class MBC_1 :
    public MBC
{
public:
    MBC_1(std::shared_ptr<std::vector<uint8_t>> pPGRMemory, std::shared_ptr<std::vector<uint8_t>> pRAM);
    ~MBC_1();

    void SetBatteryStatus(bool b)
    {
        bBatteryEnabled = b;
    }

    void LoadCartRAM(std::string filename);

public:
    //Mappe address to correct bank
    virtual uint8_t MBCRead(uint16_t address);
    virtual void MBCWrite(uint16_t address, uint8_t data);

private:
    //Registers
    uint8_t RAMEnable = 0x00; //Disabled by default
    uint8_t ROMBankNumber = 0x01;
    uint8_t RAMBankNumber = 0x00;
    uint8_t BankingModeSelect = 0x00;

private:
    std::string fname = "";
    //Write RAM
    void WriteCartRAM();
};


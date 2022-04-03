#pragma once

#include <cstdint>
#include <array>

class Bus;

class Timer
{
public:
	Timer();
	~Timer();

private:
	Bus* bus = nullptr;

public:
	//connect to bus
	void ConnectToBus(Bus* n) { bus = n; }

public:
	//Timer and Divider registers
	uint8_t DIV = 0x00; // 0xFF04 Divider register (R/W);
	uint8_t TIMA = 0x00; // FF05 - TIMA - Timer counter (R/W)
	uint8_t TMA = 0x00; // FF06 - TMA - Timer Modulo (R/W)
	
	// FF07 - TAC - Timer Control (R/W)
	union
	{
		struct
		{
			uint8_t InputClockSelect : 2;
			uint8_t TimerEnable : 1;
		};
		uint8_t reg;
	}
	TAC;

	//Clock Select
	int ClockSelect[4] = { 1024, 16, 64, 256 }; //initialize Clock select array

public:
	//Clock Timer
	void ClockDIV();
	void ClockTimer();

public:
	void cpuWrite(uint16_t address, uint8_t data);
	uint8_t cpuRead(uint16_t address);

};


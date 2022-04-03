#include "Timer.h"
#include "Bus.h"


Timer::Timer()
{
	TAC.reg = 0x00;
}

Timer::~Timer()
{
}

void Timer::ClockDIV()
{
	DIV++;
}

void Timer::ClockTimer()
{
	TIMA++;
	if (TIMA == 0)
	{
		TIMA = TMA;
		bus->cpu.irqTimer(); //Request timer if overflow
	}
}


void Timer::cpuWrite(uint16_t address, uint8_t data)
{
	if (address == 0xFF04)
		DIV = 0x00;

	if (address == 0xFF05)
		TIMA = data;

	if (address == 0xFF06)
		TMA = data;

	if (address == 0xFF07)
		TAC.reg = data;
}

uint8_t Timer::cpuRead(uint16_t address)
{
	uint8_t data = 0x00;

	if (address == 0xFF04)
		data = DIV;

	if (address == 0xFF05)
		data = TIMA;

	if (address == 0xFF06)
		data = TMA;

	if (address == 0xFF07)
		data = TAC.reg;

	return data;
}

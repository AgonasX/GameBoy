#include "SM83_APU.h"
#include "Bus.h"

SM83_APU::SM83_APU()
{
}

SM83_APU::~SM83_APU()
{
}

void SM83_APU::cpuWrite(uint16_t address, uint8_t data)
{
	switch (address)
	{
	case 0xFF10: NR10 = data; break;

	case 0xFF11: 
	{
		//std::cout << (int)((data & 0xC0) >> 6) << std::endl;
		switch ((data & 0xC0) >> 6)
		{
		case 0x00: Channel1_pulse.dutycycle = 0.125; break;
		case 0x01: Channel1_pulse.dutycycle = 0.25; break;
		case 0x02: Channel1_pulse.dutycycle = 0.50; break;
		case 0x03: Channel1_pulse.dutycycle = 0.75; break;
		}
		NR11 = data; break;
	}
	case 0xFF12: NR12 = data; break;

	case 0xFF13: NR13 = data; break;

	case 0xFF14: NR14 = data; break;

	case 0xFF16:
	{
		//std::cout << (int)((data & 0xC0) >> 6) << std::endl;
		switch ((data & 0xC0) >> 6)
		{
		case 0x00: Channel2_pulse.dutycycle = 0.125; break;
		case 0x01: Channel2_pulse.dutycycle = 0.25; break;
		case 0x02: Channel2_pulse.dutycycle = 0.50; break;
		case 0x03: Channel2_pulse.dutycycle = 0.75; break;
		}
		NR21 = data; break;
	}
	case 0xFF17: NR22 = data; break;

	case 0xFF18: NR23 = data; break;

	case 0xFF19: NR24 = data; break;
	}
}

uint8_t SM83_APU::cpuRead(uint16_t address)
{
	uint8_t data = 0x00;
	switch (address)
	{
	case 0xFF10: data = NR10; break;
	case 0xFF11: data = NR11; break;
	case 0xFF12: data = NR12; break;
	case 0xFF14: data = NR14 & 0b01000000; break;

	case 0xFF16: data = NR21; break;
	case 0xFF17: data = NR22; break;
	case 0xFF19: data = NR24 & 0b01000000; break;
	}

	return data;
}

void SM83_APU::Clock()
{
	fElapsedTime += 0.25 / 4194304.0;

	uint16_t x1 = ((NR14 & 0x7) << 8) | NR13;
	uint16_t x2 = ((NR24 & 0x7) << 8) | NR23;

	Channel1_pulse.frequency = 131072.0 / (2048.0 - (float)x1);
	Channel2_pulse.frequency = 131072.0 / (2048.0 - (float)x2);
	//Channel1_pulse.frequency = 440;
	
}

float SM83_APU::GetSample(double dGlobalTime)
{
	fChannel2_output = Channel2_pulse.sample(dGlobalTime);
	fChannel1_output = Channel1_pulse.sample(dGlobalTime);
	float sample = (0.5)*(fChannel1_output + fChannel2_output);
	return sample;
}

#pragma once
#include <array>
#include <cstdint>
#include <iostream>
#include <vector>

class Bus; //Forward declaration of bus
class SM83_APU
{
public:
	SM83_APU();
	~SM83_APU();

public:
	//Connect to bus
	void connectBus(Bus* n)
	{
		bus = n;
	}
private:
	Bus* bus = nullptr;

public:
	//Read and Write
	void cpuWrite(uint16_t address, uint8_t data);
	uint8_t cpuRead(uint16_t address);

public:
	//Clock
	void Clock();

private:
	//Registers

	//Channel 1
	uint8_t NR10 = 0x00; //FF10 - NR10 - Channel 1 Sweep register (R/W)
	uint8_t NR11 = 0x00; //FF11 - NR11 - Channel 1 Sound length/Wave pattern duty (R/W)
	uint8_t NR12 = 0x00; //FF12 - NR12 - Channel 1 Volume Envelope (R/W)
	uint8_t NR13 = 0x00; //FF13 - NR13 - Channel 1 Frequency lo (Write Only)
	uint8_t NR14 = 0x00; //FF14 - NR14 - Channel 1 Frequency hi (R/W)

	//Channel 2
	uint8_t NR21 = 0x00; //FF16 - NR21 - Channel 2 Sound Length/Wave Pattern Duty (R/W)
	uint8_t NR22 = 0x00; //FF17 - NR22 - Channel 2 Volume Envelope (R/W)
	uint8_t NR23 = 0x00; //FF18 - NR23 - Channel 2 Frequency lo data (W)
	uint8_t NR24 = 0x00; //FF19 - NR24 - Channel 2 Frequency hi data (R/W)

	//WaveFrom generator, Length Timer, Volume Envelope and Frequency Sweep
	struct WaveForm
	{
		double frequency = 0;
		double dutycycle = 0;
		double amplitude = 1;
		double pi = 3.14159;
		double harmonics = 20;

		double sample(double t)
		{
			double a = 0;
			double b = 0;
			double p = dutycycle * 2.0 * pi;

			//Approxsin for speed
			auto approxsin = [](float t)
			{
				float j = t * 0.15915;
				j = j - (int)j;
				return 20.785 * j * (j - 0.5) * (j - 1.0f);
			};

			for (double n = 1; n < harmonics; n++)
			{
				double c = n * frequency * 2.0 * pi * t;
				a += -approxsin(c) / n;
				b += -approxsin(c - p * n) / n;
			}

			return (2.0 * amplitude / pi) * (a - b);
		}
	};

	WaveForm Channel1_pulse;
	WaveForm Channel2_pulse;

private:
	//Outputs
	float fChannel1_output = 0.0;
	float fChannel2_output = 0.0;

public:
	float GetSample(double dGlobalTime);

private:
	//Frame sequencer and elapsed emulated time
	float fElapsedTime = 0.0;
	uint8_t FrameSequencer = 0;

};


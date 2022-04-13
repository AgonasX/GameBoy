#pragma once
#include <array>
#include <cstdint>
#include <iostream>
#include <vector>
#include <stdlib.h>

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

	//Channel 3
	uint8_t NR30 = 0x00; //FF1A - NR30 - Channel 3 Sound on/off (R/W)
	uint8_t NR31 = 0x00; //FF1B - NR31 - Channel 3 Sound Length (W)
	uint8_t NR32 = 0x00; //FF1C - NR32 - Channel 3 Select output level (R/W)
	uint8_t NR33 = 0x00; //FF1D - NR33 - Channel 3 Frequency’s lower data (W)
	uint8_t NR34 = 0x00; //FF1E - NR34 - Channel 3 Frequency’s higher data (R/W)
	static std::array<uint8_t, 16> WavePatternRAM; //FF30-FF3F - Wave Pattern RAM, Static so waveout can access
	static std::array<float, 32> WavePattern; //Static so waveout can access
	static std::vector<float> SampledWave;

	//Channel 4
	uint8_t NR41 = 0x00; //FF20 - NR41 - Channel 4 Sound Length(W)
	uint8_t NR42 = 0x00; //FF21 - NR42 - Channel 4 Volume Envelope (R/W)
	uint8_t NR43 = 0x00; //FF22 - NR43 - Channel 4 Polynomial Counter (R/W)
	uint8_t NR44 = 0x00; //FF23 - NR44 - Channel 4 Counter/consecutive; Inital (R/W)

	//Sound Control 
	uint8_t NR50 = 0x00; //FF24 - NR50 - Channel control / ON-OFF / Volume (R/W)
	uint8_t NR51 = 0x00; //FF25 - NR51 - Selection of Sound output terminal (R/W)
	uint8_t NR52 = 0x00; //FF26 - NR52 - Sound on/off



	 
	//WaveFrom generator, Length Timer, Volume Envelope and Frequency Sweep

	static bool Channel1_Enable; //Static so frequency sweep can access
	bool Channel2_Enable = false;

	struct WaveForm
	{
		double frequency = 0;
		double dutycycle = 0;
		double amplitude = 1;
		double pi = 3.14159;
		double harmonics = 20;

		uint8_t wave_form = 0x00;
		uint16_t frequency_timer = 0x00000;
		uint8_t output = 0x00;
		uint16_t x = 0x00;

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

		uint8_t clock()
		{
			if (frequency_timer > 0)
				frequency_timer--;

			if (frequency_timer == 0)
			{
				frequency_timer = 2048 - x;
				output = (wave_form & 0x80) >> 7;
				//Rotate WaveForm left
				wave_form = ((wave_form & 0x7F) << 1) | ((wave_form & 0x80) >> 7);
			}

			return output;

		}
	};

	struct VolumeEnvelope
	{
		uint8_t period = 0x00;
		uint8_t period_timer = 0x00;
		uint8_t current_volume = 0x00;
		bool direction = 0; //0 downwards, 1 upwwards

		void AdjustVolume()
		{
			if (period != 0)
			{
				if (period_timer > 0)
					period_timer--;

				if(period_timer == 0)
				{
					period_timer = period; //Reload period timer
					//Adjust volume
					if ((current_volume > 0x00 && !direction) || (current_volume < 0x0F && direction))
					{
						if (direction)
							current_volume++;
						else if (!direction)
							current_volume--;
					}
				}
			}
		}
	};

	struct FrequencySweep
	{
		uint8_t sweep_timer = 0x00;
		uint8_t sweep_period = 0x00;
		uint8_t sweep_shifts = 0x00;
		uint16_t frequency = 0x00;
		uint16_t old_frequency = 0x00;
		uint16_t new_frequency = 0x00;
		bool sweep_enable = false;
		bool is_incrementing = false;

		uint16_t calculate_frequency()
		{
			uint16_t new_frequency_ = 0x00;

			if (is_incrementing)
			{
				new_frequency_ = old_frequency + (old_frequency >> sweep_shifts);
			}
			else
			{
				new_frequency_ = old_frequency - (old_frequency >> sweep_shifts);
			}

			if (new_frequency_ > 2047)
			{
				Channel1_Enable = false;
			}

			return new_frequency_;
		}

		bool sweep()
		{
			bool bSweep = false;

			if (sweep_timer > 0)
				sweep_timer--;

			if (sweep_timer == 0)
			{
				//Reload sweep timer
				if (sweep_period > 0)
					sweep_timer = sweep_period; 
				else
					sweep_timer = 8;

				//Calculate new frequency
				if (sweep_enable && sweep_period > 0)
				{
					new_frequency = calculate_frequency();

					if (new_frequency <= 2047 && sweep_shifts > 0)
					{
						frequency = new_frequency;
						old_frequency = new_frequency;
						bSweep = true;

						//Overflow check
						calculate_frequency();
					}

				}
					
			}

			return bSweep;
		}
	};
	
	struct WaveOut
	{
		bool wave_on = false;
		bool wave_enable = false;
		uint16_t length_timer = 0x00;
		float output_shift = 0.0;
		uint16_t frequency_timer = 0x00;
		uint16_t frequency_reload = 0x00;
		uint16_t frequency = 0x00;
		float output = 0.0;
		uint8_t sample_counter = 0x00;
		int sample_index = 0;

		uint8_t ReadHighNybble(uint8_t byte)
		{
			return ((byte & 0xF0) >> 4);
		}

		uint8_t ReadLowNybble(uint8_t byte)
		{
			return (byte & 0x0F);
		}

		float clock()
		{
			
			if (frequency_timer > 0)
				frequency_timer--;

			if (frequency_timer == 0)
			{
				frequency_timer = frequency_reload; //Reload timer

				output = WavePattern.at(sample_index);
				sample_index = (sample_index + 1) % 32;
			}
			return output;
			

			/*
			output = 0.0;
			if (SampledWave.size() > 0)
			{
				output = SampledWave.at(sample_index);
				sample_index = (sample_index + 1) % SampledWave.size();
			}*/
		}

		void GetWavePattern()
		{
			int PatternIndex = 0;
			for (int index = 0; index < 16; index++)
			{
				uint8_t byte = WavePatternRAM.at(index);

				//Get high nybble first
				WavePattern.at(PatternIndex) = (float)ReadHighNybble(byte);
				PatternIndex++;

				//... then low nybble
				WavePattern.at(PatternIndex) = (float)ReadLowNybble(byte);
				PatternIndex++;
			}
		}

		//Sample the wave pattern at apu clock rate (4194304/4)Hz
		void SampleWave()
		{
			//Clear vector first 
			SampledWave.clear();

			uint16_t timer = frequency;
			int pattern_index = 0;
			int ticks = 0;

			//Sample at 4194304 Hz, the while loop runs at 4194304 Hz
			while (SampledWave.size() != (frequency * 16))
			{

				//4194304Hz
				
				if((ticks % 4) == 0) SampledWave.push_back(WavePattern.at(pattern_index));
				//SampledWave.push_back(WavePattern.at(pattern_index));
				
				ticks++;

				if (timer > 0)
					timer--;

				if (timer == 0)
				{
					timer = frequency; //Reload timer
					pattern_index = (pattern_index + 1) % 32 ;
				}

			}
		}
			
	};

	struct Noise
	{
		uint8_t XOR_result = 0x00;
		uint8_t Length_timer = 0x00;
		uint32_t frequency_timer = 0x0000;
		uint8_t shift_amount = 0x00;
		bool counter_width = false;
		uint32_t divisor_code = 0x00;
		uint16_t LFSR = 0x00; 
		float output = 0.0;
		bool enable = false;

		void clock()
		{
			if (frequency_timer > 0)
				frequency_timer--;

			if (frequency_timer == 0)
			{
				frequency_timer = (divisor_code > 0 ? (divisor_code << 4) : 8) << shift_amount;

				XOR_result = (LFSR & 0x0001) ^ ((LFSR & 0x0002) >> 1);
				LFSR = (LFSR >> 1) | (XOR_result << 14);

				if (counter_width)
				{
					LFSR &= ~(1 << 6);
					LFSR |= XOR_result << 6;
				}

				output = (float)((~LFSR) & 0x0001);
			}
		}
	};

	uint8_t Channel1_Length_timer = 0x00;
	uint8_t Channel2_Length_timer = 0x00;

	WaveForm Channel1_pulse;
	WaveForm Channel2_pulse;
	WaveOut Channel3_waveout;
	Noise Channel4_noise;

	VolumeEnvelope Channel1_env;
	VolumeEnvelope Channel2_env;
	VolumeEnvelope Channel4_env;

	FrequencySweep Channel1_sweep;

	//Frequencies
	uint16_t x1 = 0x00;
	uint16_t x2 = 0x00;

private:
	//Outputs
	float fChannel1_output = 0.0;
	float fChannel2_output = 0.0;
	float fChannel3_output = 0.0;
	float fChannel4_output = 0.0;

	float SO1 = 0.0;
	float SO2 = 0.0;

	float LeftRightChannel[2] = { 0.0,0.0 };

public:
	float GetSample(double dGlobalTime);

private:
	//Frame sequencer and elapsed emulated time
	float fElapsedTime = 0.0;
	uint16_t FrameSequencer = 0x00;
	uint16_t FrameCounter = 0x00;

private:
	//Moving Average filter with 5 points symmetric
	float MovingAverageFilter(std::vector<float>& input)
	{
		std::vector<float> output;
		int size = input.size();
		output.resize(size);

		float sum = 0.0;

		for (int i = 0; i < size; i++)
		{
			sum = 0.0;

			sum += input.at((i + (size - 2)) % size);
			sum += input.at((i + (size - 1)) % size);
			sum += input.at(i);
			sum += input.at((i + 1) % size);
			sum += input.at((i + 2) % size);
			
			output.at(i) = sum / 5.0;
		}

		//Copy output to input
		std::copy(output.begin(), output.end(), input.begin());
	}
};
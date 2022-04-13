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

	//Channel 1
	case 0xFF10: NR10 = data; break;

	case 0xFF11: 
	{
		//std::cout << (int)((data & 0xC0) >> 6) << std::endl;
		switch ((data & 0xC0) >> 6)
		{
		case 0x00: Channel1_pulse.dutycycle = 0.125; Channel1_pulse.wave_form = 0b00000001; break;
		case 0x01: Channel1_pulse.dutycycle = 0.25; Channel1_pulse.wave_form = 0b00000011; break;
		case 0x02: Channel1_pulse.dutycycle = 0.50; Channel1_pulse.wave_form = 0b11110000; break;
		case 0x03: Channel1_pulse.dutycycle = 0.75; Channel1_pulse.wave_form = 0b11111100; break;
		}

		NR11 = data;
		break;
	}
	case 0xFF12: NR12 = data; break;

	case 0xFF13: NR13 = data; break;

	case 0xFF14: 
		NR14 = data;
		Channel1_pulse.x = ((NR14 & 0x7) << 8) | NR13;
		//Channel 1 start
		if ((NR14 & 0x80) > 0)
		{
			//Length Timer
			if(Channel1_Length_timer <= 0x00) Channel1_Length_timer = 0x40 - (NR11 & 0x3F);

			Channel1_Enable = true;
			Channel1_pulse.frequency_timer = 2048 - (((NR14 & 0x7) << 8) | NR13);

			//Volume Envelope
			Channel1_env.period = NR12 & 0x07;
			Channel1_env.period_timer = Channel1_env.period;
			Channel1_env.current_volume = (NR12 & 0xF0) >> 4;
			Channel1_env.direction = (NR12 & 0x08) > 0 ? true : false;

			//Frequency sweep
			x1 = ((NR14 & 0x7) << 8) | NR13;
			Channel1_pulse.frequency = 131072.0 / (2048.0 - (float)x1);
			Channel1_sweep.old_frequency = x1;
			//Channel1_sweep.new_frequency = x1;
			Channel1_sweep.sweep_period = (NR10 & 0xE0) >> 5;
			Channel1_sweep.sweep_shifts = NR10 & 0x07;
			Channel1_sweep.sweep_timer = Channel1_sweep.sweep_period;
			Channel1_sweep.sweep_enable = ((NR10 & 0xE0) > 0 || Channel1_sweep.sweep_shifts > 0) ? true : false; 
			Channel1_sweep.is_incrementing = (NR10 & 0x08) > 0 ? false : true;

			
		}
		break;

	//Channel 2

	case 0xFF16:
	{
		//std::cout << (int)((data & 0xC0) >> 6) << std::endl;
		switch ((data & 0xC0) >> 6)
		{
		case 0x00: Channel2_pulse.dutycycle = 0.125; Channel2_pulse.wave_form = 0b00000001; break;
		case 0x01: Channel2_pulse.dutycycle = 0.25; Channel2_pulse.wave_form = 0b00000011; break;
		case 0x02: Channel2_pulse.dutycycle = 0.50; Channel2_pulse.wave_form = 0b11110000; break;
		case 0x03: Channel2_pulse.dutycycle = 0.75; Channel2_pulse.wave_form = 0b11111100; break;
		}
		NR21 = data; break;
	}
	case 0xFF17: NR22 = data; break;

	case 0xFF18: NR23 = data; break;

	case 0xFF19:
		NR24 = data;
		Channel2_pulse.x = ((NR24 & 0x7) << 8) | NR23;
		//Channel 2 start
		if ((NR24 & 0x80) > 0)
		{
			//Length Timer
			if (Channel2_Length_timer <= 0x00) Channel2_Length_timer = 0x40 - (NR21 & 0x3F);
			Channel2_Enable = true;

			//Volume Envelope
			Channel2_env.period = NR22 & 0x07;
			Channel2_env.period_timer = Channel2_env.period;
			Channel2_env.current_volume = (NR22 & 0xF0) >> 4;
			Channel2_env.direction = (NR22 & 0x08) > 0 ? true : false;

			//Frequency
			//x2 = ((NR24 & 0x7) << 8) | NR23;
			//Channel2_pulse.frequency = 131072.0 / (2048.0 - (float)x2);
			Channel2_pulse.frequency_timer = 2048 - (((NR24 & 0x7) << 8) | NR23);
		}
		break;

	//Channel 3

	case 0xFF1A:
		NR30 = data;
		Channel3_waveout.wave_on = (NR30 & 0x80) > 0 ? true : false;
		break;

	case 0xFF1B:
		NR31 = data;
		break;

	case 0xFF1C:
		NR32 = data;
		switch ((NR32 & 0x60) >> 5)
		{
		case 0x00: Channel3_waveout.output_shift = 0.0; break;
		case 0x01: Channel3_waveout.output_shift = 1.0; break;
		case 0x02: Channel3_waveout.output_shift = 0.5; break;
		case 0x03: Channel3_waveout.output_shift = 0.25; break;
		}
		break;

	case 0xFF1D:
		NR33 = data;
		break;

	case 0xFF1E:
		NR34 = data;
		//Channel3_waveout.frequency_reload = (2048 - (((NR34 & 0x7) << 8) | NR33))* 2;
		Channel3_waveout.frequency = (2048 - (((NR34 & 0x7) << 8) | NR33)) * 2;
		//Get wave pattern
		Channel3_waveout.GetWavePattern();

		//Sample Wave
		Channel3_waveout.SampleWave();


		//Do 1 filter passes
		for (int i = 0; i < 1; i++)
		{
			MovingAverageFilter(SampledWave);
		}

		if ((NR34 & 0x80) > 0)
		{
			//Channel 3 start
			//Channel3_waveout.frequency_reload = 2048 - (((NR34 & 0x7) << 8) | NR33);
			//Channel3_waveout.frequency_timer = 0x00;
			Channel3_waveout.frequency = 2048 - (((NR34 & 0x7) << 8) | NR33);

			//Channel3_waveout.frequency = 2048 - 1024;

			

			Channel3_waveout.sample_index = 0;
			if(Channel3_waveout.length_timer <= 0) Channel3_waveout.length_timer = 256 - NR31;
			Channel3_waveout.wave_enable = true;

			//Get wave pattern
			//Channel3_waveout.GetWavePattern();

			/*
			//Sample Wave
			Channel3_waveout.SampleWave();

			
			//Do 1 filter passes
			for (int i = 0; i < 1; i++)
			{
				MovingAverageFilter(SampledWave);
			}
			*/
		}
		break;

		//Channel 4

	case 0xFF20:
		NR41 = data; 
		break;

	case 0xFF21:
		NR42 = data;
		break;

	case 0xFF22:
		//Polynomial Counter
		NR43 = data;
		Channel4_noise.shift_amount = (NR43 & 0xF0) >> 4;
		Channel4_noise.counter_width = (NR43 & 0x08) > 0 ? true : false;
		Channel4_noise.divisor_code = (NR43 & 0x7);
		break;

	case 0xFF23:
		NR44 = data;
		//Channel 4 start
		if ((NR44 & 0x80) > 0)
		{
			Channel4_noise.enable = true;
			if(Channel4_noise.Length_timer <= 0) Channel4_noise.Length_timer = 64 - (NR41 & 0x3F);
			Channel4_noise.LFSR = 0xFF;

			//Volume Envelope
			Channel4_env.period = NR42 & 0x07;
			Channel4_env.period_timer = Channel2_env.period;
			Channel4_env.current_volume = (NR42 & 0xF0) >> 4;
			Channel4_env.direction = (NR42 & 0x08) > 0 ? true : false;
		}
		break;

	case 0xFF24:
		NR50 = data;
		break;

	case 0xFF25:
		NR51 = data;
		break;

	case 0xFF26:
		NR52 = data & 0x80;
		break;



	}

	//Wave pattern ram
	if (0xFF30 <= address && address <= 0xFF3F)
		WavePatternRAM.at(address - 0xFF30) = data;
	
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

	case 0xFF1A: data = NR30; break;

	case 0xFF1C: data = NR32; break;

	case 0xFF1E: data = NR34 & 0b01000000; break;

	case 0xFF21: data = NR42; break;

	case 0xFF22: data = NR43; break;

	case 0xFF23: data = NR44 & 0x7F; break;

	case 0xFF24: data = NR50; break;

	case 0xFF25: data = NR51; break;

	case 0xFF26: data = NR52; break;

	}

	//Wave pattern ram
	if (0xFF30 <= address && address <= 0xFF3F)
		data = WavePatternRAM.at(address - 0xFF30);

	return data;
}

void SM83_APU::Clock()
{
	//fElapsedTime += 4194304.0;

	//Channel 4
	Channel4_noise.clock();

	if ((FrameCounter % 2048) == 0)
	{
		//At FS step 0,2,4,6 clock the length timer
		if ((FrameSequencer % 2) == 0)
		{
			//Channel 1
			if ((NR14 & 0x40) > 0 && Channel1_Length_timer > 0) 
				if (Channel1_Length_timer-- == 0)
				{
					Channel1_Enable = false;
				}
			//Channel 2
			if ((NR24 & 0x40) > 0 && Channel2_Length_timer > 0) 
				if (Channel2_Length_timer-- == 0) Channel2_Enable = false;
			//Channel 3
			if ((NR34 & 0x40) > 0 && Channel3_waveout.length_timer > 0) 
				if (Channel3_waveout.length_timer-- == 0) Channel3_waveout.wave_enable = false;
			//Channel 4
			if ((NR44 & 0x40) > 0 && Channel4_noise.Length_timer > 0) 
				if (Channel4_noise.Length_timer-- == 0) Channel4_noise.Length_timer = false;
		}

		//At FS step 7 clock the volume envelope
		if ((FrameSequencer % 8) == 7)
		{
			//Channel 1
			Channel1_env.AdjustVolume();
			//Channel1_pulse.amplitude = (float)Channel1_env.current_volume / 15.0;
			//Channel1_pulse.amplitude = 0.5;


			//Channel 2
			Channel2_env.AdjustVolume();
			//Channel2_pulse.amplitude = (float)Channel2_env.current_volume / 15.0;
			//Channel2_pulse.amplitude = 0.5;

			//Channel 4
			Channel4_env.AdjustVolume();
		}

		
		//At FS 2 and 6 clock the frequency sweeper
		if ((FrameSequencer % 8) == 2 || (FrameSequencer % 8) == 6)
		{
			//Frequency sweep channel 1 only
			if (Channel1_sweep.sweep())
			{
				//Write frequency back to NR14 and NR13
				NR13 = Channel1_sweep.frequency & 0x00FF;
				NR14 = (NR14 & 0xF8) | ((Channel1_sweep.frequency & 0x0700) >> 8);
				Channel1_pulse.x = ((NR14 & 0x7) << 8) | NR13;

				//Channel1_pulse.frequency = 131072.0 / (2048.0 - (float)x1);
			}
		}
		FrameSequencer++; //FrameSequencer runs at 512Hz
	}

	/*
	//Channel 3 waveout frequency is 131072/(16*(2048-x)) Hz which mean we clock the waveout every 16 apu clocks

	if (Channel3_waveout.wave_on == true)
	{
		Channel3_waveout.clock();	
	}
	*/

	//Clock frequency timer
	Channel1_pulse.clock();
	Channel2_pulse.clock();
	//Channel3_waveout.clock();

	FrameCounter++;
}

float SM83_APU::GetSample(double dGlobalTime)
{
	fChannel1_output = 0.0;
	fChannel2_output = 0.0;
	fChannel3_output = 0.0;
	fChannel4_output = 0.0;

	SO1 = 0;
	SO2 = 0;

	NR52 = NR52 & 0x80;

	if ((NR52 & 0x80) > 0)
	{
		//----------------------Old code-----------------------------------------//
		//Set frequency for channel 1 and 2
		//x1 = ((NR14 & 0x7) << 8) | NR13;
		//Channel1_pulse.frequency = 131072.0 / (2048.0 - (float)x1);
		//x2 = ((NR24 & 0x7) << 8) | NR23;
		//Channel2_pulse.frequency = 131072.0 / (2048.0 - (float)x2);
		//if (Channel1_Enable) fChannel1_output = Channel1_pulse.sample(dGlobalTime) * ((float)Channel1_env.current_volume / 15.0);
		//if (Channel2_Enable) fChannel2_output = Channel2_pulse.sample(dGlobalTime);// * ((float)Channel2_env.current_volume / 15.0);

		if (Channel1_Enable)
		{
			fChannel1_output = Channel1_pulse.output * ((float)Channel1_env.current_volume / 15.0);
			NR52 = NR52 | 0x01;
		}
	
		if (Channel2_Enable)
		{
			fChannel2_output = Channel2_pulse.output * ((float)Channel2_env.current_volume / 15.0);
			NR52 = NR52 | (0x01 << 1);
		}
	
		if (Channel3_waveout.wave_enable)
		{
			//Nearest neighbor 
			int index = ((int)round(dGlobalTime * (4194304.0) / 4)) % SampledWave.size();
			fChannel3_output = (SampledWave.at(index) * Channel3_waveout.output_shift) / 15.0;
			//fChannel3_output = Channel3_waveout.output / 15.0;
			NR52 = NR52 | (0x01 << 2);
		}
		
	
		if (Channel4_noise.enable)
		{
			fChannel4_output = Channel4_noise.output * (float)Channel4_env.current_volume / 15.0;
			NR52 = NR52 | (0x01 << 3);
		}
		
		
	}

	//Sound mixing
	if ((NR51 & 0x01) > 0) SO1 += fChannel1_output;
	if ((NR51 & 0x02) > 0) SO1 += fChannel2_output;
	if ((NR51 & 0x04) > 0) SO1 += fChannel3_output;
	if ((NR51 & 0x08) > 0) SO1 += fChannel4_output;
	if ((NR51 & 0x10) > 0) SO2 += fChannel1_output;
	if ((NR51 & 0x20) > 0) SO2 += fChannel2_output;
	if ((NR51 & 0x40) > 0) SO2 += fChannel3_output;
	if ((NR51 & 0x80) > 0) SO2 += fChannel4_output;

	//Channel control
	SO1 *= (float)(((NR50 & 0x70) >> 4)/7.0);
	SO2 *= (float)((NR50 & 0x7)/7.0);

	LeftRightChannel[0] = SO1;
	LeftRightChannel[1] = SO2;
	
	float sample = (0.1) * 0.5 * (LeftRightChannel[0] + LeftRightChannel[1]); //Mono sound for now
	return sample;
}

//Initialize static variable here
std::array<uint8_t,16> SM83_APU::WavePatternRAM = { 0x00 };
std::array<float, 32> SM83_APU::WavePattern = { 0.0 };
std::vector<float> SM83_APU::SampledWave;
bool SM83_APU::Channel1_Enable = false;


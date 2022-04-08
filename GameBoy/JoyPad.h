#pragma once
#include <array>
#include <cstdint>
#include <iostream>

class Bus;

class JoyPad
{
public:
	JoyPad();
	~JoyPad();

private:
	Bus* bus = nullptr;

public:
	void connectBus(Bus* n) { bus = n; }

private:
	//JoyPad register
	union
	{
		struct
		{
			uint8_t P10 : 1;
			uint8_t P11 : 1;
			uint8_t P12 : 1;
			uint8_t P13 : 1;
			uint8_t P14 : 1;
			uint8_t P15 : 1;
			uint8_t P16 : 1;
			uint8_t P17 : 1;
		};
		uint8_t reg;
	}
	JOYP;

public:
	//Button presses
	void PressDown();
	void PressUp();
	void PressLeft();
	void PressRight();
	void PressStart();
	void PressSelect();
	void PressB();
	void PressA();

	//Buttons releases
	void ReleaseDown();
	void ReleaseUp();
	void ReleaseLeft();
	void ReleaseRight();
	void ReleaseStart();
	void ReleaseSelect();
	void ReleaseB();
	void ReleaseA();

	//Update Pins
	void UpdatePins();

private:
	//Booleans for buttons
	bool bButtonDown = false;
	bool bButtonUp = false;
	bool bButtonLeft = false;
	bool bButtonRight = false;
	bool bButtonStart = false;
	bool bButtonSelect = false;
	bool bButtonB = false;
	bool bButtonA = false;

public:
	//Read and Write
	void cpuWrite(uint8_t data);
	uint8_t cpuRead();


};


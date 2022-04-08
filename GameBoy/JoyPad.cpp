#include "JoyPad.h"
#include "Bus.h"

JoyPad::JoyPad()
{
	JOYP.reg = 0xFF;
}

JoyPad::~JoyPad()
{
}

void JoyPad::PressDown()
{
	bButtonDown = true;
}

void JoyPad::PressUp()
{
	bButtonUp = true;
}

void JoyPad::PressLeft()
{
	bButtonLeft = true;
}

void JoyPad::PressRight()
{
	bButtonRight = true;
}

void JoyPad::PressStart()
{
	bButtonStart = true;
}

void JoyPad::PressSelect()
{
	bButtonSelect = true;
}

void JoyPad::PressB()
{
	bButtonB = true;
}

void JoyPad::PressA()
{
	bButtonA = true;
}

void JoyPad::ReleaseDown()
{
	bButtonDown = false;
}

void JoyPad::ReleaseUp()
{
	bButtonUp = false;
}

void JoyPad::ReleaseLeft()
{
	bButtonLeft = false;
}

void JoyPad::ReleaseRight()
{
	bButtonRight = false;
}

void JoyPad::ReleaseStart()
{
	bButtonStart = false;
}

void JoyPad::ReleaseSelect()
{
	bButtonSelect = false;
}

void JoyPad::ReleaseB()
{
	bButtonB = false;
}

void JoyPad::ReleaseA()
{
	bButtonA = false;
}

void JoyPad::UpdatePins()
{
	JOYP.P13 = 1;
	JOYP.P12 = 1;
	JOYP.P11 = 1;
	JOYP.P10 = 1;

	//Action buttons
	if (JOYP.P15 == 0)
	{
		JOYP.P13 = bButtonStart ? 0 : 1;
		JOYP.P12 = bButtonSelect ? 0 : 1;
		JOYP.P11 = bButtonB ? 0 : 1;
		JOYP.P10 = bButtonA ? 0 : 1;
	}

	//Directional buttons
	if (JOYP.P14 == 0)
	{
		JOYP.P13 = bButtonDown ? 0 : 1;
		JOYP.P12 = bButtonUp ? 0 : 1;
		JOYP.P11 = bButtonLeft ? 0 : 1;
		JOYP.P10 = bButtonRight ? 0 : 1;
	}

	if (((~JOYP.reg) & 0x0F) > 0) bus->cpu.irqJoypad();
}


void JoyPad::cpuWrite(uint8_t data)
{
	JOYP.P15 = ((data & (0x1 << 5)) > 0) ? 1 : 0;
	JOYP.P14 = ((data & (0x1 << 4)) > 0) ? 1 : 0;
}

uint8_t JoyPad::cpuRead()
{
	return JOYP.reg;
}

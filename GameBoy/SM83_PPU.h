#pragma once
#include <array>
#include <cstdint>
#include <iostream>
#include <vector>

class Bus; //Forward declaration of bus

class SM83_PPU
{
public:
	SM83_PPU();
	~SM83_PPU();


public:
	//Connect to bus
	void connectBus(Bus* n)
	{
		bus = n;
	}
private:
	Bus* bus = nullptr;


public:
	//PPU read and write.
	void ppuWrite(uint16_t address, uint8_t data);
	uint8_t ppuRead(uint16_t address);

public:
	//CPU read and write
	void cpuWrite(uint16_t address, uint8_t data);
	uint8_t cpuRead(uint16_t address);


public:
	//Clock cpu
	void clock();
	int scanLine = 0;
	uint8_t X = 0; //X fetcher coordinate
	uint8_t x = 0; //x position of pixels
	uint8_t XX = 0; //X-coordinate
	uint8_t YY = 0; //Y-coordinate
	uint8_t WindowY = 0; //Window internal Y coordinate
	uint8_t WindowX = 0; //Window internal X coordinate

	int dots = 0;
	int cycles = 0; //cycles to keep track of how many cycles to fetch data to pixel FIFO
	bool bFrameComplete = false;
	bool frameComplete();

private:
	//Utility function for clock:
	void Mode0();
	void Mode1();
	void Mode2();
	void Mode3();

public:
	//Pixel FIFO and screen
	std::array<uint32_t, 23040> LCDscreen;
	uint16_t OBJPixelFIFO = 0x0000;
	uint8_t PixelFIFO = 0x0000; //Information about whether it is background (0) or object (1) pixels 
	uint32_t BGPixelFIFO = 0x00000000;
	uint16_t BGPixelFIFOLatch = 0x0000; //Latch for when BG pixel FIFO needs it

private:
	//FIFO pixel fetcher
	int pixels = 0; //Number of pixels in the pixel FIFO 
	uint16_t getTile(uint8_t X, uint8_t Y, bool bMapArea, bool bFetchObject = false); //Get tile
	uint16_t tileRowAddress = 0x0000;
	void getTileDataLow(uint16_t address);
	uint8_t TileDataLow = 0x00;
	void getTileDataHigh(uint16_t address);
	uint8_t TileDataHigh = 0x00;
	uint16_t getTileMap(uint8_t tileID, uint8_t y, bool bFetchObject = false);
	bool bMapArea = 1;
	int Fetcher = 0; //Fetcher, 0 = get tile, 1 = getTileDataLow, 2 = getTileDataHigh, 3 = push / sleep

	//For getting correct palett
	uint8_t colorIndex = 0x00;
	uint32_t argb = 0x00000000;

	//Window rendering
	bool bInWindow = false;

	//Sprite (OBJ) rendering 
	bool bPauseRender = false;
	bool bFetchObj = false;
	int SpriteIndex = -1;
	std::vector<int> vSpriteIndex;
	uint8_t PaletteNum = 0x00; //Emulator only register for Palette number information
	void AbortSpriteFetch();

	enum OBJFLAGS
	{
		PaletteNumber = (1 << 4), //PaletteNumber
		XFlip = (1 << 5), //X flip
		YFlip = (1 << 6), //Y flip
		OBJPriority = (1 << 7) //BG and Window over OBJ (0=No, 1=BG and Window colors 1-3 over the OBJ)
	};

public:
	//PPU registers

	//LCDC register
	union
	{
		struct
		{
			uint8_t BGAndWindowPriority : 1;
			uint8_t OBJEnable : 1;
			uint8_t OBJSize : 1;
			uint8_t BGTileMapArea : 1;
			uint8_t BGAndWindowTileDataArea : 1;
			uint8_t WindowEnable : 1;
			uint8_t WindowsTileMapArea : 1;
			uint8_t PPUEnable : 1;
		};
		uint8_t reg;
	} LCDC;

	//LCD status register
	union
	{
		struct
		{
			uint8_t modeFlag : 2;
			uint8_t LYFlag : 1;
			uint8_t HBLank : 1;
			uint8_t VBlank : 1;
			uint8_t OAM : 1;
			uint8_t LY : 1;
			uint8_t unused : 1;
		};
		uint8_t reg;
	} STAT;

	//Palett registers

	//Background palett register
	union
	{
		struct
		{
			uint8_t index0 : 2;
			uint8_t index1 : 2;
			uint8_t index2 : 2;
			uint8_t index3 : 2;
		};
		uint8_t reg;
	} BGP;

	//OBJ palett 0
	union
	{
		struct
		{
			uint8_t index0 : 2;
			uint8_t index1 : 2;
			uint8_t index2 : 2;
			uint8_t index3 : 2;
		};
		uint8_t reg;
	} OBP0;

	//OBJ palett 1
	union
	{
		struct
		{
			uint8_t index0 : 2;
			uint8_t index1 : 2;
			uint8_t index2 : 2;
			uint8_t index3 : 2;
		};
		uint8_t reg;
	} OBP1;
	
private:
	//LCD position and Scrolling
	//Access even during mode 3
	uint8_t SCY = 0x00; //FF42 - SCY (Scroll Y) (R/W)
	uint8_t SCX = 0x00; //FF43 - SCX (Scroll X) (R/W)
	uint8_t LY = 0x00; //FF44 - LY (LCD Y Coordinate) (R)
	uint8_t LYC = 0x00; //FF45 - LYC (LY Compare) (R/W)
	uint8_t WY = 0x00; //FF4A - WY (Window Y Position) (R/W) (0-143)
	uint8_t WX = 0x00; //FF4B - WX(Window X Position + 7) (R / W) (0-166)


public:
	//For debugging
	//Tiledata
	std::array<uint32_t, 24576> tileData = { 0x00000000 };
	std::array<uint32_t, 24576>& getTileTable();
	uint8_t RowLowByte = 0x00;
	uint8_t RowHighByte = 0x00;
	uint8_t lowBit = 0x00;
	uint8_t highBit = 0x00;
	

	//TileMaps
	std::array<uint32_t, 65536> tileMap1 = { 0x00000000 };
	std::array<uint32_t, 65536>& getTileMap1Data();
	std::array<uint32_t, 65536> tileMap0 = { 0x00000000 };
	std::array<uint32_t, 65536>& getTileMap0Data();

	/*
	//SpriteTable
	std::array<uint32_t, 5120> SpriteMap;
	std::array<uint32_t, 5120>& getSpriteMap();
	*/

private:
	//PPU RAM
	std::array<uint8_t, 8192> VRAM = { 0x00 }; //VRAM
	std::array<uint8_t, 160> OAM = { 0x00 }; //Object Attribute Memory
	
	//Structure for 10 objects to be drawn	
	struct OAMobject
	{
		uint8_t Ypos;
		uint8_t Xpos;
		uint8_t TileIndex;
		uint8_t Flags;
	};

	std::vector<OAMobject> vOAMObjects; //Vector for holding 10 sprites to be drawn
	

private:
	//other variables and functions to faciliate emulation
	std::array<uint32_t, 4> palettes = { 0x00 };
	bool bStatInterruptBlock = false; //Variable needed to emulate STAT blocking behaviour
	int nPauseDots = 0; //Pause cycles for scrolling
	uint8_t GetOBJFlag(OBJFLAGS f, OAMobject sOAMobject); //Easy way to get OBJ flags
};


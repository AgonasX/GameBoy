#include "SM83_PPU.h"
#include "Bus.h"

SM83_PPU::SM83_PPU()
{
	//Reset registers
	STAT.reg = 0x00;
	LCDC.reg = 0x00;
	LCDC.BGAndWindowTileDataArea = 1;

	//Palett colors (abgr) GameBoy LCD colors (green)
	palettes.at(0) = 0xFF0FBC9B;
	palettes.at(1) = 0xFF0FAC8B;
	palettes.at(2) = 0xFF306230;
	palettes.at(3) = 0xFF0F380F;

	LCDscreen.fill(0xFFFFFFFF);

	STAT.modeFlag = 2; //OAM Scan first

}

SM83_PPU::~SM83_PPU()
{
}

void SM83_PPU::ppuWrite(uint16_t address, uint8_t data)
{
	//VRAM
	if (0x8000 <= address && address <= 0x9FFF)
	{
		VRAM.at(address - 0x8000) = data;
		//if(address >= 0x9C00)
		//std::cout << (std::hex) << "0x" << (int)VRAM.at(address - 0x8000) << std::endl;
	}

	//Palett registers
	if (address == 0xFF47) //BG palett data
		BGP.reg = data;

	//LCD 
	if (address == 0xFF40) //LCD Control (LCDC)
		LCDC.reg = data;
	if (address == 0xFF41)//LCD status register (STAT)
	{
		uint8_t tempSTAT = STAT.reg;
		STAT.reg = (data & 0xF8); //Only upper 5 bits are writable
		STAT.reg |= (tempSTAT & 0x7);
	}

	//LCD position and Scrolling 
	if (address == 0xFF42) //SCY(Scroll Y)s
		SCY = data;
	if (address == 0xFF43) //SCX (Scroll X)
		SCX = data;
	if (address == 0xFF45) //LY LYC (LY Compare) (R/W)
		LYC = data;
	if (address == 0xFF4A) //WY (Window Y Position) (R/W)
		WY = data;
	if (address == 0xFF4B) //WX (Window X Position + 7) (R/W)
		WX = data;
}

uint8_t SM83_PPU::ppuRead(uint16_t address)
{
	uint8_t data = 0x00;
	//VRAM
	if (0x8000 <= address && address <= 0x9FFF)
	{
		data = VRAM.at(address - 0x8000);
	}

	//Palett registers
	if (address == 0xFF47) //BG palett data
		data = BGP.reg;

	//LCD 
	if (address == 0xFF40) //LCD Control (LCDC)
		data = LCDC.reg;
	if (address == 0xFF41) //LCD status register (STAT)
		data = STAT.reg ;

	//LCD position and Scrolling 
	if (address == 0xFF42) //SCY(Scroll Y)
		data = SCY;
	if (address == 0xFF43) //SCX (Scroll X)
		data = SCX;
	if (address == 0xFF44) //LY (LCD Y Coordinate) (R)
		data = LY;
	if (address == 0xFF45) //LY LYC (LY Compare) (R/W)
		data = LYC;
	if (address == 0xFF4A) //WY (Window Y Position) (R/W)
		data = WY;
	if (address == 0xFF4B) //WX (Window X Position + 7) (R/W)
		data = WX;

	return data;
}

void SM83_PPU::cpuWrite(uint16_t address, uint8_t data)
{
	//VRAM
	if (0x8000 <= address && address <= 0x9FFF)
	{
		//LCDC.PPUEnable = 0; //Force write to VRAM
		//std::cout << (std::hex) << "0x" << (int)LCDC.PPUEnable << std::endl;
		//CPU have full access to VRAM and OPM if PPU disabled
		if (LCDC.PPUEnable == 0) {
			ppuWrite(address, data);
		}

		else
		{
		//CPU does not have access to VRAM during mode 3
			if (STAT.modeFlag != 3)
				ppuWrite(address, data);
			
		}
	}

	//Registers and miscellaneous
	else
		ppuWrite(address, data);

}

uint8_t SM83_PPU::cpuRead(uint16_t address)
{
	uint8_t data = 0x00;
	//VRAM
	if (0x8000 >= address && address <= 0x9FFF)
	{
		//LCDC.PPUEnable = 0; //Force to read VRAM

		//CPU have full access to VRAM and OPM if PPU disabled
		if (LCDC.PPUEnable == 0) {
			data = ppuRead(address);
		}

		else
		{
			//CPU does not have access to VRAM during mode 3
			if (STAT.modeFlag == 3)
				data = 0xFF;
			else
				data = ppuRead(address);

		}
	}

	//Registers and miscellaneous
	else
		data = ppuRead(address);

	return data;
}

//Clocks one tick of PPU
void SM83_PPU::clock()
{
	LY = scanLine; //Give value to LY register

	if(scanLine == 0) bFrameComplete = false;

	//Update the LYC = LY flag
	if (LY == LYC) STAT.LYFlag = 1;
	else STAT.LYFlag = 0;

	//LYC = LY stat interrupt:
	if (!bStatInterruptBlock && STAT.LY == 1)
	{
		if (STAT.LYFlag == 1)
		{
			bus->cpu.irqLCDSTAT();
			bStatInterruptBlock = true;
		}
	}

	//Cycle through mode 2-3-0-1
	switch(STAT.modeFlag)
	{
		
	case 2: //OAM SCAN
		Mode2();
		break;
		
		
	case 3: //Drawing pixels
		Mode3();
		bStatInterruptBlock = false; //Unblock STAT interrupt
		break;

		
	case 0: //HBlank
		Mode0();
		break;

	case 1: //VBlank
		Mode1();
		break;

	default:
		break;
	}

	dots++; //Increment dots
}

bool SM83_PPU::frameComplete()
{
	return bFrameComplete;
}

//HBlank
void SM83_PPU::Mode0()
{

	if (x == 160) //Trigger interrupts when entering HBlank
	{
		if (!bStatInterruptBlock && STAT.HBLank == 1)
		{
			bus->cpu.irqLCDSTAT();
			bStatInterruptBlock = true;
		}
		else
			bStatInterruptBlock = false;

		//Reset values of mode 3 
		Fetcher = 0;
		cycles = 0;
		X = 0;
		x = 0;
		pixels = 0;
	}

	if ((dots % 456) == 455) 
	{
		if (scanLine == 143) STAT.modeFlag = 1; //Go to VBlank
		else if (scanLine < 143) STAT.modeFlag = 2; //Go to OAM scan
		scanLine++; //Increment scanLine
	}	
}

//VBlank
void SM83_PPU::Mode1()
{
	//Request interrupts when entering VBlank
	if (scanLine == 144)
	{
		bus->cpu.irqVBlank();
		if (!bStatInterruptBlock && STAT.VBlank == 1)
		{
			bus->cpu.irqLCDSTAT();
			bStatInterruptBlock = true;
		}
		else
			bStatInterruptBlock = false;
	}

	if ((dots % 456) == 455)
	{
		scanLine++; //Increment scanLine
		if (scanLine == 154) //End of VBlank, now PPU will go back to scanLine 0 and enter OAM scan
		{
			STAT.modeFlag = 2; //Go to OAM scan
			//Reset values
			scanLine = 0;
			LY = 0;
			dots = 0;
			bFrameComplete = true;
		}
	}
	
}

//OAM scan
void SM83_PPU::Mode2()
{
	if ((dots % 456) == 79)
	{
		STAT.modeFlag = 3; //Go to mode 3
		nPauseDots = SCX & 0x7; //Read SCX to find how many dot to pause

	}
}
	

//Pixel transfer 
void SM83_PPU::Mode3()
{
	LCDC.BGAndWindowPriority = 1; //Force BG rendering
	//Window/Background Rendering:
	if (LCDC.WindowEnable == 1 && LCDC.BGAndWindowPriority == 1)
	{
		//When entering window, destroy the current pixel FIFO
		if ((WX - 7) == x && WY == scanLine)
		{
			//Destroy current pixel FIFO:
			BGPixelFIFO = 0x0000;
			pixels = 0;
			bMapArea = LCDC.WindowsTileMapArea;
			//X and Y coordinate equal fetcher's coordinates
			XX = X;
			YY = scanLine;
			
		}
		else if ((WX - 7) < x && WY < scanLine)
		{
			bMapArea = LCDC.WindowsTileMapArea;
			//X and Y coordinate equal fetcher's coordinates
			XX = X;
			YY = scanLine;
		}
		//Background rendering
		else if ((WX - 7) > x && WY > scanLine)
		{
		bMapArea = LCDC.BGTileMapArea;
		XX = ((SCX >> 3) + X) & 0x1F;
		YY = (scanLine + SCY) & 0xFF;
		}

	}
	//Background rendering, windows disabled
	else if(LCDC.BGAndWindowPriority == 1)
	{
		bMapArea = LCDC.BGTileMapArea;
		XX = ((SCX >> 3) + X) & 0x1F;
		YY = (scanLine + SCY) & 0xFF;
	}

	//FIFO Pixel Fetcher:
	switch (Fetcher)
	{
	case 0: //Get Tile
		if (cycles == 0)
		{
			tileRowAddress = getTile(XX, YY, bMapArea); //Gets the address of low byte tile row
			cycles = 2;
			Fetcher = 1; //Next, get low byte data
		}
		break;
	case 1: //getTileDataLow
		if (cycles == 0)
		{
			getTileDataLow(tileRowAddress);
			cycles = 2;
			Fetcher = 2; //Next, get high byte data
		}
		break;
	case 2: //getTileDataLow
		if (cycles == 0)
		{
			getTileDataHigh(tileRowAddress);
			cycles = 2;
			Fetcher = 3; //Next, sleep or push
			BGPixelFIFObuffer = 0x0000;
			//Give data to buffer:
			for (int i = 7; i >= 0; i--)
			{
				uint16_t bits = ((TileDataHigh & (1 << i)) << (8 + (7 - i))) | ((TileDataLow & (1 << i)) << (8 + (6 - i)));
				BGPixelFIFObuffer |= (bits >> 2 * (7 - i));
			}
			//Also push if allowed
			if (pixels <= 0)
			{
				BGPixelFIFO |= BGPixelFIFObuffer;
				X++; //increment fetcher coordinate
				pixels += 8;
			}
		}
		break;
	case 3: //push/sleep
		if (cycles == 0)
		{
			cycles = 1;
			if (pixels <= 0)
			{
				BGPixelFIFO |= BGPixelFIFObuffer;
				pixels += 8;
				X++; //increment fetcher coordinate
				Fetcher = 0; //Go back to get tile
			}
		}
		break;
	default:
		break;
	}

	cycles--; //Decrement cycles


	//Pop pixel to LCD only if the number of pixels is over 0
	if (pixels > 0)
	{
		//At the beginning of a scanline throw away pixel equal to the 3 lower bits of SCX ie. pause for the value of SCX
		if (nPauseDots == 0) {
			colorIndex = (BGPixelFIFO & 0xC000) >> 14;
			argb = palettes.at((BGP.reg & (0x03 << 2 * colorIndex)) >> 2 * colorIndex);
			LCDscreen.at(x + 160 * scanLine) = argb;
			BGPixelFIFO <<= 2;
			x++; //increment x coordinate
		}
		else
		{
			BGPixelFIFO <<= 2;
			nPauseDots--;
		}
		pixels--; //Decrement pixels
	}

	if (x == 160)//Hblank
	{
		STAT.modeFlag = 0; //Go to mode 0
	}
}

//Gets the TileID from tilemap
uint16_t SM83_PPU::getTile(uint8_t X, uint8_t Y, bool bMapArea)
{
	uint8_t tileID = 0x00;
	uint8_t TileY = Y >> 3; //Get tile Y coordinate in map	
	uint8_t TileRow = Y & 0x7; //Get tile row
	if (bMapArea == 1)
	{
		tileID = ppuRead(0x9C00 + X + 32 * TileY);
		return getTileMap(tileID, TileRow);
	}
	else
	{
		tileID = ppuRead(0x9800 + X + 32 * TileY);
		return getTileMap(tileID, TileRow);
	}

}

//Get low byte of tile data
void SM83_PPU::getTileDataLow(uint16_t address)
{
	TileDataLow = VRAM.at(address);
}

//Get high byte of tile data
void SM83_PPU::getTileDataHigh(uint16_t address)
{
	TileDataHigh = VRAM.at(address + 1);
}

//Gets the address in VRAM of the low byte of the tile row from Tile ID
uint16_t SM83_PPU::getTileMap(uint8_t tileID, uint8_t row)
{
	uint16_t startAddress = 0x0000;
	uint16_t lowAddress = 0x0000;
	if (LCDC.BGAndWindowTileDataArea == 1)
	{
		startAddress = 0x0000;
		lowAddress = startAddress + (2 * row) + 16 * tileID;
	}
	else
	{
		startAddress = 0x1000;
		lowAddress = startAddress + (2 * row) + 16 * (int8_t)tileID;
	}
	
	return lowAddress;

}



//For debugging
std::array<uint32_t, 24576>& SM83_PPU::getTileTable()
{
	//Loop through 16*24 tiles = 16*24*8*8 = 24576 pixels
	for (int nTileY = 0; nTileY < 24; nTileY++)
	{
		for (int nTileX = 0; nTileX < 16; nTileX++)
		{
			//Loop trough pixel in the tile
			for (int y = 0; y < 8; y++)
			{
				for (int x = 0; x < 8; x++)
				{
					//Get Colorindex and the corresponding rgb value. The palettes are decided by BGP register.
					RowLowByte = VRAM.at(2 * y + (nTileX + 16 * nTileY) * 16);
					RowHighByte = VRAM.at((2 * y + 1) + (nTileX + 16 * nTileY) * 16);
					lowBit = (RowLowByte & (0x1 << (7 - x))) >> (7 - x);
					highBit = (RowHighByte & (0x1 << (7 - x))) >> (7 - x);
					colorIndex = (highBit << 1) | lowBit;
					argb = palettes.at((BGP.reg & (0x03 << 2 * colorIndex)) >> 2 * colorIndex);

					tileData.at((nTileX * 8 + 8 * 16 * 8 * nTileY) + (x + 16 * 8 * y)) = argb;
				}
			}
		}
	}

	return tileData;
}

std::array<uint32_t, 65536>& SM83_PPU::getTileMap1Data()
{
	uint16_t lowAddress = 0x0000;
	uint8_t highByte = 0x00;
	uint8_t lowByte = 0x00;
	uint16_t rowData = 0x00;
	uint16_t buffer = 0x00; //Buffer to give data to rowData
	//Loop through 32 x-coordinates of tiles and 256 rows in tile map 
	for (int row = 0; row < 256; row++)
	{
		for (int X = 0; X < 32; X++)
		{
			lowAddress = getTile(X, row, 1); //Get the low address of the tile row
			lowByte = VRAM.at(lowAddress);
			highByte = VRAM.at(lowAddress + 1);
			buffer = 0x0000;
			//Extract row data
			for (int i = 7; i >= 0; i--)
			{
				uint16_t bits = ((highByte & (1 << i)) << (8 + (7 - i))) | ((lowByte & (1 << i)) << (8 + (6 - i)));
				buffer |= (bits >> 2 * (7 - i));
			}
			rowData = buffer;
			//Loop through the 8 row pixels
			for (int x = 0; x < 8; x++)
			{
				colorIndex = (rowData & 0xC000) >> 14;
				argb = palettes.at((BGP.reg & (0x03 << 2 * colorIndex)) >> 2 * colorIndex);
				tileMap1.at(x + 8 * X + 256 * row) = argb;
				rowData <<= 2;
			}

		}
	}
	return tileMap1;
}

std::array<uint32_t, 65536>& SM83_PPU::getTileMap0Data()
{
	uint16_t lowAddress = 0x0000;
	uint8_t highByte = 0x00;
	uint8_t lowByte = 0x00;
	uint16_t rowData = 0x00;
	uint16_t buffer = 0x00; //Buffer to give data to rowData
	//Loop through 32 x-coordinates of tiles and 256 rows in tile map 
	for (int row = 0; row < 256; row++)
	{
		for (int X = 0; X < 32; X++)
		{
			lowAddress = getTile(X, row, 0); //Get the low address of the tile row
			lowByte = VRAM.at(lowAddress);
			highByte = VRAM.at(lowAddress + 1);
			buffer = 0x0000;
		
			
			//Extract row data
			for (int i = 7; i >= 0; i--)
			{
				uint16_t bits = ((highByte & (1 << i)) << (8 + (7 - i))) | ((lowByte & (1 << i)) << (8 + (6 - i)));
				buffer |= (bits >> 2 * (7 - i));
			}
			
			rowData = buffer;
			
			//Loop through the 8 row pixels
			for (int x = 0; x < 8; x++)
			{
				colorIndex = (rowData & 0xC000) >> 14;
				argb = palettes.at((BGP.reg & (0x03 << 2 * colorIndex)) >> 2 * colorIndex);
				tileMap0.at(x + 8 * X + 256 * row) = argb;
				rowData <<= 2;
			}
			

			/*
			for (int x = 0; x < 8; x++)
			{
				lowBit = (lowByte & (0x1 << (7 - x))) >> (7 - x);
				highBit = (highByte & (0x1 << (7 - x))) >> (7 - x);
				colorIndex = (highBit << 1) | lowBit;
				argb = palettes.at((BGP.reg & (0x03 << 2 * colorIndex)) >> 2 * colorIndex);
				tileMap0.at(x + 8 * X + 256 * row) = argb;
			}
			*/
		}
	}
	return tileMap0;
}
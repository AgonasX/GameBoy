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

	//LCDscreen.fill(0xFFFFFFFF);

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
	}

	//OAM
	if (0xFE00 <= address && address <= 0xFE9F)
		OAM.at(address - 0xFE00) = data;

	//Palett registers
	if (address == 0xFF47) //BG palett data
		BGP.reg = data;
	if (address == 0xFF48) //OBP0 palett data
		OBP0.reg = data;
	if (address == 0xFF48) //OBP1 palett data
		OBP1.reg = data;

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
		data = VRAM.at(address - 0x8000);

	//OAM
	if (0xFE00 <= address && address <= 0xFE9F)
		data = OAM.at(address - 0xFE00);

	//Palett registers
	if (address == 0xFF47) //BG palett data
		data = BGP.reg;
	//Palett registers
	if (address == 0xFF48) //OBP0 palett data
		data = OBP0.reg;
	//Palett registers
	if (address == 0xFF49) //OBP1 palett data
		data = OBP1.reg;

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
		//CPU have full access to VRAM and OAM if PPU disabled
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

	//OAM
	else if (0xFE00 <= address && address <= 0xFE9F)
	{
		//CPU have full access to VRAM and OAM if PPU disabled
		if (LCDC.PPUEnable == 0) {
			ppuWrite(address, data);
		}

		else
		{
			//CPU does not have access to OAM during mode 2 and mode 3
			if (STAT.modeFlag != 3 && STAT.modeFlag != 2)
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


	//OAM
	else if (0xFE00 <= address && address <= 0xFE9F)
	{
		//CPU have full access to VRAM and OAM if PPU disabled
		if (LCDC.PPUEnable == 0) {
			data = ppuRead(address);
		}

		else
		{
			//CPU does not have access to OAM during mode 2 and mode 3
			if (STAT.modeFlag != 3 && STAT.modeFlag != 2)
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

	if(scanLine == 0) bFrameComplete = false; //Reset frame complete

	//Update the LYC = LY flag
	if (LY == LYC) STAT.LYFlag = 1;
	else STAT.LYFlag = 0;

	//LYC = LY stat interrupt:
	if (!bStatInterruptBlock && STAT.LY == 1 && (dots % 456) == 0)
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
		vOAMObjects.clear(); //Clear vOAMObjects
		//Clear Pixel FIFO
		BGPixelFIFO = 0x00000000;
		PixelFIFO = 0x00;
		PaletteNum = 0x00;
	}

	if ((dots % 456) == 455) 
	{
		scanLine++; //Increment scanLine
		if (scanLine == 144) STAT.modeFlag = 1; //Go to VBlank
		else if (scanLine < 144) STAT.modeFlag = 2; //Go to OAM scan
	}	
}

//VBlank
void SM83_PPU::Mode1()
{
	//Request interrupts when entering VBlank
	if (scanLine == 144 && (dots % 456) == 0)
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
			WindowY = 0;
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
		
		int nOAMs = 0; //For counting sprites

		struct OAMobject sOAMobject;

		//Get Object size
		uint8_t ObjectSize = 0x00;
		(LCDC.OBJSize == 0) ? ObjectSize = 0x8 : ObjectSize = 0x10;


		//Scan through OAM to find 10 OAM sprites to be drawn on a scanline
		for (int nSprites = 0; nSprites < 40; nSprites++)
		{
			if (nOAMs == 10) break;

			//A sprite is selected when LY is between the sprites height (taking the sprite Y coordinate in consideration)
			if ((LY + 16 >= OAM.at(4 * nSprites)) && (LY + 16 < (OAM.at(4 * nSprites) + ObjectSize)))
			{
				//Give OAM memory to sOAMobjects
				sOAMobject.Ypos = OAM.at(4 * nSprites);
				sOAMobject.Xpos = OAM.at(4 * nSprites + 1);
				sOAMobject.TileIndex = OAM.at(4 * nSprites + 2);
				sOAMobject.Flags = OAM.at(4 * nSprites + 3);

				vOAMObjects.push_back(sOAMobject);
				nOAMs++;

			}
		}
		
		STAT.modeFlag = 3; //Go to mode 3
		nPauseDots = (SCX & 0x7); //Read SCX to find how many dots to pause 
	}
}
	

//Pixel transfer 
void SM83_PPU::Mode3()
{
	//LCDC.WindowEnable = 0; //Force disable Window rendering


	//Window/Background Rendering:
	if (LCDC.WindowEnable == 1)
	{
		//When entering window, destroy the current pixel FIFO
		if ((WX - 7) == x && WY <= scanLine && !bInWindow)
		{
			//Destroy current pixel FIFO:
			BGPixelFIFO = 0x00000000;
			pixels = 0;
			bMapArea = LCDC.WindowsTileMapArea;
			//X and Y coordinate equal Window's internal coordinate
			XX = WindowX;
			YY = WindowY;
			Fetcher = 0; //Fetcher reset to step 1
			bInWindow = true;	
		}
		else if ((WX - 7) <= x && WY <= scanLine && bInWindow)
		{
			bMapArea = LCDC.WindowsTileMapArea;
			//X and Y coordinate equal fetcher's coordinates
			XX = WindowX;
			YY = WindowY;
			bInWindow = true;
			
		}
		//Background rendering
		else
		{
		bInWindow = false;
		bMapArea = LCDC.BGTileMapArea;
		XX = ((SCX >> 3) + X) & 0x1F;
		YY = (scanLine + SCY) & 0xFF;
		}

	}

	//Background rendering, windows disabled
	else if(LCDC.WindowEnable == 0)
	{
		bInWindow = false;
		bMapArea = LCDC.BGTileMapArea;
		XX = ((SCX >> 3) + X) & 0x1F;
		YY = (scanLine + SCY) & 0xFF;
	}

	/*
	//Object rendering
	if (LCDC.OBJEnable == 1)
	{
		//Loop through sprites from OAM scan
		for (int nSprites = 0; nSprites < vOAMObjects.size(); nSprites++)
		{
			if ((vOAMObjects.at(nSprites).Xpos - 8) <= x && x < vOAMObjects.at(nSprites).Xpos && bFetchObj == false)
			{
				//Process each relevant sprites only once
				if (std::find(vSpriteIndex.begin(), vSpriteIndex.end(), nSprites) == vSpriteIndex.end())
				{
					SpriteIndex = nSprites;
					vSpriteIndex.push_back(SpriteIndex);
					bPauseRender = true;
					bFetchObj = true;

					Fetcher = 0; //Start Fetching OBJ data
					//X and Y coordinate equal fetcher's coordinates
					XX = X;
					YY = scanLine;
					break;
				}

				else //We don't find any more objects, so unpause render
				{
					if (bFetchObj == false) bPauseRender = false;
				}
			}	
		}
	}
	else 
		bPauseRender = false;
	*/

	//FIFO Pixel Fetcher:
	switch (Fetcher)
	{
	case 0: //Get Tile
		if (cycles == 0)
		{
			//Gets the address of low byte tile row
			if (bFetchObj == true) tileRowAddress = getTile(XX, YY, bMapArea, true);
			else tileRowAddress = getTile(XX, YY, bMapArea);

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
	case 2: //getTileDataHigh
		if (cycles == 0)
		{
			getTileDataHigh(tileRowAddress);
			cycles = 2;
			Fetcher = 3; //Next, sleep or push

			if (!bFetchObj)
			{
				BGPixelFIFOLatch = 0x0000;
				//Give data to BGpixelFIFOlatch:
				for (int i = 7; i >= 0; i--)
				{
					uint16_t bits = ((TileDataHigh & (1 << i)) << (8 + (7 - i))) | ((TileDataLow & (1 << i)) << (8 + (6 - i)));
					BGPixelFIFOLatch |= (bits >> 2 * (7 - i));
				}
				//Also push if allowed
				if (pixels <= 8)
				{
					BGPixelFIFO |= BGPixelFIFOLatch;

					//If 0 pixels move the 8 pixels to the front
					if (pixels == 0) BGPixelFIFO <<= 16;

					X++; //increment fetcher coordinate
					//Increment window coordinates
					if (bInWindow)
					{
						WindowX++;
					}

					pixels += 8;
					Fetcher = 0; //Return to fetcher step 1
					
				}
			}
			else //OBJ fetch and pixel mixing
			{
				//Give data to OBJPixelFIFO
				for (int i = 7; i >= 0; i--)
				{
					uint16_t bits = ((TileDataHigh & (1 << i)) << (8 + (7 - i))) | ((TileDataLow & (1 << i)) << (8 + (6 - i)));
					if(GetOBJFlag(XFlip,vOAMObjects.at(SpriteIndex)) == 0) 
						OBJPixelFIFO |= (bits >> 2 * (7 - i));
					else //XFlip
						OBJPixelFIFO |= (bits >> 2 * i);
				}

				//Pixel Mixing
				//Loop through 8 pixels
				for (int i = 7; i >= 0; i--)
				{
					//Compare pixels
					if (GetOBJFlag(OBJPriority, vOAMObjects.at(SpriteIndex)) == 0) //OBJ priority 0, draws over the background
					{
						if ((OBJPixelFIFO & (0x3 << 2 * i)) > 0) //Draw OBJ pixel only if OBJ color is not 0 (transparent)
						{
							if ((PixelFIFO & (1 << i)) == 0) //Check if we already have a object pixel
							{
								uint32_t BGPixelFIFOTemp = BGPixelFIFO & ~(0x3 << (2 * i + 16));
								BGPixelFIFO = (OBJPixelFIFO & (0x3 << 2 * i)) << 16;
								BGPixelFIFO |= BGPixelFIFOTemp;
								PixelFIFO |= (1 << i);

								//Palette number
								if (GetOBJFlag(PaletteNumber, vOAMObjects.at(SpriteIndex)) == 1)
								{
									PaletteNum |= (1 << i);
								}
							}
						}
					}
					else //OBJ priority 1, BG and Window colors 1-3 over the OBJ
					{
						colorIndex = BGPixelFIFO & (0x3 << (2 * i + 16));
						if (((BGP.reg & (0x03 << 2 * colorIndex)) >> 2 * colorIndex) == 0)
						{
							if ((PixelFIFO & (1 << i)) == 0) //Check if we already have a object pixel
							{
								uint32_t BGPixelFIFOTemp = BGPixelFIFO & ~(0x3 << (2 * i + 16));
								BGPixelFIFO = (OBJPixelFIFO & (0x3 << 2 * i)) << 16;
								BGPixelFIFO |= BGPixelFIFOTemp;
								PixelFIFO |= (1 << i);

								//Palette number
								if (GetOBJFlag(PaletteNumber, vOAMObjects.at(SpriteIndex)) == 1)
								{
									PaletteNum |= (1 << i);
								}
							}
						}
					}
				}
				bFetchObj = false;
				Fetcher = 0; //Go back to get tile
			}
		}
		break;
	case 3: //push/sleep
		if (cycles == 0)
		{
			cycles = 1;
			if (pixels <= 8)
			{
				BGPixelFIFO |= BGPixelFIFOLatch;

				//If 0 pixels move the 8 pixels to the front
				if (pixels == 0) BGPixelFIFO <<= 16;

				pixels += 8;
				X++; //increment fetcher coordinate
				if (bInWindow) WindowX++; //Increment window coordinate
				Fetcher = 0; //Go back to get tile
			}
		}
		break;
	default:
		break;
	}

	cycles--; //Decrement cycles

	//Pop pixel to LCD only if the number of pixels is over 8
	if (pixels > 8)
	{
		//nPauseDots = 0;
		if (!bPauseRender)
		{
			//At the beginning of a scanline throw away pixel equal to the 3 lower bits of SCX. (If not in Window)
			if (bInWindow) nPauseDots = 0;
			if (nPauseDots == 0) {
				colorIndex = (BGPixelFIFO & 0xC0000000) >> 30;
				//Background rendering
				if ((PixelFIFO & 0x80) == 0)
				{
					argb = palettes.at((BGP.reg & (0x03 << 2 * colorIndex)) >> 2 * colorIndex);

					if (LCDC.BGAndWindowPriority == 0) argb = palettes.at(0); //Background and Window disabled
				}
				//Object rendering
				else
				{
					//OBP0
					if ((PaletteNum & 0x80) == 0)
						argb = palettes.at((OBP0.reg & (0x03 << 2 * colorIndex)) >> 2 * colorIndex);
					//OBP1
					else 
						argb = palettes.at((OBP1.reg & (0x03 << 2 * colorIndex)) >> 2 * colorIndex);
				}

				LCDscreen.at(x + 160 * scanLine) = argb;
				x++; //increment x coordinate
			}
			else
			{
				nPauseDots--;
			}

			BGPixelFIFO = BGPixelFIFO << 2;
			PixelFIFO <<= 1;
			PaletteNum <<= 1;
			pixels--; //Decrement pixels
			vSpriteIndex.clear(); //Clear Sprite index vector
		}
	}
	


	if (x == 160)//Hblank
	{
		STAT.modeFlag = 0; //Go to mode 0
		//Increment Window internal coordinates
		if(bInWindow)
		{
			WindowY++;
			WindowX = 0;
		}
	}
}

//Gets the TileID from tilemap
uint16_t SM83_PPU::getTile(uint8_t X, uint8_t Y, bool bMapArea, bool bFetchObject)
{
	uint8_t tileID = 0x00;
	uint8_t TileY = Y >> 3; //Get tile Y coordinate in map	
	uint8_t TileRow = Y & 0x7; //Get tile row

	if (bFetchObject)
	{
		uint8_t OBJSize = 0;
		if (LCDC.OBJSize == 0) {
			tileID = vOAMObjects.at(SpriteIndex).TileIndex;
			OBJSize = 7;
		}
		else
		{
			tileID = vOAMObjects.at(SpriteIndex).TileIndex >> 1;
			OBJSize = 15;
		}
		//YFLip
		if (GetOBJFlag(YFlip, vOAMObjects.at(SpriteIndex)) == 0)
			TileRow = scanLine - (vOAMObjects.at(SpriteIndex).Ypos - 16);
		else
			TileRow = OBJSize - (scanLine - (vOAMObjects.at(SpriteIndex).Ypos - 16));

		return getTileMap(tileID, TileRow, true);
	}
	else
	{
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
uint16_t SM83_PPU::getTileMap(uint8_t tileID, uint8_t row, bool bFetchObject)
{
	uint16_t startAddress = 0x0000;
	uint16_t lowAddress = 0x0000;
	if (bFetchObject)
	{
		startAddress = 0x0000;
		if (LCDC.OBJSize == 0)
			lowAddress = startAddress + (2 * row) + 16 * tileID;
		else
			lowAddress = startAddress + (2 * row) + 32 * tileID;
	}
	else
	{
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
	}
	return lowAddress;

}

uint8_t SM83_PPU::GetOBJFlag(OBJFLAGS f, OAMobject sOAMobject)
{
	return ((sOAMobject.TileIndex & f) > 1) ? 0x1 : 0x0;
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


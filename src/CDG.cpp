/*
	cdgdump - dump cdg file to SDL window / png
    Copyright (C) 2007 wardyang

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// CD+G packets arrive at rate of 300 per second, or 1 every 0.0033333 seconds:

// 75 sectors per second from CD, 96 bytes of subchannel data per packet
// One subchannel packet=24 bytes = 4 subchannel packets per sector = 300 packets/s

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "CDG.h"
#include "SDLWin.h"
#include "PNGOut.h"

#define SC_MASK           0x3F
#define SC_CDG_COMMAND    0x09

#define CDG_MEMORYPRESET  1
#define CDG_BORDERPRESET  2
#define CDG_TILEBLKNORM   6
#define CDG_TILEBLKXOR   38
#define CDG_SCROLLPRE    20
#define CDG_SCROLLCOPY   24
#define CDG_DEFTRANS     28
#define CDG_LOADCOLLO    30
#define CDG_LOADCOLHI    31

extern bool Debug;
extern bool TextOutput;
extern bool VideoOutput;
extern bool RealTime;
extern bool PngOutput;

CDG::CDG(int InHandle)
{
	Handle=InHandle;
	
	// Initialise screen state
	for(int i=0;i<16;i++){
		ScreenState.ColourTable[i].Red=0;
		ScreenState.ColourTable[i].Green=0;
		ScreenState.ColourTable[i].Blue=0;
	}
	ScreenState.BorderColour=0;
	PresetScreen(0);
	ScreenState.TransparentColour=-1;
	ScreenState.HOffset=0;
	ScreenState.VOffset=0;
}

void CDG::PresetScreen(unsigned char Colour)
{
	for(int y=0;y<CDG_SCREEN_Y;y++){
		for(int x=0;x<CDG_SCREEN_X;x++){
			ScreenState.Field[x][y]=Colour;
		}		
	}	
}

bool CDG::Process()
{
	bool Ok=true;
	int Bytes;
	CDG_PacketStruct Packet;
	bool Changed;
	unsigned int PacketNo=0;
	CDG_StateStruct OldScreenState;
	
	do{
		if(PngOutput){
			// Open frame list file
			if(!PngInitialise()) break;
		}
		
		Changed=true;
		while(Ok){
			Bytes=read(Handle,&Packet,sizeof(CDG_PacketStruct));
			if(Bytes<sizeof(CDG_PacketStruct)) break;

			if ((Packet.Command & SC_MASK) == SC_CDG_COMMAND) {	// CD+G?
				switch (Packet.Instruction & SC_MASK) {
				case CDG_MEMORYPRESET:
					ProcessMemoryPreset((CDG_MemPresetStruct *) Packet.Data);
					Changed=true;
					break;
				case CDG_BORDERPRESET:
					ProcessBorderPreset((CDG_BorderPresetStruct *) Packet.Data);
					Changed=true;
					break;
				case CDG_TILEBLKNORM:
					ProcessTileBlk((CDG_TileBlkStruct *) Packet.Data,false);
					Changed=true;
					break;
				case CDG_TILEBLKXOR:
					ProcessTileBlk((CDG_TileBlkStruct *) Packet.Data,true);
					Changed=true;
					break;
				case CDG_SCROLLPRE:
					ProcessScroll((CDG_ScrollStruct *) Packet.Data,false);
					Changed=true;
					break;
				case CDG_SCROLLCOPY:
					ProcessScroll((CDG_ScrollStruct *) Packet.Data,true);
					Changed=true;
					break;
				case CDG_DEFTRANS:
					ProcessTransparency((CDG_TransColStruct *) Packet.Data);
					break;
				case CDG_LOADCOLLO:
					ProcessColourTable((CDG_ColourTableStruct *) Packet.Data,0);
					Changed=true;
					break;
				case CDG_LOADCOLHI:
					ProcessColourTable((CDG_ColourTableStruct *) Packet.Data,1);
					Changed=true;
					break;
				case 19: // Attempt an error correction - 19 is 38 shifted right one bit.
					printf("Attempting command 19 error correction...\n");
					ProcessTileBlk((CDG_TileBlkStruct *) Packet.Data,true);
					Changed=true;
					break;
				case 12: // Attempt an error correction - 12 is 6 shifted left one bit.
				case 3: // Attempt an error correction - 3 is 6 shifted right one bit.
					printf("Attempting command 12 error correction...\n");
					ProcessTileBlk((CDG_TileBlkStruct *) Packet.Data,false);
					Changed=true;
					break;
				default:
					printf("Unknown CD+G command: %d\n",Packet.Instruction&SC_MASK);
					if(Debug) Ok=false;
					break;
				}
			}
			else{
				if(Debug && TextOutput) printf("Non-CD+G packet: 0x%02x\n",Packet.Command&SC_MASK);
			}
			
			if(VideoOutput){
				if(!EventPollSDL()) break;
				if(RealTime){
					DelayUntilSDL(((PacketNo+1)*1000)/300);
				}
				if(Changed){
					if(PacketNo==0 || memcmp(&ScreenState,&OldScreenState,sizeof(CDG_StateStruct))!=0){
						RenderField();
						memcpy(&OldScreenState,&ScreenState,sizeof(CDG_StateStruct));
					}
					Changed=false;
				}
			}
			
			if(PngOutput){
				// We write pngs at a rate of 25 per second which is one every 12 packets
				if((PacketNo%12)==0){
					if(!WritePng(&ScreenState,PacketNo)) break;
				}
			}
			
			++PacketNo;
		}
		if(!Ok) break;
		
		if(Debug){
			float Secs;
			int Mins;
			
			--PacketNo;
			Secs=(float)PacketNo/300;
			Mins=(int)(Secs/60);
			Secs-=(float)Mins*60;
			printf("%d packets processed (%d:%02f)\n",PacketNo,Mins,Secs);
		}
	} while(0);
	
	if(PngOutput){
		PngCleanup();
	}
	
	return Ok;
}

bool CDG::ProcessMemoryPreset(CDG_MemPresetStruct *MemPreset)
{
	bool Ok=true;
	unsigned char Colour;
	
	// Unpack details
	Colour=MemPreset->Colour&0x0f;

	// Update screen state
	PresetScreen(Colour);

	// Dump details
	if(TextOutput) printf("Memory preset: Colour %d, Repeat %d\n",Colour,MemPreset->Repeat&0x0f);
	
	return Ok;
}

bool CDG::ProcessBorderPreset(CDG_BorderPresetStruct *BorderPreset)
{
	bool Ok=true;
	unsigned char Colour;

	// Unpack details
	Colour=BorderPreset->Colour&0x0f;
	
	// Update screen state
	ScreenState.BorderColour=Colour;
	
	// Dump details
	if(TextOutput) printf("Border preset: Colour %d\n",Colour);
	
	return Ok;
}

bool CDG::ProcessTileBlk(CDG_TileBlkStruct *TileBlk,bool Xor)
{
	bool Ok=true;
	unsigned char Colour0;
	unsigned char Colour1;
	unsigned char Row;
	unsigned char Column;
	int rx;
	int ry;
	int ox;
	int oy;
	unsigned char Mask;
	unsigned char PxByte;
	unsigned char Colour;
	
	// Unpack details
	Colour0=TileBlk->Colour0&0x0f;
	Colour1=TileBlk->Colour1&0x0f;
	Row=TileBlk->Row&0x1f;
	Column=TileBlk->Column&0x3f;
	
	if(Row>=CDG_ROWS || Column>=CDG_COLS){
		printf("Tile block row %d col %d is out of range\n",Row,Column);
		if(Debug) Ok=false;
	}
		else{
		rx=Column*6;
		ry=Row*12;
		
		// Update screen state
		oy=ry;
		for(int y=0;y<12;y++){
			ox=rx;
			Mask=0x20;
			PxByte=TileBlk->TilePixels[y];
			for(int x=0;x<6;x++){
				Colour=(PxByte&Mask?Colour1:Colour0);
				if(Xor)
					ScreenState.Field[ox][oy]=(ScreenState.Field[ox][oy]^Colour)&0x0f;
				else
					ScreenState.Field[ox][oy]=Colour;
				Mask>>=1;
				++ox;
			}
			++oy;
		}
		
		// Dump details
		if(TextOutput){
			printf("Tile block (%s): Colour 0 %d, Colour 1 %d, Row %d, Col %d, x %d, y %d\n",
				(Xor?"XOR":"Normal"),Colour0,Colour1,Row,Column,rx,ry);
			if(Debug){
				for(int y=0;y<12;y++){
					printf("          ");
					Mask=0x20;
					PxByte=TileBlk->TilePixels[y];
					for(int x=0;x<6;x++){
						if(PxByte&Mask) printf("#");
						else printf(".");
						Mask>>=1;
						++ox;
					}
					++oy;
					printf("\n");
				}
			}
		}
	}
	
	return Ok;	
}

bool CDG::ProcessScroll(CDG_ScrollStruct *Scroll,bool Copy)
{
	bool Ok=true;
	
	unsigned char Colour;
	unsigned char SHCmd;
	unsigned char HOffset;
	unsigned char SVCmd;
	unsigned char VOffset;
	
	unsigned char HBuf[6];
	unsigned char VBuf[12];
	
	// Unpack details
	Colour=Scroll->Colour&0x0f;
	SHCmd=(Scroll->HScroll&0x30)>>4;
	HOffset=Scroll->HScroll&0x0f;
	SVCmd=(Scroll->VScroll&0x30)>>4;
	VOffset=Scroll->VScroll&0x0f;

	// Update screen state
	ScreenState.HOffset=HOffset;
	ScreenState.VOffset=VOffset;
	
	switch(SHCmd){
	case 1: // Scroll right
		if(!Copy) for(int i=0;i<6;i++) HBuf[i]=Colour;
		for(int y=0;y<CDG_SCREEN_Y;y++){
			if(Copy) for(int i=0;i<6;i++) HBuf[i]=ScreenState.Field[(CDG_SCREEN_X-6)+i][y];
			for(int x=CDG_SCREEN_X-7;x>=0;x--) ScreenState.Field[x+6][y]=ScreenState.Field[x][y];
			for(int i=0;i<6;i++) ScreenState.Field[i][y]=HBuf[i];
		}
		// TODO REMOVE ME
		exit(3);
		break;
	case 2: // Scroll left
		if(!Copy) for(int i=0;i<6;i++) HBuf[i]=Colour;
		for(int y=0;y<CDG_SCREEN_Y;y++){
			if(Copy) for(int i=0;i<6;i++) HBuf[i]=ScreenState.Field[i][y];
			for(int x=0;x<CDG_SCREEN_X-6;x++) ScreenState.Field[x][y]=ScreenState.Field[x+6][y];
			for(int i=0;i<6;i++) ScreenState.Field[(CDG_SCREEN_X-6)+i][y]=HBuf[i];
		}
		break;
	}

	switch(SVCmd){
	case 1: // Scroll down
		if(!Copy) for(int i=0;i<12;i++) VBuf[i]=Colour;
		for(int x=0;x<CDG_SCREEN_X;x++){
			if(Copy) for(int i=0;i<12;i++) VBuf[i]=ScreenState.Field[x][(CDG_SCREEN_Y-12)+i];
			for(int y=CDG_SCREEN_Y-13;y>=0;y--) ScreenState.Field[x][y+12]=ScreenState.Field[x][y];
			for(int i=0;i<12;i++) ScreenState.Field[x][i]=VBuf[i];
		}
		break;
	case 2: // Scroll up
		if(!Copy) for(int i=0;i<12;i++) VBuf[i]=Colour;
		for(int x=0;x<CDG_SCREEN_X;x++){
			if(Copy) for(int i=0;i<12;i++) VBuf[i]=ScreenState.Field[x][i];
			for(int y=0;y<CDG_SCREEN_Y-12;y++) ScreenState.Field[x][y]=ScreenState.Field[x][y+12];
			for(int i=0;i<12;i++) ScreenState.Field[x][(CDG_SCREEN_Y-12)+i]=VBuf[i];
		}
		break;
	}
	
	// Dump details
	if(TextOutput) printf("Scroll (%s): Colour %d, HScroll %s %d, VScroll %s %d\n",
		(Copy?"Copy":"Preset"),
		Colour,
		(SHCmd==0?"Don't scroll":(SHCmd==1?"Scroll 6px right":(SHCmd==2?"Scroll 6px left":"Unknown"))),
		HOffset,
		(SVCmd==0?"Don't scroll":(SVCmd==1?"Scroll 12px down":(SVCmd==2?"Scroll 12px up":"Unknown"))),
		VOffset);
	
	return Ok;	
}

bool CDG::ProcessTransparency(CDG_TransColStruct *TransCol)
{
	bool Ok=true;
	unsigned char Colour;

	// Unpack details
	Colour=TransCol->Colour&0x0f;

	// Update screen state
	ScreenState.TransparentColour=Colour;
	
	// Dump details
	if(TextOutput) printf("Transparency colour: Colour %d\n",Colour);

	return Ok;
}

bool CDG::ProcessColourTable(CDG_ColourTableStruct *ColourTable,int Offset)
{
	bool Ok=true;
	int Entry;
	
	struct{
		unsigned char Red;
		unsigned char Green;
		unsigned char Blue;
	} Colours[8];
	
	// Unpack R,G,B
	for(int i=0;i<8;i++){
		Colours[i].Red=(ColourTable->ColourSpec[i].Hi&0x3c)>>2;
		Colours[i].Green=(ColourTable->ColourSpec[i].Hi&0x03)<<2 | (ColourTable->ColourSpec[i].Lo&0x30)>>4;
		Colours[i].Blue=ColourTable->ColourSpec[i].Lo&0x0f;
	}

	// Update screen state
	Entry=Offset*8;
	for(int i=0;i<8;i++){
		ScreenState.ColourTable[Entry].Red=Colours[i].Red<<4;
		ScreenState.ColourTable[Entry].Green=Colours[i].Green<<4;
		ScreenState.ColourTable[Entry].Blue=Colours[i].Blue<<4;
		++Entry;
	}	

	// Dump details
	if(TextOutput){
		printf("Colour table %d-%d:",Offset*8,(Offset*8)+7);
		for(int i=0;i<8;i++){
			printf(" [%01x,%01x,%01x]",Colours[i].Red,Colours[i].Green,Colours[i].Blue);
		}
		printf("\n");
	}

	return Ok;
}

void CDG::RenderField()
{
	CDG_Colour *Colour;
	
	// Draw borders
	Colour=&ScreenState.ColourTable[ScreenState.BorderColour];
	for(int i=0;i<CDG_HBORDER;i++){
		for(int y=0;y<CDG_SCREEN_Y;y++){
			Pixel(i,y,Colour->Red,Colour->Green,Colour->Blue);
			Pixel((CDG_SCREEN_X-1)-i,y,Colour->Red,Colour->Green,Colour->Blue);
		}
	}
	for(int i=0;i<CDG_VBORDER;i++){
		for(int x=0;x<CDG_SCREEN_X;x++){
			Pixel(x,i,Colour->Red,Colour->Green,Colour->Blue);
			Pixel(x,(CDG_SCREEN_Y-1)-i,Colour->Red,Colour->Green,Colour->Blue);
		}
	}

	// Draw pixels	
	for(int y=0;y<CDG_SCREEN_USEY;y++){
		for(int x=0;x<CDG_SCREEN_USEX;x++){
			Colour=&ScreenState.ColourTable[ScreenState.Field[x+CDG_HBORDER+ScreenState.HOffset][y+CDG_VBORDER+ScreenState.VOffset]];
			Pixel(x+CDG_HBORDER,y+CDG_VBORDER,Colour->Red,Colour->Green,Colour->Blue);
		}		
	}

	// Display the frame
	FlipSDL();
}


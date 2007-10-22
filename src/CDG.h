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

typedef struct{
	unsigned char Command;
	unsigned char Instruction;
	unsigned char ParityQ[2];
	unsigned char Data[16];
	unsigned char ParityP[4];
} CDG_PacketStruct;

typedef struct {
	unsigned char Colour;			// Mask with 0x0F
	unsigned char Repeat;			// Mask with 0x0F
} CDG_MemPresetStruct;

typedef struct {
	unsigned char Colour;			// Mask with 0x0F
} CDG_BorderPresetStruct;

typedef struct {
	unsigned char Colour0;			// Mask with 0x0F
	unsigned char Colour1;			// Mask with 0x0F
	unsigned char Row;				// Mask with 0x1F
	unsigned char Column;			// Mask with 0x3F
	unsigned char TilePixels[12];	// Mask each with 0x3f
} CDG_TileBlkStruct;

typedef struct {
	unsigned char Colour;			// Mask with 0x0F
	unsigned char HScroll;			// Mask with 0x3F
	unsigned char VScroll;			// Mask with 0x3F
} CDG_ScrollStruct;

typedef struct {
	unsigned char Colour;			// Mask with 0x0F
} CDG_TransColStruct; // TODO This structure is a guess

typedef struct {
	struct{
		unsigned char Hi;			// Mask with 0x3f
		unsigned char Lo;			// Mask with 0x3f
	} ColourSpec[8];
} CDG_ColourTableStruct;

#define CDG_COLS 50
#define CDG_ROWS 18

#define CDG_COL_PX 6
#define CDG_ROW_PX 12

#define CDG_SCREEN_X (CDG_COLS*CDG_COL_PX)
#define CDG_SCREEN_Y (CDG_ROWS*CDG_ROW_PX)

#define CDG_VBORDER CDG_ROW_PX
#define CDG_HBORDER CDG_COL_PX

#define CDG_SCREEN_USEX (CDG_SCREEN_X-(2*CDG_HBORDER))
#define CDG_SCREEN_USEY (CDG_SCREEN_Y-(2*CDG_VBORDER))

typedef struct
{
	unsigned char Red;
	unsigned char Green;
	unsigned char Blue;
} CDG_Colour;

typedef struct
{
	CDG_Colour ColourTable[16];
	unsigned char BorderColour;
	unsigned char Field[CDG_SCREEN_X][CDG_SCREEN_Y]; // full field 300x216    NTSC 720x480, PAL 720x576
	char TransparentColour;
	unsigned char HOffset;
	unsigned char VOffset;
} CDG_StateStruct;

class CDG
{
private:
	int Handle;
	CDG_StateStruct ScreenState;
	void PresetScreen(unsigned char Colour);
	
	bool ProcessMemoryPreset(CDG_MemPresetStruct *MemPreset);
	bool ProcessBorderPreset(CDG_BorderPresetStruct *BorderPreset);
	bool ProcessTileBlk(CDG_TileBlkStruct *TileBlk,bool Xor);
	bool ProcessScroll(CDG_ScrollStruct *Scroll,bool Copy);
	bool ProcessTransparency(CDG_TransColStruct *TransCol);
	bool ProcessColourTable(CDG_ColourTableStruct *ColourTable,int Offset);
	void RenderField();
public:
	CDG(int Handle);
	bool Process();
};


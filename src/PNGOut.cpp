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

#include <png.h>
#include <stdlib.h>
#include <limits.h>

#include "CDG.h"

extern bool OutDirSpecified;
extern char OutDir[PATH_MAX];

FILE *PngFrameList=NULL;

png_color LastPalette[16];
unsigned char LastLines[CDG_SCREEN_Y][CDG_SCREEN_X];
char LastFile[32];


bool PngInitialise()
{
	bool Ok=false;
	char Name[PATH_MAX+32];
	
	do{
		if(OutDirSpecified) sprintf(Name,"%s/",OutDir);
		else Name[0]='\x0';
		sprintf(Name,"%sFrameList.txt",Name);
			
		PngFrameList=fopen(Name,"wt");
    	if(!PngFrameList){
    		printf("Error opening file %s\n",Name);
    		break;
    	}
    	
    	Ok=true;
	} while(0);
	
	return Ok;
}

void PngCleanup()
{
	if(PngFrameList) fclose(PngFrameList);
	PngFrameList=NULL;
}

/*#define PIXEL(x,y,c) \
{\
	int xb=(x)>>1; \
	\
	switch((x)%2){ \
	case 0: \
		Lines[y][xb]=(Lines[y][xb]&0x0f) | (((c)&0x0f)<<4); \
		break; \
	case 1: \
		Lines[y][xb]=(Lines[y][xb]&0xf0) | ((c)&0x0f); \
		break; \
	} \
}*/

#define PIXEL(x,y,c) Lines[y][x]=(c)

bool WritePng(CDG_StateStruct *ScreenState,unsigned int PacketNo)
{
	bool Ok=false;
	char File[32];
	char Name[PATH_MAX+32];
	FILE *Png=NULL;
	png_structp PngPtr=NULL;
	png_infop PngInfoPtr=NULL;
	png_color Palette[16];
	png_bytep LinePtrs[CDG_SCREEN_Y];
	unsigned char Lines[CDG_SCREEN_Y][CDG_SCREEN_X];
	
	do{
		// Set up PNG palette
		for(int i=0;i<16;i++){
			Palette[i].red=ScreenState->ColourTable[i].Red;
			Palette[i].green=ScreenState->ColourTable[i].Green;
			Palette[i].blue=ScreenState->ColourTable[i].Blue;
		}
	
		// Set up line pointers
		for(int y=0;y<CDG_SCREEN_Y;y++){
			LinePtrs[y]=&Lines[y][0];
		}
		
		// Draw borders
		for(int i=0;i<CDG_HBORDER;i++){
			for(int y=0;y<CDG_SCREEN_Y;y++){
				PIXEL(i,y,ScreenState->BorderColour);
				PIXEL((CDG_SCREEN_X-1)-i,y,ScreenState->BorderColour);
			}
		}
		for(int i=0;i<CDG_VBORDER;i++){
			for(int x=0;x<CDG_SCREEN_X;x++){
				PIXEL(x,i,ScreenState->BorderColour);
				PIXEL(x,(CDG_SCREEN_Y-1)-i,ScreenState->BorderColour);
			}
		}

		// Draw pixels
		for(int y=0;y<CDG_SCREEN_USEY;y++){
			for(int x=0;x<CDG_SCREEN_USEX;x++){
				PIXEL(x+CDG_HBORDER,y+CDG_VBORDER,ScreenState->Field[x+CDG_HBORDER+ScreenState->HOffset][y+CDG_VBORDER+ScreenState->VOffset]);
			}
		}
		
		// Has frame changed?
		if(PacketNo==0 || memcmp(LastPalette,Palette,16*sizeof(png_color))!=0 || memcmp(LastLines,Lines,CDG_SCREEN_Y*(CDG_SCREEN_X))!=0){
			memcpy(LastPalette,Palette,16*sizeof(png_color));
			memcpy(LastLines,Lines,CDG_SCREEN_Y*(CDG_SCREEN_X));
			
			sprintf(File,"Frame%08d-%04d-%02d.png",PacketNo,PacketNo/300,(PacketNo%300)/12);
			if(OutDirSpecified) sprintf(Name,"%s/%s",OutDir,File);
			else sprintf(Name,"%s",File);
			
			Png=fopen(Name,"wb");
	    	if(!Png){
	    		printf("Error opening file %s\n",Name);
	    		break;
	    	}
	    	
	    	PngPtr=png_create_write_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL); // TODO replace NULLs
	    	if(!PngPtr){
	    		printf("Error creating PNG write structure\n");
	    		break;
	    	}
	    	
	    	PngInfoPtr=png_create_info_struct(PngPtr);
	    	if(!PngInfoPtr){
	    		printf("Error creating PNG info structure\n");
	    		break;
	    	}
	    	
	    	if(setjmp(png_jmpbuf(PngPtr))){
	    		printf("Error writing PNG\n");
	    		break;
	    	}
	    	
	    	png_init_io(PngPtr,Png);
	    	
			png_set_IHDR(PngPtr,PngInfoPtr,CDG_SCREEN_X,CDG_SCREEN_Y,8,PNG_COLOR_TYPE_PALETTE,PNG_INTERLACE_NONE,PNG_COMPRESSION_TYPE_DEFAULT,PNG_FILTER_TYPE_DEFAULT);   	
			
			png_set_PLTE(PngPtr,PngInfoPtr,Palette,16);
			
			png_set_rows(PngPtr,PngInfoPtr,LinePtrs);
			
			png_write_png(PngPtr,PngInfoPtr,PNG_TRANSFORM_IDENTITY,NULL);
			
			strcpy(LastFile,File);
		}
		else{
			strcpy(File,LastFile);
		}
		
		fprintf(PngFrameList,"%s\n",File);
		
		Ok=true;
	} while (0);
	
	if(PngPtr){
		if(PngInfoPtr) png_destroy_write_struct(&PngPtr,&PngInfoPtr);
		else png_destroy_write_struct(&PngPtr,NULL);
	}
	if(Png) fclose(Png);
		
	return Ok;
}


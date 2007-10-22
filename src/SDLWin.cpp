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

#include <SDL.h>
#include <SDL_thread.h>

#include <assert.h>

SDL_Surface *Screen=NULL;
const SDL_VideoInfo *VideoInfo;
bool Locked=false;
Uint32 StartTicks;

bool InitialiseSDL()
{
	bool Ok=false;

	do{
		// Initialise SDL
		if(SDL_Init(SDL_INIT_VIDEO) < 0){
			printf("Unable to init SDL\n");
			break;
		}

		// Set window caption
		SDL_WM_SetCaption("CDG Dump",NULL);

		// Get video information
		VideoInfo=SDL_GetVideoInfo();

		// Create the 'screen'
		Screen = SDL_SetVideoMode(300,216,0,SDL_HWSURFACE|SDL_ANYFORMAT); // SDL_DOUBLEBUF
		if(!Screen){
			printf("Unable to initialise the screen\n");
			break;
		}

		// Finished
		Ok=true;
	} while(0);

	return Ok;
}

void CleanupSDL()
{
	// Quit SDL
	SDL_Quit();
}

void Pixel(int x,int y,int r,int g,int b)
{
	Uint32 Colour;
	
	// Build colour
	Colour=SDL_MapRGB(Screen->format,r,g,b);

	if(!Locked){
		// Lock screen
		if(SDL_MUSTLOCK(Screen)){
			assert(SDL_LockSurface(Screen)>=0);
		}
		Locked=true;
	}
	
	switch(Screen->format->BytesPerPixel) {
	case 1: /* Assuming 8-bpp */
		{
			Uint8 *BufPtr;
			
			BufPtr=(Uint8*) Screen->pixels + (y * Screen->pitch) + x;
			*BufPtr=(Uint8) Colour;
		}
		break;
	case 2: /* Probably 15-bpp or 16-bpp */
		{
			Uint16 *BufPtr;

			BufPtr=(Uint16*) Screen->pixels + (y * (Screen->pitch / 2)) + x;
			*BufPtr=(Uint16) Colour;
		}
		break;
    case 3: /* Slow 24-bpp mode, usually not used */
		{
			Uint8 *BufPtr;
			Uint8 *ColPtr;

			BufPtr=(Uint8*) Screen->pixels + (y * Screen->pitch) + (x * 3);
			ColPtr=(Uint8*) &Colour;
			++ColPtr;
			*BufPtr=*ColPtr;
			++BufPtr;
			*BufPtr=*ColPtr;
			++BufPtr;
			*BufPtr=*ColPtr;
		}
        break;
    case 4: /* Probably 32-bpp */
		{
			Uint32 *BufPtr;

			BufPtr=(Uint32*) Screen->pixels + (y * (Screen->pitch / 4)) + x;
			*BufPtr=Colour;
		}
		break;
	}

}

void FlipSDL()
{
	if(Locked){
		// Unlock screen
		if(SDL_MUSTLOCK(Screen)){
			SDL_UnlockSurface(Screen);
		}
		Locked=false;
	}

	// Display
	SDL_Flip(Screen);
}

void StartTimerSDL()
{	
	StartTicks=SDL_GetTicks();
}

void DelayUntilSDL(unsigned int Ms)
{
	Uint32 Ticks;
	Uint32 ElapsedTicks;
	
	Ticks=SDL_GetTicks();
	ElapsedTicks=Ticks-StartTicks;
	if(ElapsedTicks<Ms){
		SDL_Delay(Ms-ElapsedTicks);
	}
}

bool EventPollSDL()
{
	bool Ok=true;
	SDL_Event Event;

	while(SDL_PollEvent(&Event)){
		switch(Event.type){
		case SDL_QUIT:
			Ok=false;
			break;
		}
	}
	
	return Ok;
}



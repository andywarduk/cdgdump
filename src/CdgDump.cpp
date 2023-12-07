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

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

#include "SDLWin.h"
#include "CDG.h"

bool Debug=false;
bool TextOutput=false;
bool VideoOutput=false;
bool RealTime=false;
bool PngOutput=false;
bool OutDirSpecified=false;
char OutDir[PATH_MAX];

bool ProcessFile(char *Path);
bool ProcessOutDir(char *Path);

int main(int ArgC,char **ArgV)
{
	bool Ok=true;
	int OptChar;

	// Parse command line
	while(Ok){
		OptChar=getopt(ArgC,ArgV,":tvdrpo:");
		if(OptChar==-1) break;
		
		switch(OptChar){
		case 't':
			TextOutput=true;
			break;
		case 'v':
			VideoOutput=true;
			break;
		case 'p':
			PngOutput=true;
			break;
		case 'd':
			Debug=true;
			break;
		case 'r':
			RealTime=true;
			break;
		case 'o':
			OutDirSpecified=true;
			if(!ProcessOutDir(optarg)){
				Ok=false;
			}
			break;
		default:
			Ok=false;
			break;
		}
	}
	
	if(!Ok || optind>=ArgC){
		printf("Usage: cgddump [-t] [-v] [-p] [-d] [-r] [-o dir] file [file...]\n"
		       "   where: -t        Produce text output\n"
		       "          -v        Produce video output\n"
		       "          -p        Produce PNG output with frame list @ 25fps\n"
		       "          -d        Produce extra debug messages\n"
		       "          -r        Process CDG packets in real time\n"
		       "          -o <dir>  Specifies output directory\n");
		return 1;		
	}

	if(VideoOutput){
		// Initialise SDL
		InitialiseSDL();
	}

	while(optind<ArgC){
		if(!ProcessFile(ArgV[optind])){
			Ok=false;
			break;
		}
		++optind;
	}
	
	if(VideoOutput){
		// Clean up SDL
		CleanupSDL();
	}

	return (Ok?0:2);
}

bool ProcessOutDir(char *Path)
{
	bool Ok=false;
	struct stat FileStat;
	
	do{
		if(strlen(Path)>PATH_MAX-1){
			printf("Output directory specified is too long\n");
			Ok=false;
			break;
		}
		
		strcpy(OutDir,optarg);
		if(stat(OutDir,&FileStat)==0){
			if(!S_ISDIR(FileStat.st_mode)){
				printf("Output directory specified is not a directory\n");
				Ok=false;
				break;
			}
			// TODO check writable?
		}
		else{
			if(mkdir(OutDir,0777)!=0){
				printf("Output directory could not be created\n");
				Ok=false;
				break;
			}
		}
		
		Ok=true;
	} while(0);
	
	return Ok;
}

bool ProcessFile(char *Path)
{
	bool Ok=false;
	int Handle;
	CDG *CDGObj;
	
	Handle=open(Path,O_RDONLY);
	if(Handle==-1){
		printf("Error opening file %s\n",Path);
	}
	else{
		printf("Processing file %s...\n",Path);
		CDGObj=new CDG(Handle);
		Ok=CDGObj->Process();
		delete CDGObj;
	}
	
	return Ok;
}


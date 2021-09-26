#include "main.h"
#include "battle_scene.h"
#include "raw_input.h"
#include "window.h"
#include "game_state.h"
#include "raw_input.h"
#include "netplay.h"
#include <enet/enet.h>
#include <fstream>
#include <SDL_gamecontroller.h>

int gameState = GS_MENU;

int main(int argc, char** argv)
{
	mainWindow = new Window();
	//This should be in raw input. Should it?
	std::ifstream keyfile("keyconf.bin", std::ifstream::in | std::ifstream::binary);
	if(keyfile.is_open())
	{
		keyfile.read((char*)modifiableSCKeys, sizeof(modifiableSCKeys));
		keyfile.close();
	}
	
 	keyfile.open("joyconf.bin", std::ifstream::in | std::ifstream::binary);
	if(keyfile.is_open())
	{
		keyfile.read((char*)modifiableJoyKeys, sizeof(modifiableJoyKeys));
		keyfile.close();
	}
	else
		memset(modifiableJoyKeys, -1, sizeof(modifiableJoyKeys));

	std::vector<SDL_GameController*> controllers;
	InitControllers(controllers);

	bool playDemo = false;
	int netState = 0;
	ENetHost *local;
	
	if(argc > 1)
	{
		if(strcmp(argv[1],"playdemo")==0)
			playDemo = true;
		else
			netState = net::NetplayArgs(argc, argv, local);
		if(netState < net::Success)
		{
			if(netState)
				enet_deinitialize();
			return 0;
		}		
	}

	
	while(!mainWindow->wantsToClose)
	{
		try{
			switch (gameState)
			{
				case GS_MENU:
				//break; Not implemented so have a fallthru.
				case GS_CHARSELECT:
				//break;
				case GS_WIN:
				//break;
				case GS_PLAY: {
					BattleScene bs;
					gameState = bs.PlayLoop(playDemo);
					break;
				}
			}
		}
		catch(std::exception e)
		{
			std::cerr << e.what() << " The program can't continue. Press enter to quit.";
			std::cin.get();
			return 0;
		}
	}

	for(auto &control : controllers)
	{
		if(control)
			SDL_GameControllerClose(control);
	}

	if(netState)
		enet_deinitialize();

	return 0;
}


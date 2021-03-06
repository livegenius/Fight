#include "audio.h"
#include "battle_scene.h"
#include "raw_input.h"
#include "window.h"
#include "game_state.h"

#include "netplay.h"
#include <enet/enet.h>
#include <fstream>
#include <SDL_gamecontroller.h>

int gameState = GS_MENU;

int main(int argc, char** argv)
{
	int aiMatch = false;
	bool playDemo = false;
	int netState = 0;
	ENetHost *local = nullptr;
	std::string address;
	
	if(argc > 1)
	{
		if(strcmp(argv[1],"playdemo")==0)
			playDemo = true;
		else if(strcmp(argv[1],"vsai")==0)
			aiMatch = 1;
		else if(strcmp(argv[1],"aionly")==0)
			aiMatch = 2;
		else
		{
			netState = net::NetplayArgs(argc, argv, local, address);
			if(netState < net::Success)
			{
				std::cerr << "No dice.";
				if(netState == net::FailedNeedsCleanup)
					enet_deinitialize();
				return 0;
			}
		}
	}

	mainWindow = new Window();
	soloud = new SoLoud::Soloud;
	soloud->init();
	
	//TODO: Move
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
	

	while(!mainWindow->wantsToClose)
	{
		#ifdef NDEBUG
		try{
		#endif
			switch (gameState)
			{
				case GS_MENU:
				//break; Not implemented so have a fallthru.
				case GS_CHARSELECT:
				//break;
				case GS_WIN:
				//break;
				case GS_PLAY: {
					int playerId = 0;
					if(netState == net::Joining)
						playerId = 1;
					BattleScene bs(local);
					gameState = bs.PlayLoop(playDemo, aiMatch, playerId, address);
					break;
				}
			}
		#ifdef NDEBUG
		}
		catch(std::exception e)
		{
			std::cerr << e.what() << " The program can't continue. Press enter to quit.";
			std::cin.get();
			return 0;
		}
		#endif
	}

	for(auto &control : controllers)
	{
		if(control)
			SDL_GameControllerClose(control);
	}

	if(netState)
		enet_deinitialize();
	soloud->deinit();
	delete soloud;
	delete mainWindow;

	return 0;
}


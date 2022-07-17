#include "battle_scene.h"
#include "chara.h"
#include "util.h"
#include "raw_input.h"
#include "window.h"

#include "game_state.h"
#include "stage.h"
#include <gfx_handler.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <ggponet.h>

#include <glm/gtc/type_ptr.hpp> //Just for glm::ortho.
#include "audio.h"

int inputDelay = 0;

BattleScene::BattleScene(ENetHost *local):
sfx(gameTicks),
local(local),
particles(rng),
interface{rng, particles, view, sfx},
player(interface), player2(interface),
hr(mainWindow->renderer)
{
	projection = glm::ortho<float>(0, internalWidth, internalHeight, 0, -32768, 32767);
	//Zoomed out scene
	//projection = glm::ortho<float>(-internalWidth*0.5, internalWidth*1.5, internalHeight*1.5, -internalHeight*0.5, -32768, 32767);
	//projection[1][1] = -projection[1][1]; //Perspective only.
}

BattleScene::~BattleScene()
{
	enet_host_destroy(local);
}


int BattleScene::PlayLoop(bool replay, int matchType, int playerId, const std::string &address)
{
	float clearColor[] = {1,1,1,1};
	mainWindow->renderer.SetClearColor(clearColor);

	SoLoud::Wav music;
	
	size_t inputSize;
	if(replay)
	{
		std::ifstream replayFile("replay", std::ios_base::binary);
		if(!replayFile.is_open())
		{
			std::cout << "There's no replay file.";
			mainWindow->wantsToClose = true;
			return 0;
		}
		{
			replayFile.read((char*)&inputSize, sizeof(size_t));
			for(int i = 0; i < playersN; ++i)
			{	
				inputs[i].buffer.resize(inputSize);
				replayFile.read((char*)inputs[i].buffer.data(), sizeof(uint32_t)*inputSize);
			}
		}
	}
	else for (int i = 0; i < playersN; ++i)
		inputs[i].buffer.reserve(0x8000); //About 9 minutes of gameplay.
	
	std::ostringstream timerString;
	timerString.precision(6);
	timerString.setf(std::ios::fixed, std::ios::floatfield);

	Hud hud(&mainWindow->renderer);
	
	hud.Load("data/hud/hud.lua");
	hud.SetMatrix(projection);

	bool p1ai = false, p2ai = false;
	if(matchType >= 1)
		p2ai = true;
	if (matchType == 2)
		p1ai = true;	
		
	player.Load(1, "data/char/vaki/vaki.fdat", 0, p1ai);
	player2.Load(-1, "data/char/vaki/vaki.fdat", 1, p2ai);
	
	sfx.LoadFromDef("data/sfx/sfx.lua");
	
	GfxHandler gfx(&mainWindow->renderer);
	gfx.LoadGfxFromDef("data/char/vaki/def.lua");
	std::string stageLuaFile("data/stage/");
	
	{//TODO: music shit
		sol::state lua;
		lua.script_file("data/stage/stages.lua");
		sol::table selectedStage = lua["stageList"]["testStage"];
		stageLuaFile.append(selectedStage["lua"]);

		lua.script_file("data/bgm/bgm.lua");
		sol::table bgmEntry = lua["bgm"][selectedStage["bgm"]];
		std::string bgmFile = "data/bgm/";
		bgmFile.append(bgmEntry["file"]);
		
		music.setLooping(true);
		music.setLoopPoint(bgmEntry["loop"].get_or(0.0));
		music.load(bgmFile.c_str());
		auto h = soloud->play(music);
		soloud->setProtectVoice(h, true);
	}

	Stage stage(gfx, stageLuaFile);
	gfx.LoadingDone();

	player.SetTarget(player2);
	player2.SetTarget(player);
	player.priority = 1;

	players[0] = &player;
	players[1] = &player2;

	//vaoTexOnly.Bind();
	

	auto keyHandler = std::bind(&BattleScene::KeyHandle, this, std::placeholders::_1);
	bool drawn = false;

	if(local)
	{
		if(!SetupGgpo(playerId, address))
		{
			mainWindow->wantsToClose = true;
			std::cerr << "Error setting ggpo up";
			return 0;
		}
	}
	
	while(!mainWindow->wantsToClose)
	{
		EventLoop(keyHandler, pause);
		if(pause){
			if(!step)
				continue;
			else
				step = false;
		}
		
		if(replay)
		{
			if(gameTicks >= inputSize)
			{
				mainWindow->wantsToClose = true;
				break;
			}
			AdvanceFrame();
		}
		else if(ggpo)
		{
			ggpo_idle(ggpo, 0);
			unsigned int ginputs[2];
			//Grab p1 controller only. 
			auto result = ggpo_add_local_input(ggpo, playerHandle[playerId], &keySend[0], sizeof(unsigned int));
			if (GGPO_SUCCEEDED(result))
			{
				result = ggpo_synchronize_input(ggpo, (void *)ginputs, sizeof(unsigned int) * 2, nullptr);
				inputs[0].buffer.push_back(ginputs[0]);
				inputs[1].buffer.push_back(ginputs[1]);
				AdvanceFrame();
				drawn = false;
			}
			if(drawn)
				continue;
			else
				drawn = true;
		}
		else
		{
			inputs[0].buffer.push_back(keySend[0]);
			inputs[1].buffer.push_back(keySend[1]);
			AdvanceFrame();
		}
		
		//Start rendering
		mainWindow->renderer.Acquire(); //Prepare for rendering. Must be here because the window may get resized and it requires a call to end drawing.
		
		drawList.Init(player, player2);
		
		if(gfx.Begin())
		{
			//Draw stage
			auto center = view.GetCameraCenterScale();
			stage.Draw(projection*viewMatrix, center);

			int p1Pos = players[1]->FillDrawList(drawList);
			int p2Pos = players[0]->FillDrawList(drawList);

			auto draw = [this,&gfx](Actor *actor, glm::mat4 &viewMatrix)
			{
				gfx.SetMatrix(projection*viewMatrix*actor->GetSpriteTransform());
				auto options = actor->GetRenderOptions();
				gfx.SetBlendingMode(options.blendingMode);
				
				if(options.paletteIndex == 0)
					gfx.SetPaletteSlot(0);
				else
					gfx.SetPaletteSlot(3);
				gfx.Draw(actor->GetSpriteIndex(), 0);
			};

			auto invertedView = glm::translate(glm::scale(viewMatrix, glm::vec3(1,-1,1)), glm::vec3(0,-64,0));

			//Draw player reflection?

			gfx.SetMulColor(1, 1, 1, 0.2);
			draw(drawList.v[p1Pos], invertedView);
			draw(drawList.v[p2Pos], invertedView);
			gfx.SetMulColorRaw(1,1,1,1);
		
			//Draw all actors
			for(auto actor : drawList.v)
			{
				draw(actor,viewMatrix);
			}


			gfx.SetMatrix(projection*viewMatrix);
			gfx.DrawParticles(particles);
		}

		//Draw boxes
		if(drawBoxes)
		{
			hr.LoadHitboxVertices();
			hr.Draw(projection*viewMatrix);
		}
				
		//Draw HUD
		//Guard bar
		hud.ResizeBarId(0, player.GetHealthRatio());
		hud.ResizeBarId(1, player2.GetHealthRatio());
		//Health bars
		hud.ResizeBarId(2, player.GetHealthRatio());
		hud.ResizeBarId(3, player2.GetHealthRatio());
		hud.Draw();

 		//TODO: Goes in HUD. Draw fps bar
/* 		timerString.seekp(0);
		timerString << "SFP: " << mainWindow->GetSpf() << " FPS: " << 1/mainWindow->GetSpf()<<"      Entities:"<<drawList.v.size()<<
			"   Particles:"<<particles.particles.size()<<"  ";

		glBindTexture(GL_TEXTURE_2D, activeTextures[0].id);
		int count = DrawText(timerString.str(), textVertData, 2, 10);
		vaoTexOnly.UpdateBuffer(textId, textVertData.data());
		vaoTexOnly.Draw(textId); */

		//End drawing.
		mainWindow->SwapBuffers();
		mainWindow->SleepUntilNextFrame();
	}

	if(!replay)
	{
		assert(inputs[0].buffer.size() == inputs[1].buffer.size() && inputs[0].buffer.size() == gameTicks);
		size_t inputSize = inputs[0].buffer.size();
		std::ofstream replayFile("replay", std::ios_base::binary);
		replayFile.write((char*)&inputSize, sizeof(size_t));
		for(int i = 0; i < playersN; ++i)
			replayFile.write((char*)inputs[i].buffer.data(), sizeof(uint32_t)*inputSize);
	}

	return GS_WIN;
}

void BattleScene::AdvanceFrame()
{
	if(player.priority >= player2.priority){
		players[0] = &player;
		players[1] = &player2;
	}else{
		players[0] = &player2;
		players[1] = &player;
	}

	Player::HitCollision(player, player2);

	for(int i = 0; i < 2; ++i)
		inputs[i].lastLoc = gameTicks;
	player.ProcessInput(inputs[0]);
	player2.ProcessInput(inputs[1]);

	if(drawBoxes)
	{
		players[0]->Update(&hr);
		players[1]->Update(&hr);
	}
	else
	{
		players[0]->Update(nullptr);
		players[1]->Update(nullptr);
	}
	
	Player::Collision(player, player2);
	viewMatrix = view.Calculate(player.GetXYCoords(), player2.GetXYCoords());

	particles.Update();

	ggpo_advance_frame(ggpo);

	++gameTicks;
}

void BattleScene::SaveState(State &state)
{
	//TODO Add inputIterator to state
	state.gameTicks = gameTicks;
	state.lastInputSize = inputs[0].buffer.size();
	state.rng = rng;
	state.particles = particles;
	state.p1 = player.GetStateCopy();
	state.p2 = player2.GetStateCopy();
	state.view = view;
}

void BattleScene::LoadState(State &state)
{	
	if(!state.p1.charObj) //There are no saves.
		return;
	for(int i = 0; i < playersN; ++i)
		inputs[i].buffer.resize(state.lastInputSize);
	rng = state.rng;
	particles = state.particles;
	player.SetState(state.p1);
	player2.SetState(state.p2);
	view = state.view;
	gameTicks = state.gameTicks;
}

bool BattleScene::KeyHandle(const SDL_KeyboardEvent &e)
{
	if(e.type != SDL_KEYDOWN)
		return false;

	switch (e.keysym.scancode){
 	case SDL_SCANCODE_F1: 
	 	if(!ggpo)
			SaveState(savedState);
		break;
	case SDL_SCANCODE_F2:
		if(!ggpo)
			LoadState(savedState);
		break;
	case SDL_SCANCODE_H:
		drawBoxes = !drawBoxes;
		break;
	case SDL_SCANCODE_P:
		pause = !pause;
		break;
	case SDL_SCANCODE_SPACE:
		if(pause)
			step = true;
		break;
	default:
		return false;
	}
	return true;
}

bool BattleScene::SetupGgpo(int playerId, const std::string &address)
{
	ready = false;
	GGPOSessionCallbacks cb = { 0 };
	cb.begin_game      = [](const char*){return true;};
	cb.advance_frame   = [this](int)->bool{ //Rollback only advance.
		unsigned int ginputs[2];
		ggpo_synchronize_input(ggpo, (void *)ginputs, sizeof(unsigned int) * 2, nullptr);
		inputs[0].buffer.push_back(ginputs[0]);
		inputs[1].buffer.push_back(ginputs[1]);
		AdvanceFrame();
		return true;
	};
	cb.load_game_state = [this](unsigned char *buffer, int){
		State *state = (State *)buffer;
		LoadState(*state);
		return true;
	};
	cb.save_game_state = [this](unsigned char **buffer, int* len, int *checksum, int){
		auto state = new State();
		SaveState(*state);
		*len = sizeof(State); 
		*buffer = (unsigned char*)state;
		*checksum = 1;
		return true;
	};
	cb.free_buffer     = [](void *buffer){State *state = (State *)buffer; delete state;};
	cb.log_game_state  = [](char *, unsigned char *, int){return false;}; 
	cb.on_event        = [this](GGPOEvent *info)
	{
		int progress;
		switch (info->code) {
		case GGPO_EVENTCODE_CONNECTED_TO_PEER:
		case GGPO_EVENTCODE_SYNCHRONIZING_WITH_PEER:
		case GGPO_EVENTCODE_SYNCHRONIZED_WITH_PEER:
			break; //Don't care.
		case GGPO_EVENTCODE_RUNNING:
			ready = true;
			break;
		case GGPO_EVENTCODE_CONNECTION_INTERRUPTED:
			std::cout<<info->u.connection_interrupted.player << ": "<<info->u.connection_interrupted.disconnect_timeout<<"\n";
			break;
		case GGPO_EVENTCODE_CONNECTION_RESUMED:
			std::cout<<info->u.connection_resumed.player << ": resumed\n";
			break;
		case GGPO_EVENTCODE_DISCONNECTED_FROM_PEER:
			std::cout<<info->u.disconnected.player<<": disconnected\n";
			mainWindow->wantsToClose = true;
			break;
		case GGPO_EVENTCODE_TIMESYNC:
			for(int i = 0; i < info->u.timesync.frames_ahead; ++i)
				mainWindow->SleepUntilNextFrame();
			break;
		}
		return true;
	};
	if(ggpo_start_session(&ggpo, &cb, nullptr, 2, sizeof(uint32_t), local->socket) != GGPO_OK)
	{
		return false;
	}
	GGPOPlayer ggplayer[2];
	for (int i = 0; i < 2; i++) {
		auto &p = ggplayer[i];
		auto &h = playerHandle[i];
		p.size = sizeof(GGPOPlayer);
		p.player_num = i + 1;
		if(i == playerId)
		{
			p.type = GGPO_PLAYERTYPE_LOCAL;
			ggpo_set_frame_delay(ggpo, h, 1);
		}
		else
		{
			p.type = GGPO_PLAYERTYPE_REMOTE;
			enet_address_get_host_ip(&local->peers[0].address, p.u.remote.ip_address, 32);
			p.u.remote.port = local->peers[0].address.port;
			std::cout << "Remote port :"<< p.u.remote.port <<"\n";
			std::cout << "Remote address :"<< p.u.remote.ip_address <<"\n";
		}
		ggpo_add_player(ggpo, &p, &h);
		
	}
	
	ggpo_set_disconnect_timeout(ggpo, 3000);
	ggpo_set_disconnect_notify_start(ggpo, 1000);
	return true;
}
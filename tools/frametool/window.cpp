#include "window.h"
#include "ini.h"
#include <vk/renderer.h>

#include <SDL.h>

#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_sdl.h>

#include <assert.h>
#include <iostream>
#include <chrono>
#include <thread>

Window *mainWindow = nullptr;

Window::Window() :
window(nullptr),
frameRateChoice(0),
targetSpf(0.01600),
realSpf(0),
startClock(std::chrono::high_resolution_clock::now())
{
	if(SDL_Init(SDL_INIT_VIDEO))
	{
		std::cerr << SDL_GetError();
		throw std::runtime_error("Couldn't init SDL.");
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.IniFilename = "frametool.ini";
	InitIni();

	window = SDL_CreateWindow(
		"Frametool",
		gSettings.posX, gSettings.posY, gSettings.winSizeX, gSettings.winSizeY,
		SDL_WINDOW_VULKAN|SDL_WINDOW_RESIZABLE|SDL_WINDOW_ALLOW_HIGHDPI | (gSettings.maximized ? SDL_WINDOW_MAXIMIZED : 0));

	if(!window)
	{
		std::cerr << SDL_GetError();
		SDL_Quit();
		throw std::runtime_error("Couldn't create window.");
	}

	renderer = new Renderer();
	renderer->Init(window, true);

	ImGui_ImplSDL2_InitForVulkan(window);
	auto info = renderer->GetImguiInfo();
	static_assert(sizeof(ImGui_ImplVulkan_InitInfo) == sizeof(info));
	ImGui_ImplVulkan_Init((ImGui_ImplVulkan_InitInfo*)&info, renderer->GetRenderPass());

	renderer->ExecuteCommand([](vk::CommandBuffer cmd){
		ImGui_ImplVulkan_CreateFontsTexture(cmd);
	});
	ImGui_ImplVulkan_DestroyFontUploadObjects();

	mf.reset(new MainFrame(renderer));
	UpdateClientRect();
}

Window::~Window()
{
	renderer->Wait();
	mf.reset();
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	delete renderer;
	SDL_DestroyWindow(window);
	SDL_Quit();
}

bool Window::PollEvents()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		ImGui_ImplSDL2_ProcessEvent(&event);
		renderer->HandleEvents(event);
		
		switch(event.type)
		{
			case SDL_QUIT:
				return false;
			case SDL_WINDOWEVENT:
				switch(event.window.event)
				{
					case SDL_WINDOWEVENT_SIZE_CHANGED:
						mf->SetClientRect(event.window.data1, event.window.data2);
						gSettings.winSizeX = event.window.data1;
						gSettings.winSizeY = event.window.data2;
						break;
					case SDL_WINDOWEVENT_MOVED:
						gSettings.posX = event.window.data1;
						gSettings.posY = event.window.data2;
						break;
					case SDL_WINDOWEVENT_MAXIMIZED:
						gSettings.maximized = 1;
						break;
					case SDL_WINDOWEVENT_RESTORED:
						gSettings.maximized = 0;
						break;
					case SDL_WINDOWEVENT_CLOSE:
						return false;
				}
				break;
			case SDL_MOUSEMOTION:
				if(!ImGui::GetIO().WantCaptureMouse)
				{
					mf->HandleMouseDrag(event.motion.xrel, event.motion.yrel,
						event.motion.state & SDL_BUTTON_LMASK, event.motion.state & SDL_BUTTON_RMASK);
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				break;
			case SDL_KEYDOWN:
				if(!ImGui::GetIO().WantCaptureKeyboard)
				{
					mf->HandleKeys(event.key.keysym.scancode);

				}
				break;
		}
	}

	return true;
}

void Window::UpdateClientRect()
{
	int w, h;
	SDL_GetWindowSize(window, &w, &h);
	mf->SetClientRect(w, h);
}

void Window::Render()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
	mf->DrawUi();
	ImGui::Render();
	
	//auto dispSize = ImGui::GetDrawData()->DisplaySize;
	renderer->Acquire();
	auto *cmd = renderer->GetCommand();
	if(cmd)
	{
		mf->DrawBack();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *cmd);
	}
	else
	{
		std::cout << "no\n";
	}
	renderer->Submit();
}

void Window::SleepUntilNextFrame()
{
	std::chrono::duration<double> targetDur(targetSpf);
	std::chrono::duration<double> dur; 

	auto now = std::chrono::high_resolution_clock::now();
	if((dur = now - startClock) < targetDur)
		std::this_thread::sleep_for((targetDur-dur));
	
	realSpf = dur.count();
	startClock = std::chrono::high_resolution_clock::now();
}

double Window::GetSpf()
{
	return realSpf;
}

void Window::ChangeFramerate()
{
	constexpr double framerateList[4] = {
		0.01600, 
		1.0/30.0, 
		1.0/10.0,
		1.0/2.0
	};

	frameRateChoice++;
	if(frameRateChoice >= 4)
		frameRateChoice = 0;
	targetSpf = framerateList[frameRateChoice];
}
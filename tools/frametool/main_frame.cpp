#include "main_frame.h"

#include "ini.h" 

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl.h>
#include <SDL_scancode.h>
#include <nfd.h>

#include <glad/glad.h>
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>

bool show_demo_window = true;

MainFrame::MainFrame():
currState{},
mainPane(&render, &fd, currState),
x(0),y(-150)
{
	LoadSettings();

	render.LoadGraphics("data/char/vaki/def.lua");
	render.LoadPalette("data/palettes/play2.act");
	
	currentFilePath = "data/char/vaki/vaki.char";
	fd.Load(currentFilePath);
}

MainFrame::~MainFrame()
{
	ImGui::SaveIniSettingsToDisk(ImGui::GetCurrentContext()->IO.IniFilename);
}

void MainFrame::LoadSettings()
{
	LoadTheme(gSettings.theme);
	SetZoom(gSettings.zoomLevel);
	memcpy(clearColor, gSettings.color, sizeof(float)*3);
}

void MainFrame::SetClientRect(int x, int y)
{
	clientRect.x = x;
	clientRect.y = y;
	render.UpdateProj(clientRect.x, clientRect.y);
}

void MainFrame::Draw()
{
	DrawUi();
	DrawBack();

	gSettings.theme = style_idx;
	gSettings.zoomLevel = zoom_idx;
	memcpy(gSettings.color, clearColor, sizeof(float)*3);
}

void MainFrame::DrawBack()
{
	glClearColor(clearColor[0], clearColor[1], clearColor[2], 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	render.x = (x+clientRect.x/2)/render.scale;
	render.y = (y+clientRect.y/2)/render.scale;
	render.Draw();
}

void MainFrame::DrawUi()
{
	ImGuiID errorPopupId = ImGui::GetID("Loading Error");
	
	//Fullscreen docker to provide the layout for the panes
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("Dock Window", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_MenuBar |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus |
		ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoBackground 
	);
		ImGui::PopStyleVar(3);
		Menu(errorPopupId);
		
		ImGuiID dockspaceID = ImGui::GetID("Dock Space");
		if (!ImGui::DockBuilderGetNode(dockspaceID)) {
			ImGui::DockBuilderRemoveNode(dockspaceID);
			ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);
			ImVec2 dim(clientRect.x, clientRect.y); 
			ImGui::DockBuilderSetNodeSize(dockspaceID, dim); 

			ImGuiID toSplit = dockspaceID;
			ImGuiID dock_left_id = ImGui::DockBuilderSplitNode(toSplit, ImGuiDir_Left, 0.30f, nullptr, &toSplit);
			ImGuiID dock_right_id = ImGui::DockBuilderSplitNode(toSplit, ImGuiDir_Right, 0.45f, nullptr, &toSplit);
			ImGuiID dock_down_id = ImGui::DockBuilderSplitNode(toSplit, ImGuiDir_Down, 0.20f, nullptr, &toSplit);

			ImGui::DockBuilderDockWindow("Left Pane", dock_left_id);
			ImGui::DockBuilderDockWindow("Right Pane", dock_right_id);
 			ImGui::DockBuilderDockWindow("Box Pane", dock_down_id);

			ImGui::DockBuilderFinish(dockspaceID);
		}
		ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f),
			ImGuiDockNodeFlags_PassthruCentralNode |
			ImGuiDockNodeFlags_NoDockingInCentralNode |
			ImGuiDockNodeFlags_AutoHideTabBar 
			//ImGuiDockNodeFlags_NoSplit
		); 
	ImGui::End();

	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal("Loading Error", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("There was a problem loading the file.\n"
			"The file couldn't be accessed or it's not a valid file.\n\n");
		ImGui::Separator();
		if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
		ImGui::EndPopup();
	}

	mainPane.Draw();

	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);

	RenderUpdate();
}

void MainFrame::RenderUpdate()
{
	if(!fd.sequences.empty() && fd.sequences[currState.seq].frames.size() > 0)
	{
		auto &frame =  fd.sequences[currState.seq].frames[currState.frame];
		render.spriteId = frame.frameProp.spriteIndex;
		
		render.GenerateHitboxVertices(frame.colbox, Render::gray);
		render.GenerateHitboxVertices(frame.greenboxes, Render::green);
		render.GenerateHitboxVertices(frame.redboxes, Render::red);
		render.LoadHitboxVertices();
		render.offsetX = frame.frameProp.spriteOffset[0];
		render.offsetY = -frame.frameProp.spriteOffset[1];
		render.rotX = frame.frameProp.rotation[0];
		render.rotY = frame.frameProp.rotation[1];
		render.rotZ = frame.frameProp.rotation[2];
		render.scaleX = frame.frameProp.scale[0];
		render.scaleY = frame.frameProp.scale[1];
	}
	else
	{
		render.spriteId = -1;
		render.DontDraw();
	}
}

void MainFrame::AdvancePattern(int dir)
{
 	currState.seq+= dir;
	if(currState.seq < 0)
		currState.seq = 0;
	else if(currState.seq >= fd.sequences.size())
		currState.seq = fd.sequences.size()-1;
	currState.frame = 0; 
}

void MainFrame::AdvanceFrame(int dir)
{
 	auto seq = fd.sequences[currState.seq];
	currState.frame += dir;
	if(currState.frame < 0)
		currState.frame = 0;
	else if(currState.frame >= seq.frames.size())
		currState.frame = seq.frames.size()-1;
}

void MainFrame::UpdateBackProj(float x, float y)
{
	render.UpdateProj(x, y);
	glViewport(0, 0, x, y);
}

void MainFrame::HandleMouseDrag(int x_, int y_, bool dragLeft, bool dragRight)
{
 	/* if(dragRight)
	{
		boxPane.BoxDrag(x_, y_);
	} */
	if(dragLeft)
	{
		x += x_;
		y -= y_;
	}
}

void MainFrame::RightClick(int x_, int y_)
{
	//boxPane.BoxStart((x_-x-clientRect.x/2)/render.scale, (y_-y-clientRect.y/2)/render.scale);
}

bool MainFrame::HandleKeys(uint64_t vkey)
{
	switch (vkey)
	{
	case SDL_SCANCODE_UP:
		AdvancePattern(-1);
		return true;
	case SDL_SCANCODE_DOWN:
		AdvancePattern(1);
		return true;
	case SDL_SCANCODE_LEFT:
		AdvanceFrame(-1);
		return true;
	case SDL_SCANCODE_RIGHT:
		AdvanceFrame(+1);
		return true;
/* 	case SDL_SCANCODE_Z:
		boxPane.AdvanceBox(-1);
		return true;
	case SDL_SCANCODE_X:
		boxPane.AdvanceBox(+1);
		return true; */
	}
	return false;
}

void MainFrame::ChangeClearColor(float r, float g, float b)
{
	clearColor[0] = r;
	clearColor[1] = g;
	clearColor[2] = b;
}

void MainFrame::SetZoom(int level)
{
	zoom_idx = level;
	switch (level)
	{
	case 0: render.scale = 0.5f; break;
	case 1: render.scale = 1.f; break;
	case 2: render.scale = 1.5f; break;
	case 3: render.scale = 2.f; break;
	case 4: render.scale = 3.f; break;
	case 5: render.scale = 4.f; break;
	}
}

void MainFrame::LoadTheme(int i )
{
	style_idx = i;
	switch (i)
	{
		case 0: WarmStyle(); break;
		case 1: ImGui::StyleColorsDark(); ChangeClearColor(0.202f, 0.243f, 0.293f); break;
		case 2: ImGui::StyleColorsLight(); ChangeClearColor(0.534f, 0.568f, 0.587f); break;
		case 3: ImGui::StyleColorsClassic(); ChangeClearColor(0.142f, 0.075f, 0.147f); break;
	}
}

void MainFrame::Menu(unsigned int errorPopupId)
{

	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New file"))
			{
				fd.Clear();
				currentFilePath.clear();
				mainPane.RegenerateNames();
			}
			if (ImGui::MenuItem("Close file"))
			{
				fd.Close();
				currentFilePath.clear();
			}

			ImGui::Separator();
			if (ImGui::MenuItem("Load char file... (OLD)"))
			{
				nfdchar_t* outPath = nullptr;
				nfdfilteritem_t filterItem[2] = {{"Character file", "char"}};
				nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, "data/char");
				if (result == NFD_OKAY && outPath) {
					if(!fd.LoadOld(outPath))
					{
						ImGui::OpenPopup(errorPopupId);
					}
					else
					{
						currentFilePath = outPath;
						mainPane.RegenerateNames();
					}
					NFD_FreePath(outPath);
				}
			}

			if (ImGui::MenuItem("Load char file..."))
			{
				nfdchar_t* outPath = nullptr;
				nfdfilteritem_t filterItem[2] = {{"Character file", "char"}};
				nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, "data/char");
				if (result == NFD_OKAY && outPath) {
					if(!fd.Load(outPath))
					{
						ImGui::OpenPopup(errorPopupId);
					}
					else
					{
						currentFilePath = outPath;
						mainPane.RegenerateNames();
					}
					NFD_FreePath(outPath);
				}
			}

			ImGui::Separator();
			//TODO: Implement hotkeys, someday.
			if (ImGui::MenuItem("Save")) 
			{
				
				if(currentFilePath.empty())
				{
					nfdchar_t* savePath = nullptr;
					nfdfilteritem_t filterItem[2] = {{"Character file", "char"}};
					nfdresult_t result = NFD_SaveDialog(&savePath, filterItem, 1, "data/char", currentFilePath.c_str());
					if (result == NFD_OKAY && savePath) {
						fd.Save(savePath);
						currentFilePath = savePath;
						NFD_FreePath(savePath);
					}
				}
				if(!currentFilePath.empty())
				{
					fd.Save(currentFilePath);
				}
			}

			if (ImGui::MenuItem("Save as...")) 
			{
				nfdchar_t* savePath = nullptr;
				nfdfilteritem_t filterItem[2] = {{"Character file", "char"}};
				nfdresult_t result = NFD_SaveDialog(&savePath, filterItem, 1, "data/char", currentFilePath.c_str());
				if (result == NFD_OKAY && savePath) {
					fd.Save(savePath);
					currentFilePath = savePath;
					NFD_FreePath(savePath);
				}
			}

			ImGui::Separator();
			if (ImGui::MenuItem("Exit"))
			{

			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Preferences"))
		{
			if (ImGui::BeginMenu("Switch preset style"))
			{		
				if (ImGui::Combo("Style", &style_idx, "Warm\0Dark\0Light\0ImGui\0"))
				{
					LoadTheme(style_idx);
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Background color"))
			{
				ImGui::ColorEdit3("##clearColor", (float*)&clearColor, ImGuiColorEditFlags_NoInputs);
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Zoom level"))
			{
				ImGui::SetNextItemWidth(80);
				if (ImGui::Combo("Zoom", &zoom_idx, "x0.5\0x1\0x1.5\0x2\0x3\0x4\0"))
				{
					SetZoom(zoom_idx);
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Help"))
		{
			//if (ImGui::MenuItem("About")) aboutWindow.isVisible = !aboutWindow.isVisible;
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

}

void MainFrame::WarmStyle()
{
	ImVec4* colors = ImGui::GetStyle().Colors;

	colors[ImGuiCol_Text]                   = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_TextDisabled]           = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
	colors[ImGuiCol_WindowBg]               = ImVec4(0.95f, 0.91f, 0.85f, 1.00f);
	colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_PopupBg]                = ImVec4(0.98f, 0.96f, 0.93f, 1.00f);
	colors[ImGuiCol_Border]                 = ImVec4(0.00f, 0.00f, 0.00f, 0.30f);
	colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg]                = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.95f, 1.00f, 0.62f, 1.00f);
	colors[ImGuiCol_FrameBgActive]          = ImVec4(0.98f, 1.00f, 0.81f, 1.00f);
	colors[ImGuiCol_TitleBg]                = ImVec4(0.82f, 0.73f, 0.64f, 0.81f);
	colors[ImGuiCol_TitleBgActive]          = ImVec4(0.97f, 0.65f, 0.00f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
	colors[ImGuiCol_MenuBarBg]              = ImVec4(0.89f, 0.83f, 0.76f, 1.00f);
	colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.89f, 0.83f, 0.76f, 1.00f);
	colors[ImGuiCol_ScrollbarGrab]          = ImVec4(1.00f, 0.96f, 0.87f, 0.99f);
	colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(1.00f, 0.98f, 0.94f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.72f, 0.66f, 0.48f, 0.99f);
	colors[ImGuiCol_CheckMark]              = ImVec4(1.00f, 0.52f, 0.00f, 1.00f);
	colors[ImGuiCol_SliderGrab]             = ImVec4(1.00f, 0.52f, 0.00f, 1.00f);
	colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.55f, 0.53f, 0.32f, 1.00f);
	colors[ImGuiCol_Button]                 = ImVec4(0.74f, 1.00f, 0.53f, 0.25f);
	colors[ImGuiCol_ButtonHovered]          = ImVec4(1.00f, 0.77f, 0.41f, 0.96f);
	colors[ImGuiCol_ButtonActive]           = ImVec4(1.00f, 0.47f, 0.00f, 1.00f);
	colors[ImGuiCol_Header]                 = ImVec4(0.74f, 0.57f, 0.33f, 0.31f);
	colors[ImGuiCol_HeaderHovered]          = ImVec4(0.94f, 0.75f, 0.36f, 0.42f);
	colors[ImGuiCol_HeaderActive]           = ImVec4(1.00f, 0.75f, 0.01f, 0.61f);
	colors[ImGuiCol_Separator]              = ImVec4(0.38f, 0.34f, 0.25f, 0.66f);
	colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.76f, 0.70f, 0.59f, 0.98f);
	colors[ImGuiCol_SeparatorActive]        = ImVec4(0.32f, 0.32f, 0.32f, 0.45f);
	colors[ImGuiCol_ResizeGrip]             = ImVec4(0.35f, 0.35f, 0.35f, 0.17f);
	colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.41f, 0.80f, 1.00f, 0.84f);
	colors[ImGuiCol_ResizeGripActive]       = ImVec4(1.00f, 0.61f, 0.23f, 1.00f);
	colors[ImGuiCol_Tab]                    = ImVec4(0.79f, 0.74f, 0.64f, 0.00f);
	colors[ImGuiCol_TabHovered]             = ImVec4(1.00f, 0.64f, 0.06f, 0.85f);
	colors[ImGuiCol_TabActive]              = ImVec4(0.69f, 0.40f, 0.12f, 0.31f);
	colors[ImGuiCol_TabUnfocused]           = ImVec4(0.93f, 0.92f, 0.92f, 0.98f);
	colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.91f, 0.87f, 0.74f, 1.00f);
	colors[ImGuiCol_DockingPreview]         = ImVec4(0.26f, 0.98f, 0.35f, 0.22f);
	colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	colors[ImGuiCol_PlotLines]              = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.58f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.40f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.45f, 0.00f, 1.00f);
	colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.69f, 0.53f, 0.32f, 0.30f);
	colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.69f, 0.58f, 0.44f, 1.00f);
	colors[ImGuiCol_TableBorderLight]       = ImVec4(0.70f, 0.62f, 0.42f, 0.40f);
	colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_TableRowBgAlt]          = ImVec4(0.30f, 0.30f, 0.30f, 0.09f);
	colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.98f, 0.89f, 0.35f);
	colors[ImGuiCol_DragDropTarget]         = ImVec4(0.26f, 0.98f, 0.94f, 0.95f);
	colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.97f, 0.98f, 0.80f);
	colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(0.70f, 0.70f, 0.70f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
	ChangeClearColor(0.324f, 0.409f, 0.185f);
}

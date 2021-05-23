#ifndef INI_H_GUARD
#define INI_H_GUARD

extern struct Settings
{
	//default
	float color[3]{0.324f, 0.409f, 0.185f};
	int zoomLevel = 3;
	int theme = 0;
	float fontSize = 18.f;
	short posX = 100;
	short posY = 100;
	short winSizeX = 1280;
	short winSizeY = 800;
	bool maximized = false;
} gSettings;

void InitIni();

#endif /* INI_H_GUARD */

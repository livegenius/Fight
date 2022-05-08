#ifndef AUDIO_H_INCLUDED
#define AUDIO_H_INCLUDED

#include <soloud.h>
#include <soloud_wav.h>
#include <soloud_wavstream.h>
#include <filesystem>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

extern SoLoud::Soloud* soloud;



#undef PlaySound
class SoundEffects{
private:
	std::vector<std::unique_ptr<SoLoud::Wav>> wavs;
	std::unordered_map<std::string, SoLoud::Wav*> loadedResources;
	std::unordered_map<std::string, SoLoud::Wav*> aliasMap;

	void LoadSound(const std::string &file, const std::string &alias = {});

	int &gameTime;
	int lastGameTime;
	float offset;

public:
	SoundEffects(int &);
	void LoadFromDef(const std::filesystem::path &file);
	void PlaySound(const std::string &alias);
};

#endif // AUDIO_H_INCLUDED

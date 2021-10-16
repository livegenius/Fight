#include "audio.h"
#include <sol/sol.hpp>
#include <iostream>

SoLoud::Soloud* soloud = nullptr;

SoundEffects::SoundEffects(int &gameTime):
gameTime(gameTime)
{}

void SoundEffects::LoadFromDef(const std::filesystem::path &file)
{
	auto folder = file.parent_path();
	sol::state lua;

	auto result = lua.script_file(file.string());
	if(!result.valid()){
		sol::error err = result;
		std::cerr << "When loading " << file <<"\n";
		std::cerr << err.what() << std::endl;
		throw std::runtime_error("Lua syntax error.");
	}

	auto previousPath = std::filesystem::current_path();
	std::filesystem::current_path(folder);
	sol::table sfxTable = lua["sfx"];
	for(auto &entry : sfxTable)
	{
		//Second column has the filepath.
		LoadSound(entry.second.as<std::string>(), entry.first.as<std::string>());
	}

	std::filesystem::current_path(previousPath);
}

void SoundEffects::LoadSound(const std::string &file, const std::string &alias)
{
	if(aliasMap.count(alias) > 0) //Alias already exists. Maybe throw.
		return;

	auto search = loadedResources.find(file);
	if(search != loadedResources.end()) //Find whether resource is already loaded.
	{
		aliasMap.insert({alias, search->second});
		return;
	}
	else
	{
		auto wav = wavs.emplace_back(new SoLoud::Wav).get();
		wav->load(file.c_str());
		loadedResources.insert({file, wav});
		aliasMap.insert({alias, wav});
	}
}

void SoundEffects::PlaySound(const std::string &alias)
{
	if(lastGameTime != gameTime)
	{
		lastGameTime = gameTime;
		offset = 0;
	}

	auto search = aliasMap.find(alias);
	if (search != aliasMap.end())
		soloud->playClocked(gameTime/60.f+offset, *search->second);
	offset+=0.005;
}
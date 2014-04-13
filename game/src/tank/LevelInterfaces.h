// LevelInterfaces.h

#pragma once
#include <cstdint>
#include <memory>
#include <string>

namespace FS
{
	class Stream;
}

struct PlayerHandle
{
	virtual size_t GetIndex() const = 0;
};

struct PlayerDesc
{
	std::string nick;
	std::string skin;
	std::string cls;
	unsigned int team;
};

struct BotDesc
{
	PlayerDesc pd;
	unsigned int level;
};


struct ILevelController
{
	virtual float GetTime() const = 0;

#ifdef NETWORK_DEBUG
	virtual uint32_t GetChecksum() const = 0;
	virtual unsigned int GetFrame() const = 0;
#endif
    
	virtual void Clear() = 0;
	virtual PlayerHandle* GetPlayerByIndex(size_t playerIndex) = 0;
	virtual void SetPlayerInfo(PlayerHandle *p, const PlayerDesc &pd) = 0;
	virtual void PlayerQuit(PlayerHandle *p) = 0;
	virtual PlayerHandle* AddHuman(const PlayerDesc &pd) = 0;
	virtual void AddBot(const BotDesc &bd) = 0;
	virtual void init_newdm(std::shared_ptr<FS::Stream> s, unsigned long seed) = 0;
};


//class ClientBase;
//struct IClientFactory
//{
//	ClientBase* CreateClient
//};

// end of file

// LevelInterfaces.h

#pragma once

struct PlayerDesc;
struct BotDesc;
namespace FS
{
	class Stream;
}

struct ILevelController
{
	virtual DWORD GetChecksum() const = 0;
	virtual DWORD GetFrame() const = 0;

	virtual void Clear() = 0;
	virtual void SetPlayerInfo(size_t playerIndex, const PlayerDesc &pd) = 0;
	virtual void PlayerQuit(size_t playerIndex) = 0;
	virtual void AddHuman(const PlayerDesc &pd, bool isLocal) = 0;
	virtual void AddBot(const BotDesc &bd) = 0;
	virtual void init_newdm(const SafePtr<FS::Stream> &s, unsigned long seed) = 0;
};


//class ClientBase;
//struct IClientFactory
//{
//	ClientBase* CreateClient
//};

// end of file

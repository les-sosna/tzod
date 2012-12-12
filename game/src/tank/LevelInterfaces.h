// LevelInterfaces.h

#pragma once

struct PlayerDesc;
struct BotDesc;
namespace FS
{
	class Stream;
}

struct PlayerHandle
{
	virtual size_t GetIndex() const = 0;
};

struct ILevelController
{
	virtual float GetTime() const = 0;

	virtual DWORD GetChecksum() const = 0;
	virtual DWORD GetFrame() const = 0;

	virtual void Clear() = 0;
	virtual PlayerHandle* GetPlayerByIndex(size_t playerIndex) = 0;
	virtual void SetPlayerInfo(PlayerHandle *p, const PlayerDesc &pd) = 0;
	virtual void PlayerQuit(PlayerHandle *p) = 0;
	virtual PlayerHandle* AddHuman(const PlayerDesc &pd) = 0;
	virtual void AddBot(const BotDesc &bd) = 0;
	virtual void init_newdm(FS::Stream *s, unsigned long seed) = 0;
};


//class ClientBase;
//struct IClientFactory
//{
//	ClientBase* CreateClient
//};

// end of file

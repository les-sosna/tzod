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
	virtual void Clear() = 0;
	virtual void SetPlayerInfo(unsigned short id, const PlayerDesc &pd, bool isLocal) = 0;
	virtual void PlayerQuit(unsigned short id) = 0;
	virtual void AddBot(const BotDesc &bd) = 0;
	virtual void init_newdm(const SafePtr<FS::Stream> &s, unsigned long seed) = 0;
};



// end of file

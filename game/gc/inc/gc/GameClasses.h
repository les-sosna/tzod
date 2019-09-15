#pragma once
#include "MovingObject.h"
#include "NeighborAware.h"
#include "ObjPtr.h"

class GC_RigidBodyStatic;
class GC_Player;

class GC_HealthDaemon : public GC_MovingObject
{
	DECLARE_SELF_REGISTRATION(GC_HealthDaemon);
	DECLARE_LIST_MEMBER(override);

public:
	GC_HealthDaemon(vec2d pos, GC_Player *owner, float damagePerSecond, float time);
	GC_HealthDaemon(FromFile);
	virtual ~GC_HealthDaemon();

	void SetVictim(World &world, GC_RigidBodyStatic *victim);

	void Serialize(World &world, SaveFile &f) override;
	void TimeStep(World &world, float dt) override;

private:
	float _time;
	float _damage; //  hp per sec

	ObjPtr<GC_RigidBodyStatic> _victim;
	ObjPtr<GC_Player> _owner;
};

/////////////////////////////////////////////////////////////

#define GC_FLAG_WOOD_                       (GC_FLAG_MO_ << 0)

class GC_Wood : public GC_MovingObject
              , public GI_NeighborAware
{
	DECLARE_SELF_REGISTRATION(GC_Wood);

public:
	explicit GC_Wood(vec2d pos);
	explicit GC_Wood(FromFile);
	virtual ~GC_Wood();

	// GC_MovingObject
	void MoveTo(World &world, const vec2d &pos) override;

	// GI_NeighborAware
	int GetNeighbors(const World &world) const override;

	// GC_Object
	void Init(World &world) override;
	void Kill(World &world) override;
};

/////////////////////////////////////////////////////////////

class GC_Text : public GC_MovingObject
{
public:
	enum Style
	{
		DEFAULT,
		SCORE_PLUS,
		SCORE_MINUS,
	};

	GC_Text(vec2d pos, std::string text);
	explicit GC_Text(FromFile) : GC_MovingObject(FromFile()) {}
	virtual ~GC_Text() = 0;

	void SetText(std::string text) { _text = std::move(text); }
	void SetStyle(Style style) { _style = style; }
	Style GetStyle() const { return _style; }
	const std::string& GetText() const { return _text; }

    void Serialize(World &world, SaveFile &f) override;

private:
	Style               _style;
	std::string         _text;
};

/////////////////////////////////////////////////////////////

class GC_Text_ToolTip : public GC_Text
{
	DECLARE_SELF_REGISTRATION(GC_Text_ToolTip);
	DECLARE_LIST_MEMBER(override);

public:
	GC_Text_ToolTip(vec2d pos, std::string text, Style style);
	GC_Text_ToolTip(FromFile) : GC_Text(FromFile()) {}

	void Serialize(World &world, SaveFile &f) override;
	void TimeStep(World &world, float dt) override;

private:
	float  _time;
};

///////////////////////////////////////////////////////////////////////////////
// end of file

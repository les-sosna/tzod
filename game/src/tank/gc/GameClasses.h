// GameClasses.h

#pragma once

#include "Actor.h"
#include "NeighborAware.h"
#include "video/DrawingContext.h"

class GC_RigidBodyStatic;
class GC_Player;

class GC_HealthDaemon : public GC_Actor
{
	DECLARE_SELF_REGISTRATION(GC_HealthDaemon);
    typedef GC_Actor base;

private:
	float _time;
	float _damage; //  hp per sec

	ObjPtr<GC_RigidBodyStatic> _victim;
	ObjPtr<GC_Player> _owner;

public:
    DECLARE_LIST_MEMBER();
	GC_HealthDaemon(World &world, GC_Player *owner, float damagePerSecond, float time);
	GC_HealthDaemon(FromFile);
	virtual ~GC_HealthDaemon();
    
    void SetVictim(World &world, GC_RigidBodyStatic *victim);

	virtual void Serialize(World &world, SaveFile &f);

	virtual void TimeStepFixed(World &world, float dt);

	void OnVictimMove(World &world, GC_Object *sender, void *param);
	void OnVictimKill(World &world, GC_Object *sender, void *param);
};

/////////////////////////////////////////////////////////////

#define GC_FLAG_WOOD_INTILE                 (GC_FLAG_ACTOR_ << 0)
#define GC_FLAG_WOOD_                       (GC_FLAG_ACTOR_ << 1)

class GC_Wood : public GC_Actor
              , public GI_NeighborAware
{
	DECLARE_SELF_REGISTRATION(GC_Wood);
    typedef GC_Actor base;
private:

/**
 *   tile bits
 *
 *   5   6   7
 *    +-----+
 *    |     |
 *  4 |  #  | 0
 *    |     |
 *    +-----+
 *   3   2   1
**/
	int _tile;

protected:
	void UpdateTile(World &world, bool flag);

public:
    DECLARE_GRID_MEMBER();
    
	GC_Wood(World &world);
	GC_Wood(FromFile);
	virtual ~GC_Wood();

	void SetTile(char nTile, bool value);
    
    // GC_Actor
    virtual void MoveTo(World &world, const vec2d &pos) override;

	// GI_NeighborAware
	virtual int GetNeighbors() const override { return _tile; }

	// GC_Object
    virtual void Kill(World &world);
	virtual void Serialize(World &world, SaveFile &f);
};

/////////////////////////////////////////////////////////////

class GC_Text : public GC_Actor
{
public:
	enum Style
	{
		DEFAULT,
		SCORE_PLUS,
		SCORE_MINUS,
	};
	
	GC_Text(World &world, const std::string &text, enumAlignText align = alignTextLT);
	GC_Text(FromFile) {}
	virtual ~GC_Text() = 0;
	
	void SetText(std::string text) { _text = std::move(text); }
	void SetAlign(enumAlignText align) { _align = align; }
	void SetStyle(Style style) { _style = style; }
	Style GetStyle() const { return _style; }
	enumAlignText GetAlign() const { return _align; }
	const std::string& GetText() const { return _text; }

	virtual void Serialize(World &world, SaveFile &f);

protected:
	Style               _style;
	enumAlignText       _align;
	std::string         _text;
};

/////////////////////////////////////////////////////////////

class GC_Text_ToolTip : public GC_Text
{
	DECLARE_SELF_REGISTRATION(GC_Text_ToolTip);
    typedef GC_Text base;

	float  _time;

public:
    DECLARE_LIST_MEMBER();
	GC_Text_ToolTip(World &world, const std::string &text, Style style);
	GC_Text_ToolTip(FromFile) : GC_Text(FromFile()) {}

	virtual void Serialize(World &world, SaveFile &f);
	virtual void TimeStepFloat(World &world, float dt);
};

///////////////////////////////////////////////////////////////////////////////
// end of file

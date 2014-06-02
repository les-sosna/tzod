// GameClasses.h

#pragma once

#include "Rotator.h"

#include "Service.h"
#include "2dSprite.h"
#include "Light.h"

/////////////////////////////////////////////////////////////

class GC_Player;

class GC_Explosion : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_Explosion);
    typedef GC_2dSprite base;
protected:

	struct FieldNode
	{
		FieldNode *parent;

		// horizontal size = 12; diagonal = 17
		unsigned int x        : 10;
		unsigned int y        : 10;
		unsigned int distance : 10;
		bool checked          :  1;
		bool open             :  1;

		FieldNode()
		{
			distance = 0;
			checked  = false;
			open     = true;
			parent   = NULL;
		};

		float GetRealDistance() const
		{
			return (float) distance / 12.0f * (float) CELL_SIZE;
		}
	};

	struct coord
	{
		short x, y;
		coord() {}
		coord(short x_, short y_) { x = x_; y = y_; }
		operator size_t () const { return x + LEVEL_MAXSIZE * y; }
	};

	typedef std::map<coord, FieldNode> FIELD_TYPE;

	bool _boomOK;

	ObjPtr<GC_Player>  _owner;
	ObjPtr<GC_Light>   _light;

	float CheckDamage(FIELD_TYPE &field, float dst_x, float dst_y, float max_distance);

public:
	float _time;
	float _time_life;
	float _time_boom;

public:
    DECLARE_MEMBER_OF();
	GC_Explosion(World &world, GC_Player *owner);
	GC_Explosion(FromFile);
	virtual ~GC_Explosion();


	float _damage;
	float _radius;

	void Boom(World &world, float radius, float damage);

	// GC_Object
	virtual void Serialize(World &world, SaveFile &f);
	virtual void TimeStepFixed(World &world, float dt);
    virtual void Kill(World &world) override;
};

/////////////////////////////////////////////////////////////

class GC_Boom_Standard : public GC_Explosion
{
	DECLARE_SELF_REGISTRATION(GC_Boom_Standard);
public:
	GC_Boom_Standard(World &world, const vec2d &pos, GC_Player *owner);
	GC_Boom_Standard(FromFile);
	virtual ~GC_Boom_Standard();
};

/////////////////////////////////////////////////////////////

class GC_Boom_Big :  public GC_Explosion
{
	DECLARE_SELF_REGISTRATION(GC_Boom_Big);
public:
	GC_Boom_Big(World &world, const vec2d &pos, GC_Player *owner);
	GC_Boom_Big(FromFile);
};

/////////////////////////////////////////////////////////////

class GC_RigidBodyStatic;

class GC_HealthDaemon : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_HealthDaemon);
    typedef GC_2dSprite base;

private:
	float _time;
	float _damage; //  hp per sec

	ObjPtr<GC_RigidBodyStatic> _victim;
	ObjPtr<GC_Player> _owner;

public:
    DECLARE_MEMBER_OF();
	GC_HealthDaemon(World &world, GC_RigidBodyStatic *victim, GC_Player *owner,
		            float damagePerSecond, float time);
	GC_HealthDaemon(FromFile);
	virtual ~GC_HealthDaemon();

	virtual void Serialize(World &world, SaveFile &f);

	virtual void TimeStepFloat(World &world, float dt);
	virtual void TimeStepFixed(World &world, float dt);

	void OnVictimMove(World &world, GC_Object *sender, void *param);
	void OnVictimKill(World &world, GC_Object *sender, void *param);
};

/////////////////////////////////////////////////////////////

#define GC_FLAG_WOOD_INTILE                 (GC_FLAG_2DSPRITE_ << 0)
#define GC_FLAG_WOOD_                       (GC_FLAG_2DSPRITE_ << 1)

class GC_Wood : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_Wood);
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
	unsigned char _tile;

protected:
	void UpdateTile(World &world, bool flag);

public:
	GC_Wood(World &world);
	GC_Wood(FromFile);
	virtual ~GC_Wood();

	void SetTile(char nTile, bool value);
    
    // GC_Actor
    virtual void MoveTo(World &world, const vec2d &pos) override;

	// GC_2dSprite
	virtual void Draw(bool editorMode) const;

	// GC_Object
    virtual void Kill(World &world);
	virtual void Serialize(World &world, SaveFile &f);
};

/////////////////////////////////////////////////////////////

class GC_Text : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_Text);

public:
	GC_Text(World &world, const std::string &text, enumAlignText align = alignTextLT);
	GC_Text(FromFile) : GC_2dSprite(FromFile()) {};

	void SetFont(const char *fontname);
	void SetText(const std::string &text);
	void SetAlign(enumAlignText align);
	const std::string& GetText() const { return _text; }

	virtual void Serialize(World &world, SaveFile &f);
	virtual void Draw(bool editorMode) const;

protected:
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
    DECLARE_MEMBER_OF();
	GC_Text_ToolTip(World &world, const std::string &text, const char *font);
	GC_Text_ToolTip(FromFile) : GC_Text(FromFile()) {};

    virtual void Draw(bool editorMode) const;
	virtual void Serialize(World &world, SaveFile &f);
	virtual void TimeStepFloat(World &world, float dt);
};

///////////////////////////////////////////////////////////////////////////////
// end of file

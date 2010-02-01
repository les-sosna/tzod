// GameClasses.h

#pragma once

#include "core/Rotator.h"

#include "Service.h"
#include "2dSprite.h"
#include "Light.h"

/////////////////////////////////////////////////////////////

class GC_Player;

class GC_Explosion : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_Explosion);
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

	SafePtr<GC_Player>  _owner;
	SafePtr<GC_Light>   _light;

	float CheckDamage(FIELD_TYPE &field, float dst_x, float dst_y, float max_distance);

public:
	float _time;
	float _time_life;
	float _time_boom;

public:
	GC_Explosion(GC_Player *owner);
	GC_Explosion(FromFile);
	virtual ~GC_Explosion();


	float _damage;
	float _radius;

	void Boom(float radius, float damage);

	// overrides
	virtual void Kill();
	virtual void Serialize(SaveFile &f);
	virtual void TimeStepFixed(float dt);
};

/////////////////////////////////////////////////////////////

class GC_Boom_Standard : public GC_Explosion
{
	DECLARE_SELF_REGISTRATION(GC_Boom_Standard);
public:
	GC_Boom_Standard(const vec2d &pos, GC_Player *owner);
	GC_Boom_Standard(FromFile);
	virtual ~GC_Boom_Standard();
};

/////////////////////////////////////////////////////////////

class GC_Boom_Big :  public GC_Explosion
{
	DECLARE_SELF_REGISTRATION(GC_Boom_Big);
public:
	GC_Boom_Big(const vec2d &pos, GC_Player *owner);
	GC_Boom_Big(FromFile);
};

/////////////////////////////////////////////////////////////

class GC_HealthDaemon : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_HealthDaemon);

private:
	float _time;
	float _damage; //  hp per sec

	SafePtr<GC_RigidBodyStatic> _victim;
	SafePtr<GC_Player> _owner;

public:
	GC_HealthDaemon(GC_RigidBodyStatic *victim, GC_Player *owner,
		            float damagePerSecond, float time);
	GC_HealthDaemon(FromFile);
	virtual ~GC_HealthDaemon();

	virtual void Kill();

	virtual void Serialize(SaveFile &f);

	virtual void TimeStepFloat(float dt);
	virtual void TimeStepFixed(float dt);

	void OnVictimMove(GC_Object *sender, void *param);
	void OnVictimKill(GC_Object *sender, void *param);
};

/////////////////////////////////////////////////////////////

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
	BYTE _tile;

protected:
	void UpdateTile(bool flag);

public:
	GC_Wood(float xPos, float yPos);
	GC_Wood(FromFile);

	virtual void Kill();
	virtual void Serialize(SaveFile &f);

	virtual void Draw() const;

public:
	void SetTile(char nTile, bool value);
};

/////////////////////////////////////////////////////////////

class GC_Text : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_Text);

public:
	GC_Text(int x, int y, const string_t &text, enumAlignText align = alignTextLT);
	GC_Text(FromFile) : GC_2dSprite(FromFile()) {};

	void SetFont(const char *fontname);
	void SetText(const string_t &text);
	void SetAlign(enumAlignText align);
	const string_t& GetText() const { return _text; }

	virtual void Serialize(SaveFile &f);
	virtual void Draw() const;

private:
	enumAlignText       _align;
	string_t            _text;
};

/////////////////////////////////////////////////////////////

class GC_Text_ToolTip : public GC_Text
{
	DECLARE_SELF_REGISTRATION(GC_Text_ToolTip);

private:
	float  _time;
	float  _y0;

public:
	GC_Text_ToolTip(vec2d pos, const string_t &text, const char *font);
	GC_Text_ToolTip(FromFile) : GC_Text(FromFile()) {};

	virtual void Serialize(SaveFile &f);
	virtual void TimeStepFloat(float dt);
};

///////////////////////////////////////////////////////////////////////////////
// end of file

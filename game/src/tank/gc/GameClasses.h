// GameClasses.h

#pragma once

#include "core/Rotator.h"

#include "Service.h"
#include "2dSprite.h"
#include "Light.h"

/////////////////////////////////////////////////////////////

class GC_Background : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_Background);

private:
	bool _drawGrid;

public:
	GC_Background();
	GC_Background(FromFile);
	~GC_Background();
	virtual void Draw();

public:
	void EnableGrid(bool bEnable);
	void OnChangeDrawGridVariable();
};

/////////////////////////////////////////////////////////////

class GC_RigidBodyStatic;

class GC_Explosion : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_Explosion);
protected:

	// узел поля (клетка)
	struct FieldNode
	{
		FieldNode *parent;

		// размер одной клетки по горизонтали = 12; по диагонали = 17
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

	SafePtr<GC_RigidBodyStatic>  _owner;
	SafePtr<GC_Light>            _light;

	float CheckDamage(FIELD_TYPE &field, float dst_x, float dst_y, float max_distance);

public:
	float _time;
	float _time_life;
	float _time_boom;

public:
	GC_Explosion(GC_RigidBodyStatic *owner);
	GC_Explosion(FromFile);
	virtual ~GC_Explosion();


	float _damage;
	float _DamRadius;

	void Boom(float radius, float damage);

	// overrides
	virtual void Kill();
	virtual bool IsSaved() const { return true; }
	virtual void Serialize(SaveFile &f);
	virtual void TimeStepFixed(float dt);
};

/////////////////////////////////////////////////////////////

class GC_Boom_Standard : public GC_Explosion
{
	DECLARE_SELF_REGISTRATION(GC_Boom_Standard);
public:
	GC_Boom_Standard(const vec2d &pos, GC_RigidBodyStatic *owner);
	GC_Boom_Standard(FromFile);
	virtual ~GC_Boom_Standard();
};

/////////////////////////////////////////////////////////////

class GC_Boom_Big :  public GC_Explosion
{
	DECLARE_SELF_REGISTRATION(GC_Boom_Big);
public:
	GC_Boom_Big(const vec2d &pos, GC_RigidBodyStatic *owner);
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
	SafePtr<GC_RigidBodyStatic> _owner;

public:
	GC_HealthDaemon(GC_RigidBodyStatic *victim, GC_RigidBodyStatic *owner,
		            float damagePerSecond, float time);
	GC_HealthDaemon(FromFile);
	virtual ~GC_HealthDaemon();

	virtual void Kill();

	virtual bool IsSaved() const { return true; }
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
	virtual bool IsSaved() const { return true; }
	virtual void Serialize(SaveFile &f);

	virtual void Draw();

public:
	void SetTile(char nTile, bool value);
};

///////////////////////////////////////////////////////////////////////////////

class GC_Line : public GC_UserSprite
{
	DECLARE_SELF_REGISTRATION(GC_Line);

private:
	vec2d _begin;
	vec2d _end;

	float _phase;
	float _sprite_width;

	int   _frame;

public:
	GC_Line(const vec2d &begin, const vec2d &end, const char *texture);
	GC_Line(FromFile) : GC_UserSprite(FromFile()) {}

	void SetPhase(float f);
	void MoveTo(const vec2d &begin, const vec2d &end);
	virtual void MoveTo(const vec2d &pos) { GC_UserSprite::MoveTo(pos); }

	virtual void Serialize(SaveFile &f);
	virtual void Draw();

	void SetLineView(int index);
};

/////////////////////////////////////////////////////////////
//class text

class GC_Text : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_Text);

private:
	std::vector<size_t> _lines;   // длины строк
	size_t              _maxline; // макс. длина строки
	enumAlignText       _align;
	float               _margin_x;
	float               _margin_y;
	string_t            _text;

private:
	void UpdateLines();

public:
	GC_Text(int x, int y, const char *text, enumAlignText align = alignTextLT);
	GC_Text(FromFile) : GC_2dSprite(FromFile()) {};

public:
	void SetFont(const char *fontname);
	void SetText(const char *text);
	void SetAlign(enumAlignText align);
	void SetMargins(float mx, float my);
	size_t GetTextLenght() { return _text.size(); }
	string_t GetText() const { return _text; }

public:
	virtual void Draw();
};

/////////////////////////////////////////////////////////////

class GC_Text_ToolTip : public GC_Text
{
	DECLARE_SELF_REGISTRATION(GC_Text_ToolTip);

private:
	float  _time;
	float  _y0;

public:
	GC_Text_ToolTip(vec2d pos, const char *text, const char *font);
	GC_Text_ToolTip(FromFile) : GC_Text(FromFile()) {};

	virtual void TimeStepFloat(float dt);
};

///////////////////////////////////////////////////////////////////////////////
// end of file

//GameClasses.h
/////////////////////////////////////////////////////////////
#pragma once

#include "Controller.h"
#include "core/Rotator.h"

#include "2dSprite.h"
#include "Light.h"

////////////////////////////////////////////////////////////
// winamp controller

class GC_Winamp : public GC_Object
{
	DECLARE_SELF_REGISTRATION(GC_Winamp);

private:
	HWND _hwnd_winamp;

	DWORD _time_last;	// время, независимое от скорости игры
	int   _time;

	char _last_b1;
	char _last_b2;
	char _last_b3;
	char _last_b4;
	char _last_b5;
	char _last_up;
	char _last_down;
	char _last_ffw;
	char _last_rew;

	UINT frame_counter;

	void SendCommand(HWND hWnd, WPARAM command, char *last, char *current)
	{
		if (*last && !*current)	PostMessage(hWnd, WM_COMMAND, command, 0);
		*last = *current;
	}

public:
	GC_Winamp();
	GC_Winamp(FromFile) : GC_Object(FromFile()) {};
	void FindWinamp();
	virtual void EndFrame();
};

////////////////////////////////////////////////////////////

class GC_Camera : public GC_Object
{
	DECLARE_SELF_REGISTRATION(GC_Camera);
	MemberOfGlobalList _memberOf;

private:
	vec2d	_target;
	float   _time_shake;
	float   _time_seed;
	float   _angle_current;
	DWORD   _dwTimeX;
	DWORD   _dwTimeY;
	float   _dt;
	bool    _active;

	Rotator _rotator;


	//--------------------------------
public:
	RECT                 _viewport;
	float                _zoom;
	SafePtr<GC_Player>  _player;

	//--------------------------------
public:
	GC_Camera(GC_Player *pPlayer);
	GC_Camera(FromFile);

	void Select();	         // применение трансформации, выбор камеры как текущей
	void Activate(bool bActivate);	// неактивная камера не отображается на экране
	bool IsActive() const { return _active && !IsKilled(); }

	static void SwitchEditor();
	static void UpdateLayout();	// пересчет координат viewports
	static bool GetWorldMousePos(vec2d &pos);

	virtual void Shake(float level);
	float GetShake() const { return _time_shake; }

	//////////////////////////////////////
	// GC_Object overrides
	virtual void Kill();
	virtual bool IsSaved() { return _player != NULL; }
	virtual void Serialize(SaveFile &f);
	virtual void TimeStepFloat(float dt);
	virtual void EndFrame();

	//////////////////////////////////////
	// message handlers
	void OnDetach(GC_Object *sender, void *param);
};

/////////////////////////////////////////////////////////////

class GC_Background : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_Background);

private:
	bool _drawGrid;

public:
	GC_Background();
	GC_Background(FromFile) : GC_2dSprite(FromFile()) {};
	virtual void Draw();

public:
	void EnableGrid(bool bEnable);
};

typedef DynamicSingleton<GC_Background> _Background;


/////////////////////////////////////////////////////////////

class GC_RigidBodyStatic;

class GC_Explosion : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_Explosion);
protected:

	// узел поля (клетка)
	struct _tagFieldNode
	{
		_tagFieldNode *parent;

		// размер одной клетки по горизонтали = 12; по диагонали = 17
		unsigned int x        : 10;
		unsigned int y        : 10;
		unsigned int distance : 10;
		bool checked          :  1;
		bool open             :  1;

		_tagFieldNode()
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

	typedef std::map<struct coord, _tagFieldNode> FIELD_TYPE;

	bool _boomOK;

	SafePtr<GC_RigidBodyStatic>  _proprietor;
	SafePtr<GC_Light>            _light;

	float CheckDamage(FIELD_TYPE &field, float dst_x, float dst_y, float max_distance);

public:
	float _time;
	float _time_life;
	float _time_boom;

public:
	GC_Explosion(GC_RigidBodyStatic *pProprietor);
	GC_Explosion(FromFile);
	virtual ~GC_Explosion();


	float _Damage;
	float _DamRadius;

	void Boom(float radius, float damage);

	//////////////////////////////////////
	// GC_Object overrides

	virtual void Kill();
	virtual bool IsSaved() { return true; };
	virtual void Serialize(SaveFile &f);
	virtual void TimeStepFixed(float dt);
};

/////////////////////////////////////////////////////////////

class GC_Boom_Standard : public GC_Explosion
{
	DECLARE_SELF_REGISTRATION(GC_Boom_Standard);
public:
	GC_Boom_Standard(const vec2d &pos, GC_RigidBodyStatic *pProprietor);
	GC_Boom_Standard(FromFile);
	virtual ~GC_Boom_Standard();
};

/////////////////////////////////////////////////////////////

class GC_Boom_Big :  public GC_Explosion
{
	DECLARE_SELF_REGISTRATION(GC_Boom_Big);
public:
	GC_Boom_Big(const vec2d &pos, GC_RigidBodyStatic *pProprietor);
	GC_Boom_Big(FromFile);
};

/////////////////////////////////////////////////////////////

class GC_HealthDaemon : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_HealthDaemon);

private:
	float _time;
	float _damage;	//  hp per sec

	SafePtr<GC_RigidBodyStatic> _victim;
	SafePtr<GC_RigidBodyStatic> _owner;

public:
	GC_HealthDaemon(GC_RigidBodyStatic *pVictim, GC_RigidBodyStatic *pOwner,
		            float damage, float time);
	GC_HealthDaemon(FromFile) : GC_2dSprite(FromFile()) {}

	virtual void Kill();

	virtual bool IsSaved() { return true; };
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
	virtual bool IsSaved() { return true; };
	virtual void Serialize(SaveFile &f);

	virtual void Draw();

public:
	void SetTile(char nTile, bool value);
};

/////////////////////////////////////////////////////////////

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

class GC_Rectangle : public GC_Line
{
	vec2d _center_pos;
	vec2d _size;

public:
	GC_Rectangle(const vec2d &pos, const vec2d &size, const char *texture);

	void Adjust(GC_2dSprite *object);

    void SetSize(const vec2d &size);
	virtual void MoveTo(const vec2d &center_pos);
	virtual void Draw();

	virtual bool IsSaved() { return false; }
};

/////////////////////////////////////////////////////////////
//class text

class GC_Text : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_Text);

private:
	std::vector<size_t> _lines;	// длины строк
	size_t              _maxline;	// макс. длина строки
	enumAlignText       _align;
	float               _margin_x;
	float               _margin_y;
	string_t            _text;

private:
	void UpdateLines();

public:
	GC_Text(int x, int y, const char *lpszText, enumAlignText align = alignTextLT);
	GC_Text(FromFile) : GC_2dSprite(FromFile()) {};

public:
	void SetFont(const char *fontname);
	void SetText(const char *lpszText);
	void SetAlign(enumAlignText align);
	void SetMargins(float mx, float my);
	size_t GetTextLenght() { return _text.size(); }
	string_t GetText() const { return _text; }

public:
	virtual void Draw();
};

/////////////////////////////////////////////////////////////
// таблица с фрагами
class GC_TextScore : public GC_Text
{
	DECLARE_SELF_REGISTRATION(GC_TextScore);

private:
	SafePtr<GC_2dSprite> _background;
	bool _bOldLimit;

protected:
	// PlayerDesc::index будет использоваться для хранения фрагов
	std::vector<PlayerDesc>	_players;

	void Refresh();

public:
	GC_TextScore();
	GC_TextScore(FromFile) : GC_Text(FromFile()) {};
	virtual void Kill();

	virtual void Draw();
	virtual void EndFrame();
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

/////////////////////////////////////////////////////////////

class GC_Text_MessageArea : public GC_Text
{
private:
	struct Line
	{
		float time;
		string_t str;
	};
	std::deque<Line> _lines;

public:
	GC_Text_MessageArea();
	virtual void TimeStepFloat(float dt);
	void message(const char *text);
};
typedef DynamicSingleton<GC_Text_MessageArea> _MessageArea;

///////////////////////////////////////////////////////////////////////////////
// end of file

// RigidBody.h

#pragma once

#include "2dSprite.h"

///////////////////////////////////////////////////////////////////////////////

#define GC_FLAG_RBSTATIC_TRACE0     (GC_FLAG_2DSPRITE_ << 0) // penetration of projectiles
#define GC_FLAG_RBSTATIC_DESTROYED  (GC_FLAG_2DSPRITE_ << 1) // object has been destroyed
#define GC_FLAG_RBSTATIC_           (GC_FLAG_2DSPRITE_ << 2)

class GC_Player;

struct DamageDesc
{
	float  damage;
	vec2d  hit;
	GC_Player *from;
};

class GC_RigidBodyStatic : public GC_2dSprite
{
	std::string _scriptOnDestroy;  // on_destroy()
	std::string _scriptOnDamage;   // on_damage()

protected:
	class MyPropertySet : public GC_2dSprite::MyPropertySet
	{
		typedef GC_2dSprite::MyPropertySet BASE;
		ObjectProperty _propOnDestroy;
		ObjectProperty _propOnDamage;
		ObjectProperty _propHealth;
		ObjectProperty _propMaxHealth;
	public:
		MyPropertySet(GC_Object *object);
		virtual int GetCount() const;
		virtual ObjectProperty* GetProperty(int index);
		virtual void MyExchange(World &world, bool applyToObject);
	};
	virtual PropertySet* NewPropertySet();

public:
	GC_RigidBodyStatic(World &world);
	GC_RigidBodyStatic(FromFile);
	virtual ~GC_RigidBodyStatic();

	virtual GC_Player* GetOwner() const { return NULL; }

	// GC_Object
	virtual void MapExchange(World &world, MapFile &f);
	virtual void Serialize(World &world, SaveFile &f);

	virtual unsigned char GetPassability() const = 0;

	float GetRadius() const { return _radius; }
	void AlignToTexture();

	float GetHalfWidth() const { return _width/2; }
	float GetHalfLength() const { return _length/2; }

	virtual bool CollideWithLine(const vec2d &lineCenter, const vec2d &lineDirection, vec2d &outEnterNormal, float &outEnter, float &outExit);
	virtual bool CollideWithRect(const vec2d &rectHalfSize, const vec2d &rectCenter, const vec2d &rectDirection, vec2d &outWhere, vec2d &outNormal, float &outDepth);

#ifdef NDEBUG
	__declspec(deprecated("Using this function is not recomended for Release build"))
#endif
	vec2d GetVertex(int index) const
	{
		float x, y;
		switch( index )
		{
		default: assert(false);
		case 0: x =  _length / 2; y =  _width / 2; break;
		case 1: x = -_length / 2; y =  _width / 2; break;
		case 2: x = -_length / 2; y = -_width / 2; break;
		case 3: x =  _length / 2; y = -_width / 2; break;
		}
		return vec2d(
			GetPos().x + x * GetDirection().x - y * GetDirection().y,
			GetPos().y + x * GetDirection().y + y * GetDirection().x
		);
	}


	//
	// health
	//

protected:
	float _health;
	float _health_max;

public:
	void SetHealth(float cur, float max);
	void SetHealthCur(float hp);
	void SetHealthMax(float hp);
	float GetHealth() const { return _health;     }
	float GetHealthMax() const { return _health_max; }

	virtual float GetDefaultHealth() const = 0;
	virtual void  OnDestroy(World &world);

	// return true if object has been killed
	virtual bool TakeDamage(World &world, float damage, const vec2d &hit, GC_Player *from);
	virtual void TDFV(GC_Actor *from);

	//
	// physics
	//
	void SetSize(float width, float length);
private:
	float _radius;     // cached radius of bounding sphere
	float _width;
	float _length;


	//--------------------------------

#ifdef NETWORK_DEBUG
public:
	virtual DWORD checksum(void) const
	{
		DWORD cs = reinterpret_cast<const DWORD&>(GetPos().x) ^ reinterpret_cast<const DWORD&>(GetPos().y);
		cs ^= reinterpret_cast<const DWORD&>(_health);
		cs ^= reinterpret_cast<const DWORD&>(_width) ^ reinterpret_cast<const DWORD&>(_length);
		return GC_2dSprite::checksum() ^ cs;
	}
#endif

};

/////////////////////////////////////////////////////////////

#define GC_FLAG_WALL_CORNER_1 (GC_FLAG_RBSTATIC_ << 0) //  0-----1
#define GC_FLAG_WALL_CORNER_2 (GC_FLAG_RBSTATIC_ << 1) //  |     |
#define GC_FLAG_WALL_CORNER_3 (GC_FLAG_RBSTATIC_ << 2) //  |     |
#define GC_FLAG_WALL_CORNER_4 (GC_FLAG_RBSTATIC_ << 3) //  3-----2
#define GC_FLAG_WALL_STYLE_BIT_0  (GC_FLAG_RBSTATIC_ << 4)
#define GC_FLAG_WALL_STYLE_BIT_1  (GC_FLAG_RBSTATIC_ << 5)
#define GC_FLAG_WALL_             (GC_FLAG_RBSTATIC_ << 6)

#define GC_FLAG_WALL_CORNER_ALL   (GC_FLAG_WALL_CORNER_1\
                                  |GC_FLAG_WALL_CORNER_2\
                                  |GC_FLAG_WALL_CORNER_3\
                                  |GC_FLAG_WALL_CORNER_4)


class GC_Wall : public GC_RigidBodyStatic
{
	DECLARE_SELF_REGISTRATION(GC_Wall);

private:
	void SetCorner(World &world, unsigned int index); // 01
	unsigned int GetCorner(void) const; // 32

	void SetStyle(int style); // 0-3
	int GetStyle() const;

protected:
	virtual const char *GetCornerTexture(int i);

protected:
	class MyPropertySet : public GC_RigidBodyStatic::MyPropertySet
	{
		typedef GC_RigidBodyStatic::MyPropertySet BASE;
		ObjectProperty _propCorner;
		ObjectProperty _propStyle;
	public:
		MyPropertySet(GC_Object *object);
		virtual int GetCount() const;
		virtual ObjectProperty* GetProperty(int index);
		virtual void MyExchange(World &world, bool applyToObject);
	};
	virtual PropertySet* NewPropertySet();

public:
	GC_Wall(World &world, float xPos, float yPos);
	GC_Wall(FromFile);
	virtual ~GC_Wall();

	virtual bool CollideWithLine(const vec2d &lineCenter, const vec2d &lineDirection, vec2d &outEnterNormal, float &outEnter, float &outExit);
	virtual bool CollideWithRect(const vec2d &rectHalfSize, const vec2d &rectCenter, const vec2d &rectDirection, vec2d &outWhere, vec2d &outNormal, float &outDepth);

	virtual float GetDefaultHealth() const { return 50; }

    virtual void Kill(World &world);
	virtual void Serialize(World &world, SaveFile &f);
	virtual void MapExchange(World &world, MapFile &f);

	virtual unsigned char GetPassability() const { return 1; }

	virtual void OnDestroy(World &world);
	virtual bool TakeDamage(World &world, float damage, const vec2d &hit, GC_Player *from);
};

/////////////////////////////////////////////////////////////

class GC_Wall_Concrete : public GC_Wall
{
	DECLARE_SELF_REGISTRATION(GC_Wall_Concrete);

protected:
	virtual const char *GetCornerTexture(int i);

public:
	GC_Wall_Concrete(World &world, float xPos, float yPos);
	GC_Wall_Concrete(FromFile) : GC_Wall(FromFile()) {};

	virtual unsigned char GetPassability() const { return 0xFF; } // impassable
	virtual bool TakeDamage(World &world, float damage, const vec2d &hit, GC_Player *from);
};

/////////////////////////////////////////////////////////////

class GC_Water : public GC_RigidBodyStatic
{
	DECLARE_SELF_REGISTRATION(GC_Water);
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
	GC_Water(World &world, float xPos, float yPos);
	GC_Water(FromFile);
	~GC_Water();

	void SetTile(char nTile, bool value);

    virtual void Kill(World &world);
	virtual void Serialize(World &world, SaveFile &f);

	virtual void Draw(bool editorMode) const;

	virtual unsigned char GetPassability() const { return 0xFF; }  // impassable
	virtual float GetDefaultHealth() const { return 0; }
	virtual bool TakeDamage(World &world, float damage, const vec2d &hit, GC_Player *from);
};

///////////////////////////////////////////////////////////////////////////////
// end of file

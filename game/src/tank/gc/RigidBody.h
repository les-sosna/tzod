// RigidBody.h

#pragma once

#include "2dSprite.h"

///////////////////////////////////////////////////////////////////////////////

#define GC_FLAG_RBSTATIC_TRACE0     (GC_FLAG_2DSPRITE_ << 0) // penetration of projectiles
#define GC_FLAG_RBSTATIC_DESTROYED  (GC_FLAG_2DSPRITE_ << 1) // object has been destroyed
#define GC_FLAG_RBSTATIC_PHANTOM    (GC_FLAG_2DSPRITE_ << 2)
#define GC_FLAG_RBSTATIC_           (GC_FLAG_2DSPRITE_ << 3)

class GC_RigidBodyStatic;

struct DamageDesc
{
	float  damage;
	vec2d  hit;
	GC_RigidBodyStatic *from;
};

class GC_RigidBodyStatic : public GC_2dSprite
{
	string_t _scriptOnDestroy;  // on_destroy()
	string_t _scriptOnDamage;   // on_damage()

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
		virtual void MyExchange(bool applyToObject);
	};
	virtual PropertySet* NewPropertySet();

public:
	GC_RigidBodyStatic();
	GC_RigidBodyStatic(FromFile);

	virtual void Kill();

	virtual void mapExchange(MapFile &f);
	virtual void Serialize(SaveFile &f);
	virtual bool IsSaved() const { return true; }
	virtual unsigned char GetPassability() const = 0;

	float GetRadius() const { return _radius; }
	void AlignToTexture();

	inline vec2d GetVertex(int index)
	{
		assert(index >= 0 && index < 4);
		return vec2d(
			GetPos().x + _vertices[index].x*_direction.x - _vertices[index].y*_direction.y,
			GetPos().y + _vertices[index].x*_direction.y + _vertices[index].y*_direction.x
		);
	}

	inline bool GetTrace0()
	{
		return CheckFlags(GC_FLAG_RBSTATIC_TRACE0);
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
	virtual void  OnDestroy();

	// return true if object has been killed
	virtual bool TakeDamage(float damage, const vec2d &hit, GC_RigidBodyStatic *from);


	//
	// physics
	//

	float _radius;     // cached radius of bounding sphere
	vec2d _direction;  // cached direction
	vec2d _vertices[4];


	//--------------------------------

#ifdef NETWORK_DEBUG
public:
	virtual DWORD checksum(void) const
	{
		DWORD cs = reinterpret_cast<const DWORD&>(GetPos().x) ^ reinterpret_cast<const DWORD&>(GetPos().y);
		cs ^= reinterpret_cast<const DWORD&>(_health);
		cs ^= reinterpret_cast<const DWORD&>(_direction.x) ^ reinterpret_cast<const DWORD&>(_direction.y);
		cs ^= reinterpret_cast<const DWORD&>(_vertices[0].x) ^ reinterpret_cast<const DWORD&>(_vertices[0].y);
		cs ^= reinterpret_cast<const DWORD&>(_vertices[1].x) ^ reinterpret_cast<const DWORD&>(_vertices[1].y);
		cs ^= reinterpret_cast<const DWORD&>(_vertices[2].x) ^ reinterpret_cast<const DWORD&>(_vertices[2].y);
		cs ^= reinterpret_cast<const DWORD&>(_vertices[3].x) ^ reinterpret_cast<const DWORD&>(_vertices[3].y);
		return GC_2dSprite::checksum() ^ cs;
	}
#endif

};

/////////////////////////////////////////////////////////////

#define GC_FLAG_WALL_CORNER_LT    (GC_FLAG_RBSTATIC_ << 0)
#define GC_FLAG_WALL_CORNER_RT    (GC_FLAG_RBSTATIC_ << 1)
#define GC_FLAG_WALL_CORNER_RB    (GC_FLAG_RBSTATIC_ << 2)
#define GC_FLAG_WALL_CORNER_LB    (GC_FLAG_RBSTATIC_ << 3)
#define GC_FLAG_WALL_             (GC_FLAG_RBSTATIC_ << 4)

#define GC_FLAG_WALL_CORNER_ALL (                     \
    GC_FLAG_WALL_CORNER_LT|GC_FLAG_WALL_CORNER_RT |   \
	GC_FLAG_WALL_CORNER_RB|GC_FLAG_WALL_CORNER_LB )


class GC_Wall : public GC_RigidBodyStatic
{
	DECLARE_SELF_REGISTRATION(GC_Wall);

private:
	void SetCorner(int index); // 0 means normal view
	int  GetCorner(void);

protected:
	virtual const char *GetCornerTexture(int i);

protected:
	class MyPropertySet : public GC_RigidBodyStatic::MyPropertySet
	{
		typedef GC_RigidBodyStatic::MyPropertySet BASE;
		ObjectProperty _propCorner;
	public:
		MyPropertySet(GC_Object *object);
		virtual int GetCount() const;
		virtual ObjectProperty* GetProperty(int index);
		virtual void MyExchange(bool applyToObject);
	};
	virtual PropertySet* NewPropertySet();

public:
	GC_Wall(float xPos, float yPos);
	GC_Wall(FromFile);
	virtual ~GC_Wall();

	virtual void Kill();

	virtual float GetDefaultHealth() const { return 50; }

	virtual void Serialize(SaveFile &f);
	virtual void mapExchange(MapFile &f);

	virtual unsigned char GetPassability() const { return 1; }

	virtual void OnDestroy();
	virtual bool TakeDamage(float damage, const vec2d &hit, GC_RigidBodyStatic *from);

	virtual void EditorAction();
};

/////////////////////////////////////////////////////////////

class GC_Wall_Concrete : public GC_Wall
{
	DECLARE_SELF_REGISTRATION(GC_Wall_Concrete);

protected:
	virtual const char *GetCornerTexture(int i);

public:
	GC_Wall_Concrete(float xPos, float yPos);
	GC_Wall_Concrete(FromFile) : GC_Wall(FromFile()) {};

	virtual unsigned char GetPassability() const { return 0xFF; } // непроходимое препятствие
	virtual bool TakeDamage(float damage, const vec2d &hit, GC_RigidBodyStatic *from);
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
	BYTE _tile;

protected:
	void UpdateTile(bool flag);

public:
	GC_Water(float xPos, float yPos);
	GC_Water(FromFile);
	~GC_Water();

	void SetTile(char nTile, bool value);

	virtual void Kill();
	virtual void Serialize(SaveFile &f);

	virtual void Draw() const;

	virtual unsigned char GetPassability() const { return 0xFF; }  // непроходимое препятствие
	virtual float GetDefaultHealth() const { return 0; }
	virtual bool TakeDamage(float damage, const vec2d &hit, GC_RigidBodyStatic *from);
};

///////////////////////////////////////////////////////////////////////////////
// end of file

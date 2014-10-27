// RigidBody.h

#pragma once

#include "Actor.h"
#include "NeighborAware.h"


#define GC_FLAG_RBSTATIC_TRACE0     (GC_FLAG_ACTOR_ << 0) // penetration of projectiles
#define GC_FLAG_RBSTATIC_DESTROYED  (GC_FLAG_ACTOR_ << 1)
#define GC_FLAG_RBSTATIC_           (GC_FLAG_ACTOR_ << 2)

class GC_Player;

struct DamageDesc
{
	float  damage;
	vec2d  hit;
	GC_Player *from;
};

class GC_RigidBodyStatic : public GC_Actor
{
	DECLARE_GRID_MEMBER();
    typedef GC_Actor base;

public:
	explicit GC_RigidBodyStatic(vec2d pos);
	explicit GC_RigidBodyStatic(FromFile);
	virtual ~GC_RigidBodyStatic();
	
	const std::string& GetOnDestroy() const { return _scriptOnDestroy; }
	const std::string& GetOnDamage() const { return _scriptOnDamage; }
	
	void SetHealth(float cur, float max);
	void SetHealth(float hp);
	void SetHealthMax(float hp);
	float GetHealth() const { return _health;     }
	float GetHealthMax() const { return _health_max; }
	
	void SetSize(float width, float length);
	float GetHalfWidth() const { return _width/2; }
	float GetHalfLength() const { return _length/2; }
	float GetRadius() const { return _radius; }
	vec2d GetVertex(int index) const;
	bool GetTrace0() const { return CheckFlags(GC_FLAG_RBSTATIC_TRACE0); }
	
	virtual bool CollideWithLine(const vec2d &lineCenter, const vec2d &lineDirection, vec2d &outEnterNormal, float &outEnter, float &outExit);
	virtual bool CollideWithRect(const vec2d &rectHalfSize, const vec2d &rectCenter, const vec2d &rectDirection, vec2d &outWhere, vec2d &outNormal, float &outDepth);
	
	// return true if object has been killed
	void TakeDamage(World &world, DamageDesc dd);
	
	virtual float GetDefaultHealth() const = 0;
	virtual unsigned char GetPassability() const = 0;
	virtual GC_Player* GetOwner() const { return NULL; }
    
    // GC_Actor
    virtual void MoveTo(World &world, const vec2d &pos);

	// GC_Object
	virtual void Init(World &world) override;
    virtual void Kill(World &world) override;
	virtual void MapExchange(MapFile &f) override;
	virtual void Serialize(World &world, SaveFile &f) override;

protected:
	class MyPropertySet : public GC_Actor::MyPropertySet
	{
		typedef GC_Actor::MyPropertySet BASE;
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
	virtual void OnDestroy(World &world, GC_Player *by);
	virtual void OnDamage(World &world, DamageDesc &damageDesc);

private:
	float _health;
	float _health_max;
	float _radius;     // cached radius of bounding sphere
	float _width;
	float _length;
	std::string _scriptOnDestroy;  // on_destroy()
	std::string _scriptOnDamage;   // on_damage()

#ifdef NETWORK_DEBUG
public:
	virtual DWORD checksum(void) const
	{
		DWORD cs = reinterpret_cast<const DWORD&>(GetPos().x) ^ reinterpret_cast<const DWORD&>(GetPos().y);
		cs ^= reinterpret_cast<const DWORD&>(_health);
		cs ^= reinterpret_cast<const DWORD&>(_width) ^ reinterpret_cast<const DWORD&>(_length);
		return GC_Actor::checksum() ^ cs;
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
	DECLARE_GRID_MEMBER();
	DECLARE_SELF_REGISTRATION(GC_Wall);
    typedef GC_RigidBodyStatic base;

public:
	explicit GC_Wall(vec2d pos);
	GC_Wall(FromFile);
	virtual ~GC_Wall();
	
	void SetCorner(World &world, unsigned int index); // 01
	unsigned int GetCorner(void) const; // 32
	
	void SetStyle(int style); // 0-3
	int GetStyle() const;
	
	// GC_RigidBodyStatic
	virtual bool CollideWithLine(const vec2d &lineCenter, const vec2d &lineDirection, vec2d &outEnterNormal, float &outEnter, float &outExit);
	virtual bool CollideWithRect(const vec2d &rectHalfSize, const vec2d &rectCenter, const vec2d &rectDirection, vec2d &outWhere, vec2d &outNormal, float &outDepth);
	virtual float GetDefaultHealth() const { return 50; }
	virtual unsigned char GetPassability() const { return 1; }

	// GC_Object
	virtual void Init(World &world) override;
    virtual void Kill(World &world) override;
	virtual void MapExchange(MapFile &f) override;
	virtual void Serialize(World &world, SaveFile &f) override;

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
	
	virtual void OnDestroy(World &world, GC_Player *by) override;
	virtual void OnDamage(World &world, DamageDesc &dd) override;
};

/////////////////////////////////////////////////////////////

class GC_Wall_Concrete : public GC_Wall
{
	DECLARE_SELF_REGISTRATION(GC_Wall_Concrete);

public:
	GC_Wall_Concrete(vec2d pos);
	GC_Wall_Concrete(FromFile) : GC_Wall(FromFile()) {}

	virtual unsigned char GetPassability() const { return 0xFF; } // impassable
	
protected:
	virtual void OnDamage(World &world, DamageDesc &dd) override;
};

/////////////////////////////////////////////////////////////

#define GC_FLAG_WATER_INTILE        (GC_FLAG_RBSTATIC_ << 0)
#define GC_FLAG_WATER_              (GC_FLAG_RBSTATIC_ << 1)

class GC_Water : public GC_RigidBodyStatic
               , public GI_NeighborAware
{
	DECLARE_GRID_MEMBER();
	DECLARE_SELF_REGISTRATION(GC_Water);
    typedef GC_RigidBodyStatic base;

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
	GC_Water(vec2d pos);
	GC_Water(FromFile);
	~GC_Water();

	void SetTile(char nTile, bool value);

    virtual void MoveTo(World &world, const vec2d &pos) override;
    virtual void Kill(World &world);
	virtual void Serialize(World &world, SaveFile &f);

	// GI_NeighborAware
	virtual int GetNeighbors() const override { return _tile; }

	virtual unsigned char GetPassability() const { return 0xFF; }  // impassable
	virtual float GetDefaultHealth() const { return 0; }

protected:
	virtual void OnDamage(World &world, DamageDesc &dd) override;
};

///////////////////////////////////////////////////////////////////////////////
// end of file

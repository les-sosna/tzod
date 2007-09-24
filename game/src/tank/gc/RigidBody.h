// RigidBody.h

#pragma once

#include "2dSprite.h"

///////////////////////////////////////////////////////////////////////////////

#define GC_FLAG_RBSTATIC_TRACE0     (GC_FLAG_2DSPRITE_ << 0) // penetration of projectiles
#define GC_FLAG_RBSTATIC_DESTROYED  (GC_FLAG_2DSPRITE_ << 1) // object has been destroyed
#define GC_FLAG_RBSTATIC_           (GC_FLAG_2DSPRITE_ << 2)

class GC_RigidBodyStatic;

struct DamageDesc
{
	float  damage;
	vec2d  hit;
	GC_RigidBodyStatic *from;
};

class GC_RigidBodyStatic : public GC_2dSprite
{
	string_t _scriptOnDestroy;  // OnDestroy()

protected:
	class MyPropertySet : public GC_2dSprite::MyPropertySet
	{
		typedef GC_2dSprite::MyPropertySet BASE;
		ObjectProperty _propOnDestroy;
		ObjectProperty _propHealth;
		ObjectProperty _propMaxHealth;
	public:
		MyPropertySet(GC_Object *object);
		virtual int GetCount() const;
		virtual ObjectProperty* GetProperty(int index);
		virtual void Exchange(bool applyToObject);
	};

public:
	GC_RigidBodyStatic::GC_RigidBodyStatic();
	GC_RigidBodyStatic::GC_RigidBodyStatic(FromFile);

	virtual void MoveTo(const vec2d &pos);
	virtual void Kill();

	virtual SafePtr<PropertySet> GetProperties();
	virtual void mapExchange(MapFile &f);
	virtual void Serialize(SaveFile &f);
	virtual unsigned char GetPassability() const = 0;

	float GetRadius() const { return _radius; }
	void AlignToTexture();

	inline vec2d GetVertex(int index)
	{
		_ASSERT(index >= 0 && index < 4);
		return vec2d(
			GetPos().x + _vertices[index].x*_direction.x - _vertices[index].y*_direction.y,
			GetPos().y + _vertices[index].x*_direction.y + _vertices[index].y*_direction.x
		);
	}

	inline bool Trace0()
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
	// phisics
	//

	float _radius;   // radius of bounding sphere
	vec2d _direction;
	vec2d _vertices[4];


	//--------------------------------

#ifdef NETWORK_DEBUG
public:
	virtual DWORD checksum(void) const
	{
		DWORD cs = reinterpret_cast<const DWORD&>(GetPos().x) ^ reinterpret_cast<const DWORD&>(GetPos().y);
		cs ^= reinterpret_cast<const DWORD&>(_direction.x) ^ reinterpret_cast<const DWORD&>(_direction.y);
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
	void SetCornerView(int index); // 0 means normal view
	int  GetCornerView(void);

protected:
	virtual const char *getCornerTexture(int i);

public:
	GC_Wall(float xPos, float yPos);
	GC_Wall(FromFile);


	virtual float GetDefaultHealth() const { return 50; }

	virtual bool IsSaved() { return true; }
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
	virtual const char *getCornerTexture(int i);

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

	virtual void Kill();
	virtual bool IsSaved() { return true; };
	virtual void Serialize(SaveFile &f);

	virtual void Draw();

	virtual unsigned char GetPassability() const { return 0xFF; }  // непроходимое препятствие

	virtual float GetDefaultHealth() const { return 0; }

	virtual bool TakeDamage(float damage, const vec2d &hit, GC_RigidBodyStatic *from);

public:
	void SetTile(char nTile, bool value);
};

/////////////////////////////////////////////////////////////

#define GC_FLAG_RBDYMAMIC_ACTIVE    (GC_FLAG_RBSTATIC_ << 0)
#define GC_FLAG_RBDYMAMIC_PARITY    (GC_FLAG_RBSTATIC_ << 1)
#define GC_FLAG_RBDYMAMIC_          (GC_FLAG_RBSTATIC_ << 2)


class GC_RigidBodyDynamic : public GC_RigidBodyStatic
{
	struct Contact
	{
		GC_RigidBodyDynamic *obj1_d;
		GC_RigidBodyDynamic *obj2_d;
		GC_RigidBodyStatic  *obj2_s;
		vec2d o,n,t; // origin, normal, tangent
		float total_np, total_tp;
//		bool  inactive;
	};

	typedef std::vector<Contact> ContactList;
	static ContactList _contacts;
	static bool _glob_parity;

	bool Intersect(GC_RigidBodyStatic *pObj, vec2d &origin, vec2d &normal);

	float geta_s(const vec2d &n, const vec2d &c, const GC_RigidBodyStatic *obj) const;
	float geta_d(const vec2d &n, const vec2d &c, const GC_RigidBodyDynamic *obj) const;

	void impulse(const vec2d &origin, const vec2d &impulse);

	bool parity() { return CheckFlags(GC_FLAG_RBDYMAMIC_PARITY); }


	vec2d _external_force;
	float _external_momentum;

	vec2d _external_impulse;
	float _external_torque;


	void apply_external_forces(float dt);

public:
	GC_RigidBodyDynamic::GC_RigidBodyDynamic();
	GC_RigidBodyDynamic::GC_RigidBodyDynamic(FromFile);

	virtual void Serialize(SaveFile &f);
	virtual void TimeStepFixed(float dt);
	static void ProcessResponse(float dt);

	float Energy() const;


	float _angle;   // current rotation
	float _av;      // angular velocity
	vec2d _lv;      // linear velocity

	float _inv_m;   // 1/mass
	float _inv_i;   // 1/moment_of_inertia

//	float _Nb;      // braking friction factor X

	float _Nx;      // dry friction factor X
	float _Ny;      // dry friction factor Y
	float _Nw;      // angilar dry friction factor

	float _Mx;      // viscous friction factor X
	float _My;      // viscous friction factor Y
	float _Mw;      // angular viscous friction factor

	float _percussion;  // percussion factor
	float _fragility;   // fragility factor (0 - invulnerability)

	//--------------------------------

	void ApplyMomentum(float momentum);
	void ApplyForce(const vec2d &force);
	void ApplyForce(const vec2d &force, const vec2d &origin);

	void ApplyTorque(float torque);
	void ApplyImpulse(const vec2d &impulse);
	void ApplyImpulse(const vec2d &impulse, const vec2d &origin);

	void SetBodyAngle(float a);


	//--------------------------------

#ifdef NETWORK_DEBUG
public:
	virtual DWORD checksum(void) const
	{
		DWORD cs = reinterpret_cast<const DWORD&>(_av);
		cs ^= reinterpret_cast<const DWORD&>(_lv.x);
		cs ^= reinterpret_cast<const DWORD&>(_lv.y);
		cs ^= reinterpret_cast<const DWORD&>(_health);
		return GC_RigidBodyStatic::checksum() ^ cs;
	}
#endif

};

///////////////////////////////////////////////////////////////////////////////
// end of file

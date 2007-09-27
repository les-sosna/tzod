// RigidBodyDynamic.cpp

#include "stdafx.h"
#include "RigidBodyDinamic.h"
#include "Projectiles.h"
#include "Sound.h"

#include "fs/SaveFile.h"
#include "fs/MapFile.h"

#include "level.h"


///////////////////////////////////////////////////////////////////////////////

GC_RigidBodyDynamic::MyPropertySet::MyPropertySet(GC_Object *object)
  : BASE(object)
  , _propM(ObjectProperty::TYPE_FLOAT, "M")
  , _propI(ObjectProperty::TYPE_FLOAT, "I")
  , _propPercussion(ObjectProperty::TYPE_FLOAT, "percussion")
  , _propFragility(ObjectProperty::TYPE_FLOAT, "fragility")
  , _propNx(ObjectProperty::TYPE_FLOAT, "Nx")
  , _propNy(ObjectProperty::TYPE_FLOAT, "Ny")
  , _propNw(ObjectProperty::TYPE_FLOAT, "Nw")
{
	_propM.SetFloatRange(0, 10000);
	_propI.SetFloatRange(0, 10000);
	_propPercussion.SetFloatRange(0, 10000);
	_propFragility.SetFloatRange(0, 10000);
	_propNx.SetFloatRange(0, 10000);
	_propNy.SetFloatRange(0, 10000);
	_propNw.SetFloatRange(0, 10000);
}

int GC_RigidBodyDynamic::MyPropertySet::GetCount() const
{
	return BASE::GetCount() + 7;
}

ObjectProperty* GC_RigidBodyDynamic::MyPropertySet::GetProperty(int index)
{
	if( index < BASE::GetCount() )
		return BASE::GetProperty(index);

	switch( index - BASE::GetCount() )
	{
	case 0: return &_propM;
	case 1: return &_propI;
	case 2: return &_propPercussion;
	case 3: return &_propFragility;
	case 4: return &_propNx;
	case 5: return &_propNy;
	case 6: return &_propNw;
	}

	_ASSERT(FALSE);
	return NULL;
}

void GC_RigidBodyDynamic::MyPropertySet::Exchange(bool applyToObject)
{
	BASE::Exchange(applyToObject);

	GC_RigidBodyDynamic *tmp = static_cast<GC_RigidBodyDynamic *>(GetObject());
	if( applyToObject )
	{
		tmp->_inv_m = 1.0f / _propM.GetFloatValue();
		tmp->_inv_i = 1.0f / _propI.GetFloatValue();
		tmp->_percussion = _propPercussion.GetFloatValue();
		tmp->_fragility = _propFragility.GetFloatValue();
		tmp->_Nx = _propNx.GetFloatValue();
		tmp->_Ny = _propNy.GetFloatValue();
		tmp->_Nw = _propNw.GetFloatValue();
	}
	else
	{
		_propM.SetFloatValue(1.0f / tmp->_inv_m);
		_propI.SetFloatValue(1.0f / tmp->_inv_i);
		_propPercussion.SetFloatValue(tmp->_percussion);
		_propFragility.SetFloatValue(tmp->_fragility);
		_propNx.SetFloatValue(tmp->_Nx);
		_propNy.SetFloatValue(tmp->_Ny);
		_propNw.SetFloatValue(tmp->_Nw);
	}
}

///////////////////////////////////////////////////////////////////////////////

std::vector<GC_RigidBodyDynamic::Contact> GC_RigidBodyDynamic::_contacts;
bool GC_RigidBodyDynamic::_glob_parity = false;

GC_RigidBodyDynamic::GC_RigidBodyDynamic() : GC_RigidBodyStatic()
{
	_lv.Zero();
	_av     = 0;
	_inv_m  = 1.0f / 1;
	_inv_i  = _inv_m * 12.0f / (36*36 + 36*36);
	_angle  = 0;

	_Nx     = 0;
	_Ny     = 0;
	_Nw     = 0;

	_Mx     = 0;
	_My     = 0;
	_Mw     = 0;

	_percussion = 1;
	_fragility  = 1;

	_external_force.Zero();
	_external_momentum = 0;

	_external_impulse.Zero();
	_external_torque = 0;


	if( _glob_parity ) SetFlags(GC_FLAG_RBDYMAMIC_PARITY);
	SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FIXED);
}

GC_RigidBodyDynamic::GC_RigidBodyDynamic(FromFile) : GC_RigidBodyStatic(FromFile())
{
}

PropertySet* GC_RigidBodyDynamic::NewPropertySet()
{
	return new MyPropertySet(this);
}

void GC_RigidBodyDynamic::mapExchange(MapFile &f)
{
	GC_RigidBodyStatic::mapExchange(f);

	MAP_EXCHANGE_FLOAT(inv_m, _inv_m, 1);
	MAP_EXCHANGE_FLOAT(inv_i, _inv_i, 1);
	MAP_EXCHANGE_FLOAT(percussion, _percussion, 1);
	MAP_EXCHANGE_FLOAT(fragility, _fragility, 1);
	MAP_EXCHANGE_FLOAT(Nx, _Nx, 0);
	MAP_EXCHANGE_FLOAT(Ny, _Ny, 0);
	MAP_EXCHANGE_FLOAT(Nw, _Nw, 0);
}

void GC_RigidBodyDynamic::Serialize(SaveFile &f)
{
	GC_RigidBodyStatic::Serialize(f);

	f.Serialize(_av);
	f.Serialize(_lv);
	f.Serialize(_inv_m);
	f.Serialize(_inv_i);
	f.Serialize(_angle);
	f.Serialize(_Nx);
	f.Serialize(_Ny);
	f.Serialize(_Nw);
	f.Serialize(_Mx);
	f.Serialize(_My);
	f.Serialize(_Mw);
	f.Serialize(_percussion);
	f.Serialize(_external_force);
	f.Serialize(_external_momentum);
	f.Serialize(_external_impulse);
	f.Serialize(_external_torque);
}

bool GC_RigidBodyDynamic::Intersect(GC_RigidBodyStatic *pObj, vec2d &origin, vec2d &normal)
{
	if( (pObj->GetPos() - GetPos()).len() > GetRadius() + pObj->GetRadius() )
	{
		return false;
	}


	//
	// exact intersection testing
	//

	vec2d verts1[4];
	vec2d verts2[4];

	for( int i = 0; i < 4; ++i )
	{
		verts1[i] = GetVertex(i);
		verts2[i] = pObj->GetVertex(i);
	}

	float ax1, ay1, ax2, ay2;
	float bx1, by1, bx2, by2;
	float ax, ay, bx, by;

	float det, ta, tb;

	vec2d pt[2];
	vec2d ns[2];
	int icount = 0;

	for( int i = 0; i < 4; ++i )
	for( int j = 0; j < 4; ++j )
	{
		ax1 = verts1[i].x;
		ay1 = verts1[i].y;
		ax2 = verts1[(i+1)&3].x;
		ay2 = verts1[(i+1)&3].y;
		bx1 = verts2[j].x;
		by1 = verts2[j].y;
		bx2 = verts2[(j+1)&3].x;
		by2 = verts2[(j+1)&3].y;

		ax = ax2 - ax1;
		ay = ay2 - ay1;
		bx = bx2 - bx1;
		by = by2 - by1;

		if( fabsf(det = ay*bx - ax*by) > 1e-7 )
		{
			ta = ( by*(ax1-bx1) + bx*(by1-ay1) ) / det;
			if( ta < 0 || ta > 1 )
				continue;

			tb = ( ay*(ax1-bx1) + ax*(by1-ay1) ) / det;
			if( tb < 0 || tb > 1 )
				continue;

			origin.x = ax1 + ax * ta;
			origin.y = ay1 + ay * ta;

			pt[icount++] = origin;

			if( 2 == icount )
			{
				origin = (pt[0] + pt[1]) * 0.5f;

				normal.x = pt[1].y - pt[0].y;
				normal.y = pt[0].x - pt[1].x;

				if( normal.x * (origin.x - GetPos().x) +
					normal.y * (origin.y - GetPos().y) > 0 )
				{
					normal.x = -normal.x;
					normal.y = -normal.y;
				}


				float len = normal.len();
				if( len < 1e-7 )
				{
					return false;
				}

				normal /= len;
				return true;
			}

			if( fabsf(ta - 0.5f) < fabsf(tb - 0.5f) )
			{
				normal.x = -ay;
				normal.y =  ax;
			}
			else
			{
				normal.x =  by;
				normal.y = -bx;
			}
			normal.Normalize();
		}
		else
		{
		}
	}

	return (icount > 0);
}

void GC_RigidBodyDynamic::TimeStepFixed(float dt)
{
	vec2d dx = _lv * dt;
	float da = _av * dt;


	//
	// защита от пролетания снарядов через объект
	//

	AddRef();
	{
		float s = sinf(da);
		float c = cosf(da) - 1;
		const OBJECT_LIST &ls = g_level->GetList(LIST_projectiles);
		OBJECT_LIST::safe_iterator it = ls.safe_begin();
		while( it != ls.end() )
		{
			GC_Projectile* pProj = static_cast<GC_Projectile*>(*it);

			vec2d delta, tmp = pProj->GetPos() - GetPos();
			delta.x = tmp.x * c - tmp.y * s - dx.x;
			delta.y = tmp.x * s - tmp.y * c - dx.y;

			float dl_sq = delta.sqr();
			if( dl_sq < 1 ) delta /= sqrtf(dl_sq);

			pProj->SpecialTrace(this, delta);
			++it;

			if( IsKilled() )
			{
				Release();
				return;
			}
		}
	}
	Release();



	MoveTo(GetPos() + dx);
	_angle = fmodf(_angle + da, PI2);
	if( _angle < 0 ) _angle += PI2;


	_direction = vec2d(_angle);
	SetRotation(_angle);

	//------------------------------------

	apply_external_forces(dt);

	//------------------------------------
	// friction

	vec2d dir_y(_direction.y, -_direction.x);

	float vx = _lv.x * _direction.x + _lv.y * _direction.y;
	float vy = _lv.x * _direction.y - _lv.y * _direction.x;

	_lv.Normalize();
	vec2d dev(_lv * _direction, _lv * dir_y);

	if( vx > 0 )
		vx = __max(0, vx - _Nx * dt * dev.x);
	else
		vx = __min(0, vx - _Nx * dt * dev.x);
	vx *= expf(-_Mx * dt);

	if( vy > 0 )
		vy = __max(0, vy - _Ny * dt * dev.y);
	else
		vy = __min(0, vy - _Ny * dt * dev.y);
	vy *= expf(-_My * dt);

	_lv = _direction * vx + dir_y * vy;


	if( _av > 0 )
		_av = __max(0, _av - _Nw * dt);
	else
		_av = __min(0, _av + _Nw * dt);
	_av *= expf(-_Mw * dt);


	//------------------------------------
	// collisions

	std::vector<OBJECT_LIST*> receive;
	g_level->grid_rigid_s.OverlapCircle(receive,
		GetPos().x / LOCATION_SIZE, GetPos().y / LOCATION_SIZE, 0);

	g_level->grid_water.OverlapCircle(receive,
		GetPos().x / LOCATION_SIZE, GetPos().y / LOCATION_SIZE, 0);

	std::vector<OBJECT_LIST*>::iterator rit = receive.begin();

	Contact c;
	c.total_np = 0;
	c.total_tp = 0;
	c.obj1_d   = this;

	for( ; rit != receive.end(); ++rit )
	{
		OBJECT_LIST::iterator it = (*rit)->begin();
		for( ; it != (*rit)->end(); ++it )
		{
			GC_RigidBodyStatic *object = (GC_RigidBodyStatic *) (*it);
			if( this == object ) continue;

//			if( object->parity() != _glob_parity )
			{
				if( Intersect(object, c.o, c.n) )
				{
					c.t.x =  c.n.y;
					c.t.y = -c.n.x;
					c.obj2_s = object;
					c.obj2_d = dynamic_cast<GC_RigidBodyDynamic*>(object);

					c.obj1_d->AddRef();
					c.obj2_s->AddRef();

					_contacts.push_back(c);
				}
			}
		}
	}
}

float GC_RigidBodyDynamic::geta_s(const vec2d &n, const vec2d &c,
								  const GC_RigidBodyStatic *obj) const
{
	float k1 = n.x*(c.y-GetPos().y) - n.y*(c.x-GetPos().x);
	return (float)
		(
		2 * ( _av * k1 - n.y*_lv.y - n.x*_lv.x ) /
		( k1*k1*_inv_i + n.sqr() * _inv_m )
		);
}

float GC_RigidBodyDynamic::geta_d(const vec2d &n, const vec2d &c,
								  const GC_RigidBodyDynamic *obj) const
{
	float k1 = n.x*(c.y-GetPos().y) - n.y*(c.x-GetPos().x);
	float k2 = n.y*(c.x-obj->GetPos().x) - n.x*(c.y-obj->GetPos().y);
	return (float)
		(
		2 * ( n.y*(obj->_lv.y-_lv.y) + n.x*(obj->_lv.x-_lv.x) + _av*k1 + obj->_av*k2 ) /
		( k1*k1*_inv_i + k2*k2*obj->_inv_i + n.sqr() * (_inv_m+obj->_inv_m) )
		);
}

void GC_RigidBodyDynamic::ProcessResponse(float dt)
{
	for( int i = 0; i < 128; i++ )
	{
		for( ContactList::iterator it = _contacts.begin(); it != _contacts.end(); ++it )
		{
			if( it->obj1_d->IsKilled() || it->obj2_s->IsKilled() ) continue;

			float a;
			if( it->obj2_d )
				a = 0.65f * it->obj1_d->geta_d(it->n, it->o, it->obj2_d );
			else
				a = 0.65f * it->obj1_d->geta_s(it->n, it->o, it->obj2_s );

			if( a >= 0 )
			{
				a = __max(0.01f * (float) (i>>2), a);
				it->total_np += a;

				float nd = it->total_np/60;

				if( nd > 3 )
				{
					if( it->obj2_d )
					{
						it->obj1_d->TakeDamage(a/60 * it->obj2_d->_percussion *
							it->obj1_d->_fragility, it->o, it->obj2_d);
						it->obj2_d->TakeDamage(a/60 * it->obj1_d->_percussion *
							it->obj2_d->_fragility, it->o, it->obj1_d);
					}
					else
					{
						it->obj1_d->TakeDamage(a/60 * it->obj1_d->_fragility, it->o, it->obj1_d);
						it->obj2_s->TakeDamage(a/60 * it->obj1_d->_percussion, it->o, it->obj1_d);
					}
				}

				if( it->obj1_d->IsKilled() || it->obj2_s->IsKilled() )
					a *= 0.1f;

				vec2d delta_p = it->n * a;
				it->obj1_d->impulse(it->o, delta_p);
				if( it->obj2_d ) it->obj2_d->impulse(it->o, -delta_p);


				//
				// трение
				//

				const float N = 0.1f;

				float maxb;
				if( it->obj2_d )
					maxb = 0.5f * it->obj1_d->geta_d(it->t, it->o, it->obj2_d );
				else
					maxb = 0.5f * it->obj1_d->geta_s(it->t, it->o, it->obj2_s );

				float signb = maxb > 0 ? 1.0f : -1.0f;
				if( (maxb = fabsf(maxb)) > 0 )
				{
					float b = __min(maxb, a*N);
					it->total_tp += b;
					delta_p = it->t * b * signb;
					it->obj1_d->impulse(it->o,  delta_p);
					if( it->obj2_d ) it->obj2_d->impulse(it->o, -delta_p);
				}
			}
		}
	}

	for( ContactList::iterator it = _contacts.begin(); it != _contacts.end(); ++it )
	{
		float nd = (it->total_np + it->total_tp)/60;

		if( nd > 3 )
		{
			if( nd > 10 )
				PLAY(SND_Impact2, it->o);
			else
				PLAY(SND_Impact1, it->o);
		}
		else
			if( it->total_tp > 10 )
			{
				PLAY(SND_Slide1, it->o);
			}

			/////////////////////////////////////
			it->obj1_d->Release();
			it->obj2_s->Release();
	}

	_contacts.clear();
}

void GC_RigidBodyDynamic::impulse(const vec2d &origin, const vec2d &impulse)
{
	_lv += impulse * _inv_m;
	_av += ((origin.x-GetPos().x)*impulse.y-(origin.y-GetPos().y)*impulse.x) * _inv_i;
}

void GC_RigidBodyDynamic::apply_external_forces(float dt)
{
	_lv += (_external_force * dt + _external_impulse) * _inv_m;
	_av += (_external_momentum * dt + _external_torque) * _inv_i;
	_external_force.Zero();
	_external_impulse.Zero();
	_external_momentum = 0;
	_external_torque = 0;
}

void GC_RigidBodyDynamic::ApplyMomentum(float momentum)
{
	_external_momentum += momentum;
}

void GC_RigidBodyDynamic::ApplyForce(const vec2d &force)
{
	_external_force += force;
}

void GC_RigidBodyDynamic::ApplyForce(const vec2d &force, const vec2d &origin)
{
	_external_force += force;
	_external_momentum += (origin.x-GetPos().x)*force.y-(origin.y-GetPos().y)*force.x;
}

void GC_RigidBodyDynamic::ApplyImpulse(const vec2d &impulse, const vec2d &origin)
{
	_external_impulse += impulse;
	_external_torque  += (origin.x-GetPos().x)*impulse.y-(origin.y-GetPos().y)*impulse.x;
}

void GC_RigidBodyDynamic::ApplyImpulse(const vec2d &impulse)
{
	_external_impulse += impulse;
}

void GC_RigidBodyDynamic::ApplyTorque(float torque)
{
	_external_torque  += torque;
}

void GC_RigidBodyDynamic::SetBodyAngle(float a)
{
	_angle = a;
	_direction = vec2d(_angle);
	SetRotation(_angle);
}

float GC_RigidBodyDynamic::Energy() const
{
	float e = 0;
	if( _inv_i != 0 ) e  = _av*_av / _inv_i;
	if( _inv_m != 0 ) e += _lv.sqr() / _inv_m;
	return e;
}

// end of file

// Rotator.cpp: implementation of the Rotator class.

#include "stdafx.h"
#include "rotator.h"

#include "fs/SaveFile.h"

#include "gc/sound.h"

//////////////////////////////////////////////////////////////////////

// передается ссылка на управлюемую переменную.
// Rotator будет автоматически изменять ее значение
Rotator::Rotator(float &angle)
  : _rCurrent(angle)
  , _velocity_current(0)
{
}

Rotator::~Rotator()
{
}

//-------------------------------------------------------------------

void Rotator::reset(float angle, float velocity, float limit, float current, float stop)
{
	_rCurrent         = angle;
	_velocity_current = velocity;
	setl(limit, current, stop);
	_state = RS_STOPPED;
}

void Rotator::setl(float limit, float current, float stop)
{
	_ASSERT(current > 0);
	_ASSERT(stop  > 0);
	_ASSERT(limit > 0);

	_velocity_limit   = limit;
	_accel_current    = current;
	_accel_stop       = stop;

	if( _velocity_current >  limit ) _velocity_current =  limit;
	if( _velocity_current < -limit ) _velocity_current = -limit;
}

//-------------------------------------------------------------------

void Rotator::rotate_to(float new_target)
{
	_ASSERT(!_isnan(new_target) && _finite(new_target));
	_ASSERT(_accel_current > 0);
	_ASSERT(_accel_stop > 0);

	new_target = fmodf(new_target, PI2);
	if( new_target < 0 ) new_target += PI2;

	if( new_target == _rCurrent && RS_STOPPED == _state ) return;

	_angle_target = new_target;
	_state = RS_GETTING_ANGLE;
}

void Rotator::rotate_left()
{
	_ASSERT(_accel_current > 0);
	_ASSERT(_accel_stop > 0);
	_state = RS_LEFT;
}

void Rotator::rotate_right()
{
	_ASSERT(_accel_current > 0);
	_ASSERT(_accel_stop > 0);
	_state = RS_RIGHT;
}

void Rotator::stop()
{
	_ASSERT(_accel_stop > 0);

	if( RS_STOPPED != _state )
	{
		_state = RS_DEACTIVATED;
	}
}

void Rotator::impulse(float dv)
{
	_velocity_current += dv;

	if( _velocity_current >  _velocity_limit ) _velocity_current =  _velocity_limit;
	if( _velocity_current < -_velocity_limit ) _velocity_current = -_velocity_limit;

	if( RS_STOPPED == _state ) _state = RS_DEACTIVATED;
}

//-------------------------------------------------------------------

bool Rotator::process_dt(float dt)
{
	float old = _rCurrent;


	switch (_state)
	{
	case RS_RIGHT:
		if( _velocity_current < 0 )
		{
			if( -_velocity_current < dt * _accel_stop )
			{
				float new_v = _accel_current * (dt + _velocity_current / _accel_stop);
				if( new_v > _velocity_limit )
				{
					_rCurrent += -(_velocity_current * (_velocity_current - 2.0f * _velocity_limit)) /
						(2.0f * _accel_stop) + dt * _velocity_limit -
						(_velocity_limit * _velocity_limit) / (2.0f * _accel_current);
					_velocity_current = _velocity_limit;
				}
				else
				{
					_rCurrent += (_accel_current*powf(_accel_stop * dt + _velocity_current, 2) -
						_accel_stop * _velocity_current * _velocity_current) /
						(2.0f * _accel_stop * _accel_stop);
					_velocity_current = new_v;
				}
			}
			else
			{
				_rCurrent += dt * (_velocity_current + dt * _accel_stop * 0.5f);
				_velocity_current = _velocity_current + dt * _accel_stop;
			}
		}
		else
		{
			float new_v = _velocity_current + dt * _accel_current;
			if( new_v > _velocity_limit )
			{
				_rCurrent += (_accel_current * dt * _velocity_limit -
					0.5f * powf(_velocity_current - _velocity_limit,2)) / _accel_current;

				_velocity_current = _velocity_limit;
			}
			else
			{
				_rCurrent += dt * (_velocity_current + _accel_current * dt * 0.5f);
				_velocity_current = new_v;
			}
		}
		break;
	case RS_LEFT:
		if( _velocity_current > 0 )
		{
			if( _velocity_current < dt * _accel_stop )
			{
				float new_v = _accel_current * (_velocity_current / _accel_stop - dt);
				if( new_v < -_velocity_limit )
				{
					_rCurrent += -(_velocity_current * (_velocity_current + 2.0f * _velocity_limit)) /
						(-2.0f * _accel_stop) - dt * _velocity_limit +
						(_velocity_limit * _velocity_limit) / (2.0f * _accel_current);
					_velocity_current = -_velocity_limit;
				}
				else
				{
					_rCurrent += (_accel_stop * _velocity_current * _velocity_current -
						_accel_current*powf(_velocity_current - _accel_stop * dt, 2)) /
						(-2.0f * _accel_stop * _accel_stop);
					_velocity_current = new_v;
				}
			}
			else
			{
				_rCurrent += dt * (_velocity_current - dt * _accel_stop * 0.5f);
				_velocity_current = _velocity_current - dt * _accel_stop;
			}
		}
		else
		{
			float new_v = _velocity_current - dt * _accel_current;
			if( new_v < -_velocity_limit )
			{
				_rCurrent += (0.5f * powf(_velocity_current + _velocity_limit,2) -
					_accel_current * dt * _velocity_limit) / _accel_current;

				_velocity_current = -_velocity_limit;
			}
			else
			{
				_rCurrent += dt * (_velocity_current - _accel_current * dt * 0.5f);
				_velocity_current = new_v;
			}
		}
		break;
	case RS_DEACTIVATED:
		if( _velocity_current > 0 )
		{
			float new_v = _velocity_current - dt * _accel_stop;
			if( new_v < 0 )
			{
				_rCurrent += _velocity_current * _velocity_current / (2.0f * _accel_stop);
				_velocity_current = 0;
				_state = RS_STOPPED;
			}
			else
			{
				_rCurrent += dt * (_velocity_current - _accel_stop * dt * 0.5f);
				_velocity_current = new_v;
			}
		}
		else
		{
			float new_v = _velocity_current + dt * _accel_stop;
			if( new_v > 0 )
			{
				_rCurrent -= _velocity_current * _velocity_current / (2.0f * _accel_stop);
				_velocity_current = 0;
				_state = RS_STOPPED;
			}
			else
			{
				_rCurrent += dt * (_velocity_current + _accel_stop * dt * 0.5f);
				_velocity_current = new_v;
			}
		}
		break;
	case RS_GETTING_ANGLE:
		OnGettingAngle(dt);
		break;
	} // end switch (_state)


	_rCurrent = fmodf(_rCurrent, PI2);
	if( _rCurrent < 0 ) _rCurrent += PI2;

	return _rCurrent != old;
}

void Rotator::OnGettingAngle(float dt)
{
	float ac, as, vl, xt;

	float &xc = _rCurrent;
	float &vc = _velocity_current;


	//
	// выбираем направление
	//

	float xt1 = _angle_target - PI2;
	float xt2 = _angle_target + PI2;

	if( fabsf(xc - xt1) < fabsf(xc - xt2) &&
		fabsf(xc - xt1) < fabsf(xc - _angle_target) )
	{
		// отрицательное

		xt = xt1;
		vl = -_velocity_limit;
		ac = -_accel_current;
		as = _accel_stop;

	}
	else if( fabsf(xc - xt2) < fabsf(xc - xt1) &&
		fabsf(xc - xt2) < fabsf(xc - _angle_target) )
	{
		// положительное

		xt = xt2;
		vl = _velocity_limit;
		ac = _accel_current;
		as = -_accel_stop;
	}
	else
	{
		xt = _angle_target;

		if( xt > xc )
		{
			// положительное

			vl = _velocity_limit;
			ac = _accel_current;
			as = -_accel_stop;
		}
		else
		{
			// отрицательное

			vl = -_velocity_limit;
			ac = -_accel_current;
			as = _accel_stop;
		}
	}


	if( xc == xt && 0 == vc )
	{
		_state = RS_STOPPED;
		return;
	}


	// к этому моменту имеем:
	// xt - целевая точка
	// vl - предел скорости с учетом направления
	// as - коэффициент торможения с учетом направления
	// aс - коэффициент ускорения с учетом направления

	if( xt > xc && vc >= 0 || xt < xc && vc <= 0 )
	{
		// начальная скорость уже направлена в нужную сторону,
		// или равняется нулю. предварительная остановка не требуется

		float x0;

		// координата цели такова, что наблюдается самый общий случай
		// т.е. ускорение, движение с постоянной скоростью и замедление
		x0 = (vl*vl - vc*vc)/(2.0f * ac) - (vl*vl)/(2.0f * as) + xc;
		if( (xc <= x0 && x0 <= xt) || (xc >= x0 && x0 >= xt) )
		{
			ga_t1(dt, ac, as, vl, xt);
			return;
		}

		// вырождается участок с прямолинейным движением.
		// в этом случае не достигается максимальная скорость
		x0 = xc - (vc*vc)/(2.0f * as);
		if( (xc <= x0 && x0 <= xt) || (xc >= x0 && x0 >= xt) )
		{
			ga_t2(dt, ac, as, xt);
			return;
		}


		// если дошли до этого места, то целевая точка
		// слишком близко и затормозить вовремя невозможно
		ga_t3(dt, as);
	}
	else
	{
		// двигаемся не туда, куда нужно.
		// вначале необходимо затормозить
		// далее - обычный порядок обработки с нулевой начальной скоростью

		ga_t3(dt, -as);
	}
}

// ускорение, движение с постоянной скоростью и замедление
void Rotator::ga_t1(float t, float ac, float as, float vl, float xt)
{
	float &xc = _rCurrent;
	float &vc = _velocity_current;

	// фаза ускорения
	float t1 = (vl - vc) / ac;
	if( t <= t1 )
	{
		xc = (ac * t*t) * 0.5f + t*vc + xc;
		vc = ac*t + vc;
		return;
	}

	// фаза движения с постоянной скоростью
	float t2 = (ac * vl*vl + as*((vc - vl)*(vc - vl) + 2*ac*(xt - xc)))/(2.0f*ac*as*vl);
	if( t <= t2 )
	{
		xc = t*vl - (vc - vl)*(vc - vl)/(2.0f*ac) + xc;
		vc = vl;
		return;
	}

	// фаза замедления
	float t3 = (as*((vc - vl)*(vc - vl) + 2*ac*(-xc + xt)) - ac*vl*vl)/(2.0f*ac*as*vl);
	if( t <= t3 )
	{
		float new_xc = (vl*vl/as + (as*powf((vc - vl)*(vc - vl) - 2*ac*(t*vl + xc - xt),2))/
			(ac*ac*vl*vl) + (-2*(vc - vl)*(vc - vl) + 4*ac*(t*vl + xc + xt))/ac)/8.0f;
		vc = (4*vl - (4*as*((vc - vl)*(vc - vl) - 2*ac*(t*vl + xc - xt)))/(ac*vl))/8.0f;
		xc = new_xc;
		return;
	}

	// целевая точка достигнута. полная остановка
	xc = xt;
	vc = 0;
	_state = RS_STOPPED;
}

// только ускорение и замедление
void Rotator::ga_t2(float t, float ac, float as, float xt)
{
	float &xc = _rCurrent;
	float &vc = _velocity_current;

	// фаза ускорения
	float t1;
	if( ac - as > 0 && as < 0 )
		t1 = ((-vc + sqrtf((as*(2*ac*(xc - xt) - (vc*vc)))/(ac - as)))/ac);
	else
		t1 = ((-vc - sqrtf((as*(2*ac*(xc - xt) - (vc*vc)))/(ac - as)))/ac);
	if( t <= t1 )
	{
		xc = (ac * t*t) * 0.5f + t*vc + xc;
		vc = ac*t + vc;
		return;
	}

	// фаза замедления
	float t2;
	if( ac - as > 0 && as < 0 )
		t2 = ((-vc + sqrtf(((ac - as)*(2*ac*(xc - xt) - (vc*vc)))/as))/ac);
	else
		t2 = ((-vc - sqrtf(((ac - as)*(2*ac*(xc - xt) - (vc*vc)))/as))/ac);
	if( t <= t2 )
	{
		float new_xc = ((ac*ac)*as*(t*t) + 2*ac*as*t*vc - ac*(vc*vc) + 2*as*(vc*vc) +
			2*(ac*ac)*xc - 2*ac*as*xc + 2*(ac*t + vc)*sqrtf((ac - as)*as*(2*ac*(xc - xt) -
			(vc*vc))) + 2*ac*as*xt)/(2.0f*(ac*ac));
		vc = (ac*as*t + as*vc + sqrtf((ac - as)*as*(2*ac*xc - 2*ac*xt - vc*vc)))/ac;
		xc = new_xc;
		return;
	}

	// целевая точка достигнута. полная остановка
	xc = xt;
	vc = 0;
	_state = RS_STOPPED;
}

// только замедление, далее рекурсивно: ускорение, торможение
void Rotator::ga_t3(float t, float as)
{
	float &xc = _rCurrent;
	float &vc = _velocity_current;

	// здесь целевая точка не может быть достигнута.
	// поэтому двигаемся до полной остановки, а потом
	// вызываем стандартную процедуру достижения цели

	// критическое время - момент остановки
	float t0 = -vc/as;
	if( t <= t0 )
	{
		// точка остановки не достигнута
		xc = (as * t*t) * 0.5f + t*vc + xc;
		vc = as*t + vc;
		return;
	}

	// проскочили точку остановки
	// возвращаемся в нее и начинаем движение с нулевой скоростью
	vc = 0;
	xc = xc - (vc*vc)/(2.0f * as);
	process_dt(t - t0);
}

//-------------------------------------------------------------------

void Rotator::setup_sound(GC_Sound *pSound)
{
	if( RS_STOPPED == _state )
	{
		pSound->Pause(true);
	}
	else
	{
		pSound->Pause(false);

		pSound->SetSpeed(0.5f + 0.5f * fabsf(_velocity_current) / _velocity_limit);
		pSound->SetVolume(0.9f + 0.1f * fabsf(_velocity_current) / _velocity_limit);
	}
}

//-------------------------------------------------------------------

void Rotator::Serialize(SaveFile &f)
{
	f.Serialize(_accel_current);
	f.Serialize(_accel_stop);
	f.Serialize(_angle_target);
	f.Serialize(_state);
	f.Serialize(_velocity_current);
	f.Serialize(_velocity_limit);
}

///////////////////////////////////////////////////////////////////////////////
// end of file

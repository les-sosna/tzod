// Rotator.h: interface for the Rotator class.
//
// описание:
// - моделирует вращение инертного тела под действием постоянной силы.
//
// замечания:
// - точность расчета не зависит от шага dt
// - управляющие команды можно посылать только после process_dt()
//
//////////////////////////////////////////////////////////////////////

#pragma once

enum RotatorState
{
	RS_STOPPED,         // остановлен
	RS_RIGHT,           // включен и поворачивается с ускорением +_accel_current
	RS_LEFT,            // включен и поворачивается с ускорением -_accel_current
	RS_DEACTIVATED,     // включен и останавливается с ускорением _accel_stop
	RS_GETTING_ANGLE    // поворачивается на заданный угол
};

// Rotator самостоятельно переключается в режим RS_STOPPED
// из режимов RS_DEACTIVATED и RS_GETTING_ANGLE

class GC_Sound;
class SaveFile;

class Rotator
{
	RotatorState _state;

	float _angle_target;
	float _velocity_current;
	float _velocity_limit;
	float _accel_current;
	float _accel_stop;
	float &_rCurrent;

protected:
	void ga_t3(float t, float as);
	void ga_t2(float t, float ac, float as, float xt);
	void ga_t1(float t, float ac, float as, float vl, float xt);
	void OnGettingAngle(float dt);

public:
	Rotator(float &angle);
	virtual ~Rotator();

public:
	RotatorState GetState() const
	{
		return _state;
	}

	// настройка
	void reset(float angle, float velocity, float limit, float current, float stop);
	void setl(float limit, float current, float stop);
	float geta() const { return (RS_STOPPED == _state || RS_DEACTIVATED == _state) ? _rCurrent : _angle_target;}
	float getv() const { return _velocity_current; }


	bool process_dt(float dt);  // return true if angle changes


	// управляющие команды
	void impulse(float dv);
	void rotate_to(float angle);
	void rotate_left();
	void rotate_right();
	void stop();

	// установка громкости/скорости звука
	void setup_sound(GC_Sound *pSound);

	// сохранение/загрузка состояния
	void Serialize(SaveFile &f);
};

///////////////////////////////////////////////////////////////////////////////
// end of file

// ai.h: interface for the GC_PlayerAI class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "Level.h" // FIXME!
#include "Player.h"

// forward declarations
template<class T> class JobManager;
struct VehicleState;
class GC_Actor;
class GC_RigidBodyStatic;

///////////////////////////////////////////////////////////////////////////////

struct AIITEMINFO
{
	GC_Actor  *object;
	AIPRIORITY priority;
};

struct AIWEAPSETTINGS
{
	BOOL  bNeedOutstrip;       // FALSE, если мгновенное оружие (gauss, ...)
	float fMaxAttackAngle;     // максимальный прицельный угол
	float fProjectileSpeed;    // скорость снар€да
	float fAttackRadius_min;   // минимальный радиус атаки
	float fAttackRadius_max;   // максимальный радиус атаки
	float fAttackRadius_crit;  // критический радиус атаки, когда можно убитьс€
	float fDistanceMultipler;  // сложность пробивани€ стен
};

class GC_PlayerAI : public GC_Player
{
	DECLARE_SELF_REGISTRATION(GC_PlayerAI);

	static JobManager<GC_PlayerAI> _jobManager;

	typedef std::list<SafePtr<GC_RigidBodyStatic> > AttackListType;



	/*
     путь состоит из списка узлов и списка атаки, который может быть пустым.

	 танк ¬—≈√ƒј едет к начальному узлу в списке.
	 в процессе движени€ танка узлы удал€ютс€ автоматически.
	 если путь пустой, танк стоит на месте.

     в списке атаки те объекты, которые необходимо уничтожить дл€ того,
	 чтобы попасть в пункт назначени€.
	----------------------------------------------------------*/

	// узел пути
	struct PathNode
	{
		vec2d coord;
	};

	//
	// текущий путь
	//

	std::list<PathNode> _path;  // список узлов
	AttackListType _attackList;


	//-------------------------------------------------------------------------
	// Desc: —троит путь до заданной точки, вычисл€ет стоимости пути
	//  dst_x, dst_y - координаты точки назначени€
	//  max_depth    - максимальна€ глубина поиска в клетках
	//  bTest        - если true, то вычисл€етс€ только стоимомть пути
	// Return: стоимость пути или -1 если путь не найден
	//-------------------------------------------------------------------------
	float CreatePath(float dst_x, float dst_y, float max_depth, bool bTest);


	//-------------------------------------------------------------------------
	// Name: CreatePath()
	// Desc: ќчищает текущий путь и список атаки.
	//-------------------------------------------------------------------------
	void ClearPath();


	//-------------------------------------------------------------------------
	// Desc: ƒобавл€ет в текущий путь дополнительные узлы дл€ получени€ более
	//       плавной траектории.
	//-------------------------------------------------------------------------
	void SmoothPath();


	//-------------------------------------------------------------------------
	// Desc: ѕроверка проходимости €чейки пол€.
	//-------------------------------------------------------------------------
	bool CheckCell(const FieldCell &cell);

	struct TargetDesc
	{
		GC_Vehicle *target;
        bool bIsVisible;
	};

	// состо€ни€ »»
	enum aiState_l2
	{
		L2_PATH_SELECT,   // бесцельно слон€емс€ по уровню
		L2_PICKUP,        // едем за предметом
		L2_ATTACK,        // преследуем и, если возможно, атакуем цель
	} _aiState_l2;

	// сообщение верхнему уровню
	enum aiState_l1
	{
		L1_NONE,           // все идет по плану
		L1_PATH_END,       // достигнут конец пути
		L1_STICK,          // застр€ли
	} _aiState_l1;

protected:
	SafePtr<GC_PickUp>          _pickupCurrent;
	SafePtr<GC_RigidBodyStatic> _target;  // текуща€ цель
	AIWEAPSETTINGS _weapSettings; // настроики оружи€

	bool IsTargetVisible(GC_RigidBodyStatic *target, GC_RigidBodyStatic** ppObstacle = NULL);
	void LockTarget(GC_RigidBodyStatic *target);
	void FreeTarget();
	AIPRIORITY GetTargetRate(GC_Vehicle *target);

	bool FindTarget(/*out*/ AIITEMINFO &info);   // return true если цель найдена
	bool FindItem(/*out*/ AIITEMINFO &info);     // return true если что-то найдено

	// смещение прицела дл€ понижени€ меткости стрельбы
	float _desired_offset;
	float _current_offset;

	// любимое оружие
	ObjectType _otFavoriteWeapon;

	// точность
	int _accuracy;

protected:
	void RotateTo(VehicleState *pState, const vec2d &x, bool bForv, bool bBack);
	void TowerTo (VehicleState *pState, const vec2d &x, bool bFire);

	// вычисл€ет координаты мнимой цели дл€ стрельбы на опережение
	// target - цель
	// Vp      - скорость снар€да
	void CalcOutstrip(GC_Vehicle *target, float Vp, vec2d &fake);

	void ProcessAction();

	void SetL1(GC_PlayerAI::aiState_l1 new_state); // переключене состо€ни€ l1
	void SetL2(GC_PlayerAI::aiState_l2 new_state); // переключене состо€ни€ l2

	void SelectState();
	void DoState(VehicleState *pVehState);

	virtual void Serialize(SaveFile &f);

public:
	GC_PlayerAI();
	GC_PlayerAI(FromFile);
	virtual ~GC_PlayerAI();
	virtual void Kill();

	virtual void OnRespawn();
	virtual void OnDie();

protected:
	virtual void TimeStepFixed(float dt);
};

///////////////////////////////////////////////////////////////////////////////
// end of file

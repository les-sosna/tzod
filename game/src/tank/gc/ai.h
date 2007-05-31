// ai.h: interface for the AIController class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "Controller.h"

#include "Level.h" // FIXME!
#include "GameClasses.h" // FIXME!

class GC_Object;
class GC_RigidBodyStatic;

//----------------------------------------------------------

struct AIITEMINFO
{
	GC_Object  *object;
	AIPRIORITY priority;
};

struct AIWEAPSETTINGS
{
	BOOL  bNeedOutstrip;       // FALSE, если мгновенное оружие (gauss, ...)
	float fMaxAttackAngle;     // максимальный прицельный угол
	float fProjectileSpeed;    // скорость снаряда
	float fAttackRadius_min;   // минимальный радиус атаки
	float fAttackRadius_max;   // максимальный радиус атаки
	float fAttackRadius_crit;  // критический радиус атаки, когда можно убиться
	float fDistanceMultipler;  // сложность пробивания стен
};

//----------------------------------------------------------

class CAttackList
{
	// Промежуточная цель (стена, турель...)
	struct tagAttackNode
	{
		tagAttackNode      *_nextNode;
		tagAttackNode      *_prevNode;
		GC_RigidBodyStatic *_target;
	} *_firstTarget, *_lastTarget;

	static MemoryManager<tagAttackNode> s_anAllocator;

protected:
	tagAttackNode* FindObject(GC_RigidBodyStatic *object);
	void RemoveFromList(tagAttackNode *pNode);  // удалить цель из списка

public:
	CAttackList();
	CAttackList(CAttackList &al);
	virtual ~CAttackList();

	GC_RigidBodyStatic* Pop(BOOL bRemoveFromList = TRUE);  // извлечь цель из начала списка
	void PushToBegin(GC_RigidBodyStatic *target);          // поместить цель в начало списка
	void PushToEnd  (GC_RigidBodyStatic *target);          // поместить цель в конец  списка

	void Clean();                                   // освободить все убитые объекты
	void ClearList() { while (!IsEmpty()) Pop(); }  // очистить список

	inline BOOL IsEmpty() {return (NULL == _firstTarget);}

public:
	CAttackList& operator= (CAttackList &al);
};


template<class T> class JobManager;

class AIController : public CController
{
	static JobManager<AIController> _jobManager;


	/*
     путь состоит из списка узлов и списка атаки, который может быть пустым.

	 танк ВСЕГДА едет к начальному узлу в списке.
	 в процессе движения танка узлы удаляются автоматически.
	 если путь пустой, танк стоит на месте.

     в списке атаки те объекты, которые необходимо уничтожить для того,
	 чтобы попасть в пункт назначения.
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
	CAttackList  _AttackList;   // список атаки



	//-------------------------------------------------------------------------
	// Desc: Строит путь до заданной точки, вычисляет стоимости пути
	//  dst_x, dst_y - координаты точки назначения
	//  max_depth    - максимальная глубина поиска в клетках
	//  bTest        - если true, то вычисляется только стоимомть пути
	// Return: стоимость пути или -1 если путь не найден
	//-------------------------------------------------------------------------
	float CreatePath(float dst_x, float dst_y, float max_depth, bool bTest);


	//-------------------------------------------------------------------------
	// Name: CreatePath()
	// Desc: Очищает текущий путь и список атаки.
	//-------------------------------------------------------------------------
	void ClearPath();


	//-------------------------------------------------------------------------
	// Desc: Добавляет в текущий путь дополнительные узлы для получения более
	//       плавной траектории.
	//-------------------------------------------------------------------------
	void SmoothPath();


	//-------------------------------------------------------------------------
	// Desc: Проверка проходимости ячейки поля.
	//-------------------------------------------------------------------------
	bool CheckCell(const FieldCell &cell);

	struct TargetDesc
	{
		GC_Vehicle *target;
        bool bIsVisible;
	};


	// состояния ИИ
	enum aiState_l2
	{
		L2_PATH_SELECT,   // бесцельно слоняемся по уровню
		L2_PICKUP,        // едем за предметом
		L2_ATTACK,        // преследуем и, если возможно, атакуем цель
	} _aiState_l2;

	// сообщение верхнему уровню
	enum aiState_l1
	{
		L1_NONE,           // все идет по плану
		L1_PATH_END,       // достигнут конец пути
		L1_STICK,          // застряли
	} _aiState_l1;

protected:
	SafePtr<GC_PickUp> _pickupCurrent;
	GC_RigidBodyStatic* _target;  // текущая цель
	AIWEAPSETTINGS _weapSettings; // настроики оружия

	bool IsTargetVisible(GC_RigidBodyStatic *target, GC_RigidBodyStatic** ppObstacle = NULL);
	void LockTarget(GC_RigidBodyStatic *target);
	void FreeTarget();
	AIPRIORITY GetTargetRate(GC_Vehicle *target);

	bool FindTarget(/*out*/ AIITEMINFO &info);   // return true если цель найдена
	bool FindItem(/*out*/ AIITEMINFO &info);     // return true если что-то найдено

	// смещение прицела для понижения меткости стрельбы
	float _desired_offset;
	float _current_offset;

	// любимое оружие
	ObjectType _otFavoriteWeapon;

protected:
	void RotateTo(VehicleState *pState, const vec2d &x, bool bForv, bool bBack);
	void TowerTo (VehicleState *pState, const vec2d &x, bool bFire);

	// вычисляет координаты мнимой цели для стрельбы на опережение
	// target - цель
	// Vp      - скорость снаряда
	void CalcOutstrip(GC_Vehicle *target, float Vp, vec2d &fake);

	void ProcessAction();

	void SetL1(AIController::aiState_l1 new_state); // переключене состояния l1
	void SetL2(AIController::aiState_l2 new_state); // переключене состояния l2

	void SelectState();
	void DoState(VehicleState *pVehState);

public:
	AIController(GC_Player *pPlayer);
	virtual ~AIController();

	virtual void Reset(); // сброс состояния, освобождение всех ссылок
	virtual void OnPlayerRespawn();
	virtual void OnPlayerDie();

protected:
	virtual void GetControl(VehicleState *pState, float dt);
};

///////////////////////////////////////////////////////////////////////////////
// end of file

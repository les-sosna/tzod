// constants.h

#pragma once

//-----------------------------------------------------------------------------
// z-уровни
enum enumZOrder
{
	Z_EDITOR,           // метки в редакторе
	Z_WATER,            // вода
	Z_GAUSS_RAY,        // луч от Гаусса (проходит под стенами и над водой)
	Z_WALLS,            // стены
	Z_FREE_ITEM,        // лежащий на земле предмет (не виден под танком)
	Z_VEHICLES,         // танки
	Z_ATTACHED_ITEM,    // предмет, установленый на танке (оружие и т.д.)
	Z_PROJECTILE,       // летящие снаряды
	Z_EXPLODE,          // взрывы
	Z_VEHICLE_LABEL,    // метки на танке (прицел, здоровье, ...)
	Z_PARTICLE,         // частицы, дым
	Z_WOOD,             // лес
	//------------------//
	Z_COUNT,            //
	//------------------//
	Z_NONE = 0x7FFFFFFF // не рисуется
};


//-----------------------------------------------------------------------------
// Параметы таблицы фрагов
#define SCORE_POS_NUMBER     16
#define SCORE_POS_NAME       48
#define SCORE_POS_SCORE      16 // от правого края таблицы
#define SCORE_LIMITS_LEFT    64
#define SCORE_TIMELIMIT_TOP  16
#define SCORE_FRAGLIMIT_TOP  36
#define SCORE_NAMES_TOP      64
#define SCORE_ROW_HEIGHT     24

//-----------------------------------------------------------------------------
// Параметры уровня
#define LEVEL_MINSIZE   16
#define LEVEL_MAXSIZE   512
#define CELL_SIZE       32             // размер клетки
#define LOCATION_SIZE   (CELL_SIZE*8)  // размер локации (должен быть как минимум
                                       // в 2 раза больша любого объекта)

#define MAX_GAMESPEED   200
#define MIN_GAMESPEED   20

#define MAX_TIMELIMIT   180
#define MAX_FRAGLIMIT   1000

#define MAX_NETWORKSPEED    60
#define MIN_NETWORKSPEED    10

#define MAX_LATENCY         10
#define MIN_LATENCY         1


//-----------------------------------------------------------------------------
// Параметры оружия
#define WEAP_MG_TIME_RELAX      3.0f    // время восстановления прицела у пулемета

#define WEAP_RL_HOMMING_FACTOR  600.0f  // к-нт наводимости ракеты
#define WEAP_RL_HOMMING_TIME    4.0f    // время до потери цели для ракеты

#define WEAP_BFG_HOMMING_FACTOR 500.0f  // к-нт наводимости bfg
#define WEAP_BFG_RADIUS         200.0f  // радиус повреждения шарика bfg

#define WEAP_RAM_PERCUSSION     8.0f

// скорости всякие
#define SPEED_ROCKET         750.0f
#define SPEED_PLAZMA         800.0f
#define SPEED_BULLET        4000.0f
#define SPEED_TANKBULLET    1400.0f
#define SPEED_GAUSS        10000.0f
#define SPEED_ACBULLET      1800.0f
#define SPEED_DISK          2000.0f
#define SPEED_BFGCORE        500.0f
#define SPEED_SMOKE          vec2d(0, -40.0f)

// повреждение от снарядов
#define DAMAGE_ROCKET_AK47   50.0f
#define DAMAGE_BULLET         4.0f
#define DAMAGE_PLAZMA        30.0f
#define DAMAGE_TANKBULLET    60.0f
#define DAMAGE_BFGCORE       60.0f
#define DAMAGE_ACBULLET      15.0f
#define DAMAGE_GAUSS         60.0f
#define DAMAGE_GAUSS_FADE    15.0f
#define DAMAGE_DISK_MIN      20.0f
#define DAMAGE_DISK_MAX      28.0f
#define DAMAGE_DISK_FADE      3.0f
#define DAMAGE_RAM_ENGINE   100.0f


// скорость анимации
#define ANIMATION_FPS        25.0f  // кадры в секунду


// Расстояние до прицела
#define CH_DISTANCE_NORMAL  200.0f  // обычный
#define CH_DISTANCE_THIN    150.0f  // у пулемета

// вращение башни танка
#define TOWER_ROT_SPEED      5.5f   // максимальная скорость
#define TOWER_ROT_ACCEL     15.5f   // ускорение
#define TOWER_ROT_SLOWDOWN  30.1f   // замедление

// время действия
#define BOOSTER_TIME        20.0f
#define PROTECT_TIME        15.0f


//-----------------------------------------------------------------------------
// полезность предметов

// если полезность <= AIP_NOTREQUIRED то бот игнорирует предмет
#define AIP_NOTREQUIRED     0.0f

// эталоный уровень полезности
#define AIP_NORMAL          1.0f

// с расстоянием полезность уменьшается: p = (base - AIP_NORMAL * l / AI_MAX_DEPTH)
// где base - базовый уровень полезности, l - расстояние в клетках


#define AIP_WEAPON_NORMAL   (AIP_NORMAL)        // полезность оружия, когда танк безоружен
#define AIP_WEAPON_FAVORITE (AIP_NORMAL / 2)    // бонус для любимого оружия
#define AIP_WEAPON_ADVANCED (AIP_NORMAL / 2)    // полезность оружия с прицепленным бестером
#define AIP_HEALTH          (AIP_NORMAL)        // полезность аптеки, когда здоровье танка на нуле
#define AIP_BOOSTER         (AIP_NORMAL)        // бустер оружия
#define AIP_BOOSTER_HAVE    (AIP_BOOSTER / 10)  // если бустер уже есть
#define AIP_SHOCK           (AIP_NORMAL)        // электрошок
#define AIP_INVULN          (AIP_NORMAL)        // неуязвимость

#define AI_MAX_DEPTH   50.0f
#define AI_MAX_SIGHT   20.0f
#define AI_MAX_LEVEL   4

//-----------------------------------------------------------------------------
// Параметры стационарных установок
#define TURET_ROCKET_RELOAD  0.9f
#define TURET_CANON_RELOAD   0.4f
#define TURET_SIGHT_RADIUS   500

//-----------------------------------------------------------------------------
// Параметры игрока
#define PLAYER_RESPAWNTIME   2.0f  // задержка рождения игрока


//-----------------------------------------------------------------------------
// Совместимость

#define TXT_VERSION      "Tank: Zone of Death (1.48)"
#define TXT_WNDCLASS     "TankMainWindow"
#define TXT_PROPGRID     "TankPropertyGrid"
#define TXT_PROPGRIDINT  "TankPropertyGridInt"

// версия  файла  Save  для  первичной  проверки на корректность.
// файлы  с  отличающейся  версией не будут отображаться в списке
#define VERSION    0x1480

//-----------------------------------------------------------------------------
// Ограничения

// максимальное число игроков
#define MAX_PLAYERS     32
#define MAX_HUMANS       4

// максимальное число команд в командных играх (включая 0)
#define MAX_TEAMS        6

// максимальное время dt. Если при сильных тормозах dt вдруг
// окажется больше, то оно принудительно будет уменьшено до MAX_DT
#define MAX_DT           0.05f

#define MAX_DT_FIXED     0.02f


// дублирование сетевых пакетов
#define NET_MULTIPLER    2

//-----------------------------------------------------------------------------
// Типы игры

// режим редактора.
// в этом режиме не активны timelimit и fraglimit,
// редактор доступен всегда
#define GT_EDITOR        0

// тип игры deathmatch. каждый сам за себя.
#define GT_DEATHMATCH    1

// main menu intro
#define GT_INTRO         2


//-----------------------------------------------------------------------------
// математические константы
#define PI    3.141593f
#define PI2   6.283185f


//-----------------------------------------------------------------------------
// файлы
#define DIR_SCRIPTS      "scripts"
#define DIR_SAVE         "save"
#define DIR_MAPS         "maps"
#define DIR_SKINS        "skins"
#define DIR_THEMES       "themes"
#define DIR_MUSIC        "music"
#define DIR_SCREENSHOTS  "screenshots"

#define FILE_CONFIG      "config.cfg"
#define FILE_LANGUAGE    "lang.cfg"
#define FILE_TEXTURES    DIR_SCRIPTS"/textures.lua"
#define FILE_STARTUP     DIR_SCRIPTS"/init.lua"

///////////////////////////////////////////////////////////////////////////////
// end of file

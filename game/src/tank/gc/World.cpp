// World.cpp

#include "World.h"
#include "World.inl"
#include "WorldEvents.h"
#include "RigidBodyDinamic.h"
#include "Player.h"
#include "Sound.h"
#include "Macros.h"

#include "MapFile.h"
#include "SaveFile.h"
#include "ThemeManager.h"

#include "core/Debug.h"

#include "config/Config.h"

#include <fs/FileSystem.h>


#define MAX_THEME_NAME  128

struct SaveHeader
{
    uint32_t dwVersion;
    bool  nightmode;
	bool  started;
    float timelimit;
    int   fraglimit;
    float time;
    int   width;
    int   height;
    char  theme[MAX_THEME_NAME];
};


// don't create game objects in the constructor
World::World()
  : _gameStarted(false)
  , _frozen(false)
  , _limitHit(false)
  , _sx(0)
  , _sy(0)
  , _locationsX(0)
  , _locationsY(0)
  , _seed(1)
  , _time(0)
  , _safeMode(true)
#ifdef NETWORK_DEBUG
  , _checksum(0)
  , _frame(0)
  , _dump(NULL)
#endif
{
	TRACE("Constructing the world");

	// register config handlers
	g_conf.s_volume.eventChange = std::bind(&World::OnChangeSoundVolume, this);
}

bool World::IsEmpty() const
{
	return GetList(LIST_objects).empty();
}

void World::OnKill(GC_Object &obj)
{
	for( auto ls: eWorld._listeners )
		ls->OnKill(obj);
}

void World::GameMessage(const char *msg)
{
	for( auto ls: eWorld._listeners )
		ls->OnGameMessage(msg);
}

void World::Resize(int X, int Y)
{
	assert(IsEmpty());


	//
	// Resize
	//

	_locationsX  = (X * CELL_SIZE / LOCATION_SIZE + ((X * CELL_SIZE) % LOCATION_SIZE != 0 ? 1 : 0));
	_locationsY  = (Y * CELL_SIZE / LOCATION_SIZE + ((Y * CELL_SIZE) % LOCATION_SIZE != 0 ? 1 : 0));
	_sx          = (float) X * CELL_SIZE;
	_sy          = (float) Y * CELL_SIZE;

	grid_rigid_s.resize(_locationsX, _locationsY);
	grid_walls.resize(_locationsX, _locationsY);
	grid_wood.resize(_locationsX, _locationsY);
	grid_water.resize(_locationsX, _locationsY);
	grid_pickup.resize(_locationsX, _locationsY);
	grid_actors.resize(_locationsX, _locationsY);

	_field.Resize(X + 1, Y + 1);
}

void World::Clear()
{
	assert(IsSafeMode());

    ObjectList &ls = GetList(LIST_objects);
    while( !ls.empty() )
    {
        ls.at(ls.begin())->Kill(*this);
    }

	// reset info
	_infoAuthor.clear();
	_infoEmail.clear();
	_infoUrl.clear();
	_infoDesc.clear();
	_infoTheme.clear();
	_infoOnInit.clear();

	// reset variables
	_time = 0;
	_limitHit = false;
	_frozen = false;
	_gameStarted = false;
#ifdef NETWORK_DEBUG
	_checksum = 0;
	_frame = 0;
	if( _dump )
	{
		fclose(_dump);
		_dump = NULL;
	}
#endif
    assert(IsEmpty());
}

void World::HitLimit()
{
	assert(!_limitHit);
//	PauseLocal(true);
	_limitHit = true;
	for( auto ls: eWorld._listeners )
		ls->OnGameFinished();
}

World::~World()
{
	assert(IsSafeMode());
	TRACE("Destroying the world");
	Clear();

	// unregister config handlers
	g_conf.s_volume.eventChange = nullptr;

	assert(IsEmpty() && _garbage.empty());
}

void World::Unserialize(std::shared_ptr<FS::Stream> stream, const ThemeManager &themeManager, TextureManager &tm)
{
	assert(IsSafeMode());
	assert(IsEmpty());

	SaveFile f(stream, true);

	try
	{
		SaveHeader sh;
		if( 1 != stream->Read(&sh, sizeof(SaveHeader), 1) )
            throw std::runtime_error("unexpected end of file");

		if( VERSION != sh.dwVersion )
			throw std::runtime_error("invalid version");

		g_conf.sv_timelimit.SetFloat(sh.timelimit);
		g_conf.sv_fraglimit.SetInt(sh.fraglimit);
		g_conf.sv_nightmode.Set(sh.nightmode);

		_gameStarted = sh.started;
		_time = sh.time;
		Resize(sh.width, sh.height);


		// fill pointers cache
		for(;;)
		{
			ObjectType type;
			f.Serialize(type);
			if( INVALID_OBJECT_TYPE == type ) // end of list signal
				break;
			if( GC_Object *obj = RTTypes::Inst().CreateFromFile(*this, type) )
			{
				f.RegPointer(obj);
			}
			else
			{
				TRACE("ERROR: unknown object type - %u", type);
				throw std::runtime_error("Load error: unknown object type");
			}
		}

		// read objects contents in the same order as pointers
		for( ObjectList::id_type it = GetList(LIST_objects).begin(); it != GetList(LIST_objects).end(); it = GetList(LIST_objects).next(it) )
		{
            GetList(LIST_objects).at(it)->Serialize(*this, f);
		}

		// apply the theme
		_infoTheme = sh.theme;
		themeManager.ApplyTheme(themeManager.FindTheme(sh.theme), tm);
	}
	catch( const std::runtime_error& )
	{
		Clear();
		throw;
	}
}

void World::Serialize(std::shared_ptr<FS::Stream> stream)
{
	assert(IsSafeMode());

	SaveFile f(stream, false);

	SaveHeader sh = {0};
	strcpy(sh.theme, _infoTheme.c_str());
	sh.dwVersion    = VERSION;
	sh.fraglimit    = g_conf.sv_fraglimit.GetInt();
	sh.timelimit    = g_conf.sv_timelimit.GetFloat();
	sh.nightmode    = g_conf.sv_nightmode.Get();
	sh.started      = _gameStarted;
	sh.time         = _time;
	sh.width        = (int) _sx / CELL_SIZE;
	sh.height       = (int) _sy / CELL_SIZE;

	stream->Write(&sh, sizeof(SaveHeader));


	//
	// pointers to game objects
	//
    ObjectList &objects = GetList(LIST_objects);
	for( auto it = objects.begin(); it != objects.end(); it = objects.next(it) )
	{
		GC_Object *object = objects.at(it);
		ObjectType type = object->GetType();
		stream->Write(&type, sizeof(type));
		f.RegPointer(object);
	}
	ObjectType terminator(INVALID_OBJECT_TYPE);
	f.Serialize(terminator);


	//
	// write objects contents in the same order as pointers
	//

	for( auto it = objects.begin(); it != objects.end(); it = objects.next(it) )
	{
        objects.at(it)->Serialize(*this, f);
	}
}

void World::Import(std::shared_ptr<FS::Stream> s, const ThemeManager &themeManager, TextureManager &tm)
{
	assert(IsEmpty());
	assert(IsSafeMode());

	MapFile file(s, false);

	int width, height;
	if( !file.getMapAttribute("width", width) ||
		!file.getMapAttribute("height", height) )
	{
		throw std::runtime_error("unknown map size");
	}

	file.getMapAttribute("theme", _infoTheme);
	themeManager.ApplyTheme(themeManager.FindTheme(_infoTheme), tm);

	file.getMapAttribute("author",   _infoAuthor);
	file.getMapAttribute("desc",     _infoDesc);
	file.getMapAttribute("link-url", _infoUrl);
	file.getMapAttribute("e-mail",   _infoEmail);
	file.getMapAttribute("on_init",  _infoOnInit);

	Resize(width, height);

	while( file.NextObject() )
	{
		float x = 0;
		float y = 0;
		file.getObjectAttribute("x", x);
		file.getObjectAttribute("y", y);
		ObjectType t = RTTypes::Inst().GetTypeByName(file.GetCurrentClassName());
		if( INVALID_OBJECT_TYPE == t )
			continue;
		GC_Object *object = RTTypes::Inst().GetTypeInfo(t).Create(*this, x, y);
		object->MapExchange(*this, file);
	}
}

void World::Export(std::shared_ptr<FS::Stream> s)
{
	assert(IsSafeMode());

	MapFile file(s, true);

	//
	// map info
	//

	file.setMapAttribute("type", "deathmatch");

	std::ostringstream str;
	str << VERSION;
	file.setMapAttribute("version", str.str());

	file.setMapAttribute("width",  (int) _sx / CELL_SIZE);
	file.setMapAttribute("height", (int) _sy / CELL_SIZE);

	file.setMapAttribute("author",   _infoAuthor);
	file.setMapAttribute("desc",     _infoDesc);
	file.setMapAttribute("link-url", _infoUrl);
	file.setMapAttribute("e-mail",   _infoEmail);

	file.setMapAttribute("theme",    _infoTheme);

	file.setMapAttribute("on_init",  _infoOnInit);

	// objects
	FOREACH( GetList(LIST_objects), GC_Object, object )
	{
		if( RTTypes::Inst().IsRegistered(object->GetType()) )
		{
			file.BeginObject(RTTypes::Inst().GetTypeName(object->GetType()));
			object->MapExchange(*this, file);
			file.WriteCurrentObject();
		}
	}
}

void World::PauseSound(bool pause)
{
	FOREACH( GetList(LIST_sounds), GC_Sound, pSound )
	{
		pSound->Freeze(pause);
	}
}

int World::net_rand()
{
	return ((_seed = _seed * 214013L + 2531011L) >> 16) & NET_RAND_MAX;
}

float World::net_frand(float max)
{
	return (float) net_rand() / (float) NET_RAND_MAX * max;
}

vec2d World::net_vrand(float len)
{
	return vec2d(net_frand(PI2)) * len;
}

bool World::CalcOutstrip( const vec2d &fp, // fire point
                          float vp,        // speed of the projectile
                          const vec2d &tx, // target position
                          const vec2d &tv, // target velocity
                          vec2d &out_fake) // out: fake target position
{
	float vt = tv.len();

	if( vt >= vp || vt < 1e-7 )
	{
		out_fake = tx;
		return false;
	}

	float cg = tv.x / vt;
	float sg = tv.y / vt;

	float x   = (tx.x - fp.x) * cg + (tx.y - fp.y) * sg;
	float y   = (tx.y - fp.y) * cg - (tx.x - fp.x) * sg;
	float tmp = vp*vp - vt*vt;

	float fx = x + vt * (x*vt + sqrt(x*x * vp*vp + y*y * tmp)) / tmp;

	out_fake.x = std::max(0.0f, std::min(_sx, fp.x + fx*cg - y*sg));
	out_fake.y = std::max(0.0f, std::min(_sy, fp.y + fx*sg + y*cg));
	return true;
}

GC_RigidBodyStatic* World::TraceNearest( Grid<ObjectList> &list,
                                         const GC_RigidBodyStatic* ignore,
                                         const vec2d &x0,      // origin
                                         const vec2d &a,       // direction with length
                                         vec2d *ht,
                                         vec2d *norm) const
{
//	DbgLine(x0, x0 + a);

	struct SelectNearest
	{
		const GC_RigidBodyStatic *ignore;
		vec2d x0;
		vec2d lineCenter;
		vec2d lineDirection;

		GC_RigidBodyStatic *result;
		vec2d resultPos;
		vec2d resultNorm;

		SelectNearest()
			: result(NULL)
		{
		}

		bool Select(GC_RigidBodyStatic *obj, vec2d norm, float enter, float exit)
		{
			if( ignore != obj )
			{
				result = obj;
				resultPos = lineCenter + lineDirection * enter;
				resultNorm = norm;

				lineDirection *= enter + 0.5f;
				lineCenter = x0 + lineDirection / 2;
			}
			return false;
		}
		inline const vec2d& GetCenter() const { return lineCenter; }
		inline const vec2d& GetDirection() const { return lineDirection; }
	};
	SelectNearest selector;
	selector.ignore = ignore;
	selector.x0 = x0;
	selector.lineCenter = x0 + a/2;
	selector.lineDirection = a;
	RayTrace(list, selector);
	if( selector.result )
	{
		if( ht ) *ht = selector.resultPos;
		if( norm ) *norm = selector.resultNorm;
	}
	return selector.result;
}

void World::TraceAll( Grid<ObjectList> &list,
                      const vec2d &x0,      // origin
                      const vec2d &a,       // direction with length
                      std::vector<CollisionPoint> &result) const
{
	struct SelectAll
	{
		vec2d lineCenter;
		vec2d lineDirection;

		std::vector<CollisionPoint> &result;

		explicit SelectAll(std::vector<CollisionPoint> &r)
			: result(r)
		{
		}

		bool Select(GC_RigidBodyStatic *obj, vec2d norm, float enter, float exit)
		{
			CollisionPoint cp;
			cp.obj = obj;
			cp.normal = norm;
			cp.enter = enter;
			cp.exit = exit;
			result.push_back(cp);
			return false;
		}
		inline const vec2d& GetCenter() const { return lineCenter; }
		inline const vec2d& GetDirection() const { return lineDirection; }
	};
	SelectAll selector(result);
	selector.lineCenter = x0 + a/2;
	selector.lineDirection = a;
	RayTrace(list, selector);
}

IMPLEMENT_POOLED_ALLOCATION(ResumableObject);

ResumableObject* World::Timeout(GC_Object &obj, float timeout)
{
	assert(GetTime() + timeout >= GetTime());
	auto id = new ResumableObject(obj);
	_resumables.emplace(std::unique_ptr<ResumableObject>(id), GetTime() + timeout);
	return id;
}

void World::Step(float dt)
{
	if( !_gameStarted )
	{
		_gameStarted = true;
		for( auto ls: eWorld._listeners )
			ls->OnGameStarted();
	}
	
	float nextTime = _time + dt;
	while (!_resumables.empty() && _resumables.top().time < nextTime)
	{
		_time = _resumables.top().time;
		GC_Object *obj = _resumables.top().obj->ptr;
		_resumables.pop();
		if (obj)
			obj->Resume(*this);
	}
	
	_time = nextTime;

	if( !_frozen )
	{
		_safeMode = false;
        ObjectList &ls = GetList(LIST_timestep);
        ls.for_each([=](ObjectList::id_type id, GC_Object *o){
            o->TimeStep(*this, dt);
        });
		GC_RigidBodyDynamic::ProcessResponse(*this, dt);
		_safeMode = true;
	}

    assert(_safeMode);

	if( g_conf.sv_timelimit.GetInt() && g_conf.sv_timelimit.GetInt() * 60 <= _time )
	{
		HitLimit();
	}

	GetList(LIST_sounds).for_each([=](ObjectList::id_type id, GC_Object *o) {
		static_cast<GC_Sound *>(o)->KillWhenFinished(*this);
	});


	//
	// sync lost error detection
	//

#ifdef NETWORK_DEBUG
	if( !_dump )
	{
		char fn[MAX_PATH];
		sprintf_s(fn, "network_dump_%u_%u.txt", GetTickCount(), GetCurrentProcessId());
		_dump = fopen(fn, "w");
		assert(_dump);
	}
	++_frame;
	fprintf(_dump, "\n### frame %04d ###\n", _frame);

	DWORD dwCheckSum = 0;
	for( ObjectList::safe_iterator it = ts_fixed.safe_begin(); it != ts_fixed.end(); ++it )
	{
		if( DWORD cs = (*it)->checksum() )
		{
			dwCheckSum = dwCheckSum ^ cs ^ 0xD202EF8D;
			dwCheckSum = (dwCheckSum >> 1) | ((dwCheckSum & 0x00000001) << 31);
			fprintf(_dump, "0x%08x -> local 0x%08x, global 0x%08x  (%s)\n", (*it), cs, dwCheckSum, typeid(**it).name());
		}
	}
	_checksum = dwCheckSum;
	fflush(_dump);
#endif
}

GC_Object* World::FindObject(const std::string &name) const
{
	std::map<std::string, const GC_Object*>::const_iterator it = _nameToObjectMap.find(name);
	return _nameToObjectMap.end() != it ? const_cast<GC_Object*>(it->second) : NULL;
}

void World::OnChangeSoundVolume()
{
	FOREACH( GetList(LIST_sounds), GC_Sound, pSound )
	{
		pSound->UpdateVolume();
	}
}

///////////////////////////////////////////////////////////////////////////////

GC_Player* World::GetPlayerByIndex(size_t playerIndex)
{
	GC_Player *player = NULL;
	FOREACH(GetList(LIST_players), GC_Player, p)
	{
		if( 0 == playerIndex-- )
		{
			player = p;
			break;
		}
	}
	return player;
}

void World::Seed(unsigned long seed)
{
    _seed = seed;
}


///////////////////////////////////////////////////////////////////////////////
// end of file

-- init.lua
-- autorun script


dofile("scripts/func.lua")       -- define some usefull functions
dofile("scripts/vehicles.lua")   -- register vehicle classes
dofile("scripts/weapons.lua")    -- set up parameters of weapons
dofile("scripts/names.lua")      -- fill the random_names array

initcmdqueue()

--=============================================================================

conf.sv_timelimit = 0;
conf.sv_fraglimit = 0;
conf.sv_nightmode = false;
conf.sv_speed = 100;

loadmap("maps/intro.map")

for _,i in pairs{"red","yellow","blue","FBI Tank","neutral"} do
	addbot{skin=i}
end


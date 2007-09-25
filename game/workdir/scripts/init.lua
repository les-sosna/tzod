-- init.lua
-- autorun script


dofile("scripts/func.lua")       -- define some usefull functions
dofile("scripts/vehicles.lua")   -- register vehicle classes
dofile("scripts/weapons.lua")    -- set up parameters of weapons
dofile("scripts/names.lua")      -- fill the random_names array

--=============================================================================

conf.sv_timelimit = 0;
conf.sv_fraglimit = 0;
conf.sv_speed = 100;

print( math.random() )

if math.random() > 0.5 then
  conf.sv_nightmode = false;
  loadmap("maps/intro.map")
else
  conf.sv_nightmode = true;
  loadmap("maps/intro02.map")
end

for _,i in pairs{"red","yellow","blue","FBI Tank","neutral"} do
  service("ai", {skin=i})
end


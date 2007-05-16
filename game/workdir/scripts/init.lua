-- init.lua
-- autorun script


dofile("scripts/func.lua")       -- define some usefull functions
dofile("scripts/vehicles.lua")   -- register vehicle classes
dofile("scripts/weapons.lua")    -- set up parameters of weapons
dofile("scripts/names.lua")      -- fill the random_names array

--=============================================================================

loadmap("maps/intro.map")

for _,i in pairs{"red","yellow","blue","FBI Tank","neutral"} do
	addplayer{cls="default", skin=i}
end


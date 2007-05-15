-- init.lua
-- autorun script


dofile("scripts/func.lua")       -- define some usefull functions
dofile("scripts/vehicles.lua")   -- register vehicle classes
dofile("scripts/weapons.lua")    -- set up parameters of weapons
dofile("scripts/names.lua")      -- fill the random_names array

--=============================================================================

print("loading startup level");

loadmap("maps/intro.map")


message("добро пожаловать в игру Танк: Зона смерти")


for _,i in pairs{"red","yellow","blue","FBI Tank","neutral"} do
	addplayer{cls="default", skin=i}
end



--message("-----------------------------------------")
--message("чтобы начать игру, нажмите F2")
--message("чтобы попасть в главное меню, нажмите ESC")

--pause(false)

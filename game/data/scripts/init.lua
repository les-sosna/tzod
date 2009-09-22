-- init.lua
-- autorun script

package.path = "data/scripts/?.lua"


require "autocomplete"  -- init autocomplete engine for the console
require "func"          -- define some usefull functions
require "vehicles"      -- register vehicle classes
require "weapons"       -- set up parameters of weapons
require "names"         -- fill the random_names array

---------------------------------------------------------------------
-- load one of two intro maps

conf.sv_timelimit = 0;
conf.sv_fraglimit = 0;
conf.sv_speed = 100;
conf.ui_showmsg = false;

if math.random() > 0.5 then
  conf.sv_nightmode = math.random() > 0.5;
  loadmap("maps/intro.map")
else
  conf.sv_nightmode = math.random() > 0.5;
  loadmap("maps/intro02.map")
end

for _,s in pairs{"red","yellow","blue","FBI Tank","neutral"} do
  service("ai", {skin=s})
end

pushcmd( function() music "default.ogg" end, 1 )


-- end of file

-- init.lua
-- autorun script

package.path = "data/scripts/?.lua"


require "autocomplete"  -- init autocomplete engine for the console
require "func"          -- define some usefull functions
require "editor"        -- define editor actions

quit = game.quit

---------------------------------------------------------------------
-- load one of two intro maps

game.start("intro")

conf.sv_timelimit = 0;
conf.sv_fraglimit = 0;
conf.sv_speed = 100;
conf.ui_showmsg = false;

if math.random() > 0.5 then
  conf.sv_nightmode = math.random() > 0.5;
  game.loadmap("maps/intro.map")
else
  conf.sv_nightmode = math.random() > 0.5;
  game.loadmap("maps/intro02.map")
end

for _,s in ipairs{"red","yellow","blue","FBI Tank","neutral"} do
  world.service("ai", {skin=s})
end

pushcmd( function() music "default.ogg" end, 1 )

-- end of file

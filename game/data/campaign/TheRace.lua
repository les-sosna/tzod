-- The Racer Company by [A_K aka NC22] v1.15 [fixes only]
reset()
-------TODO---Для версии 2.0---------
--2 режима отображения круга (1 - под чекпоинтами -немного не доработан 2 - под игроком постоянно при движении несколько секунд после прохождения чекпоинта)
--режим на 2 игроков (вместо player и tank1 проверять objtype функцией (только щас увидел эту функцию в наборе :) ))
---Добавить бонусы(спец объект - сплайн+оружие, по наезду на него бот\плеер берет один из бонусов, оружие незаметно забираем. сплайн тоже исчезает)
--дать ботам добро на подбор бонусов (ai_pickup)
--ускорение (редактируем класс. max_speed \ power
--миноукладка (раз в секунду создается мина под тобой (конечно в начале берем кординаты игрока, а потом через какоето время кладем мину :))
--заморозка (active 0 у ботов, замена профиля у player)
--строения \ новые сплайны дороги \сплайны для поворотов \ / а не только по квадрату
--минное поле. мины в случайном порядке
-------новый класс грузовик в генератор.
---миссии за полицию \ трамплины и прыжки при помощи setposition  \
--- 
conf.sv_timelimit = 0
conf.sv_fraglimit = 0
conf.sv_nightmode = false;
music ""
-----константы----
local stage_total=7
local campdir ="campaign/TheRace/"
-----таблица переменных юзера
local options = {}
options.wathtype=0
options.botreduce=0
options.stage=1
options.music=1
------таблица переменных карты
local map = {}
------------Системное для генерации карты----------
map.x_lap=0
map.y_lap=0 --текущая координата для генерации круга
map.road=0 --количиство дорожных блоков на карте
map.walls=0 -- количество стен на карте
map.night=0
map.checkpoint=5
map.checkpoint_default=5 
map.checkpoints=0 --количество чекпоинтов на карте
---------------------------------------------------
map.currentmap=0 -- текущий ID карты
map.respawns=0 -- вкл \ выкл респавны
map.police=0 -- полиция
map.withguns=0 --оружие у гоньщиков
map.lockplayer=0 --блокировать ли игрока на старте
map.start=0 --начата ли гонка
map.respawn_delai=0.8 --задержка при респавне
-----таблица переменных игрока
local gamer = {}

-----------КЛАССЫ--------------
classes["user/car"] = tcopy(classes.default)
classes["user/car"].health = 150;
classes["user/car"].width = 30;
classes["user/car"].length = 44;
classes["user/car"].mass = 0.5;
classes["user/car"].inertia = 2000.0;
classes["user/car"].vis_fric[3]= 9;
classes["user/car"].power[1] = 800; 
classes["user/car"].power[2] = 80;
classes["user/car"].max_speed[1]=500;
classes["user/car"].max_speed[2]=30.5;
----
classes["user/car_bot"] = tcopy(classes["user/car"])
classes["user/car_bot"].max_speed[1]=800;
classes["user/car_bot"].power[1] = 1200;
----
classes["user/f1"] = tcopy(classes.default)
classes["user/f1"].health = 100;
classes["user/f1"].width = 38;--38
classes["user/f1"].length = 44;--45
classes["user/f1"].mass = 0.5;
classes["user/f1"].inertia = 2000.0;
classes["user/f1"].vis_fric[3]= 9;
classes["user/f1"].power[1] = 1800; 
classes["user/f1"].power[2] = 80;
classes["user/f1"].max_speed[1]=1000;
classes["user/f1"].max_speed[2]=63.5;
---
classes["user/f1bot"] = tcopy(classes["user/f1"])
classes["user/f1bot"].max_speed[1]=2000;
classes["user/f1bot"].power[1] = 2300;

----самому за него всех подавить \ против ботов
classes["user/truck"] = {
  display     = "ЗИЛ",
  health      = 2000,
  percussion  = 160,
  fragility   = 1,
  width     = 40,
  length    = 64,
  mass      = 3.0,
  inertia   = 700.0,
  dry_fric  = {120, 1000, 120},  
  vis_fric  = {0.5, 2.5, 1},  
  power     = {1600, 978},   
  max_speed = {100, 2.5},    
}
----

--------------------------------------------------
local function get32(num)
return ((num-1) * 32) + 16;
end

local function m(mus)
track="..//campaign//TheRace//music//"..mus..".ogg"
if music(track) ==false then return 0 
else return 1
end
end
----------------------------------Отображение кругов-------------------
local function place()
local place=4;
for i=1,3 do
    if ((gamer["bot_tank"..i].laps+1)<(gamer["tank1"].laps+1)) then place=place-1
	else if ((gamer["bot_tank"..i].laps+1)==(gamer["tank1"].laps+1)) and (gamer["bot_tank"..i].point<gamer["tank1"].point) then place=place-1 end 
	end
 end
 return place;
end


local function lapviewattach(x,y)
local id,p;
if options.wathtype<1 and x~=nil and y~=nil then
p=0
 for i=1,6 do
 if (exists("interface"..i) == true) then kill("interface"..i) end; 
  if i==1 then id=44 
  elseif i==2 then id=16+gamer['tank1'].laps+1 
  elseif i==3 then id=0 p=20
  elseif i==4 then id=16+place() p=0 
  elseif i==5 then id=15 
  elseif i==6 then id=16+4 end
	actor("user_sprite", x+i*20+p,y+200, {name="interface"..i, frame=id, texture="font_default", layer=10})
 end
elseif options.wathtype==1 and exists("tank1")==true then
	if (exists("tank1")==true) then
	x,y = position("tank1"); p=0
	gamer['tank1'].counter=1 
	if (exists("interface1") == false) then
		for i=1,6 do
			actor("user_sprite", x+i*20+p,y+200, {name="interface"..i, frame=16, texture="font_default", layer=10})
		end
	end
	for i=1,6 do
	  if i==1 then id=44 
	  elseif i==2 then id=16+gamer['tank1'].laps+1 
	  elseif i==3 then id=0 p=20
	  elseif i==4 then id=16+place() p=0 
	  elseif i==5 then id=15 
	  elseif i==6 then id=16+4 end
		setposition("interface"..i,x+i*20+p,y+200)
		pset("interface"..i,"frame",id)	
	end
	pushcmd(function() lapviewattach() end,0.01)
	end
else
	for i=1,6 do
		if (exists("interface"..i) == true) then kill("interface"..i) end; 
	end
	gamer['tank1'].counter=nil 
end
end

local function CreatePlayer(pname,pvehname,pskin,pclass,l)
service( "player_local", { 
			name=pname, 
			skin=pskin, 
			team=1, 
			on_respawn="user.EquipRacer('"..pvehname.."') user.EraseSpawnPoints()",
			on_die="user.OnDie('"..pname.."')", 
			vehname=pvehname,
			class=pclass, 
			nick="Racer" } )
		gamer[pvehname]={}
		gamer[pvehname].playername=pname
		gamer[pvehname].inrace=1;
		gamer[pvehname].lifes= l;
end

local function CreateBot(bname,bvehname,bskin,bclass,l)
local bot=service( "ai", { 
				name=bname, 
				skin=bskin, 
				team=1, 
				on_respawn="user.EquipRacer('"..bvehname.."') user.EraseSpawnPoints()",
				on_die="user.OnDie('"..bname.."')",
				class=bclass,
				vehname=bvehname,
				level=1, 
				active=0
				} )		
		gamer[bvehname]={}
		gamer[bvehname].playername=bname
		gamer[bvehname].inrace=1;
		gamer[bvehname].lifes= l;
return bot;
end

----------Все что связано с респавнами-----------
function user.EraseSpawnPoints()
if (exists("r1")==true) then
local used=0;
	for n = 1,3 do
	if (exists("bot_tank"..n)==true) then used=used+1 end
	end
if (exists("tank1")==true) then used=used+1 end
	if (used==4) then
	for n = 1,4 do
	kill("r"..n);
	end
	end
end
end

function user.EraseSpawnPointsP()
if (exists("rp1")==true) then
local used=0;
	for n = 1,10 do
	if (exists("punisher"..n)==true) then used=used+1 end
	end
	if (used==6) then
	for n = 1,10 do
	kill("rp"..n);
	end
	end
end
end

local function CheckpointXtriggerOffset(prefix,x,y)
local x_start = x - 32*2;
for n = 1,5 do
actor("trigger", x_start+(n-1)*32,y, {name="t"..n.."c"..prefix, on_enter="user.CheckPoint("..prefix..","..(x_start+20)..","..(y+20)..",who)", layer=3});
end
end

local function CheckpointYtriggerOffset(prefix,x,y)
local y_start = y - 32*2;
for n = 1,5 do
actor("trigger", x,y_start+(n-1)*32, {name="t"..n.."c"..prefix, on_enter="user.CheckPoint("..prefix..","..(x+20)..","..(y_start+20)..",who)", layer=3});
end
end

local function CreateWayXForward(size,spin)
local x;
for n = 1,size do
x=map.x_lap+(128*spin);
map.x_lap=x;
map.road=map.road+1;
map.checkpoint=map.checkpoint-1;
if (map.checkpoint == 0) then 
map.checkpoints=map.checkpoints+1; 
map.checkpoint=map.checkpoint_default; 
CheckpointYtriggerOffset(map.checkpoints,x,map.y_lap)

----
local rotator;
if spin<0 then rotator=3.14159
else rotator=0 end

if map.night==1 then 
	actor('spotlight',x,map.y_lap+104,{active=1,dir=rotator})
	actor('spotlight',x,map.y_lap-104,{active=1,dir=rotator})
end
actor("user_sprite", x,map.y_lap, {name="helper"..x..map.y_lap, texture="font_default", layer=0, frame=0,rotation=rotator});
----
actor("user_sprite", x,map.y_lap, {name="c"..map.checkpoints, texture="user/point", layer=3});
end
actor("user_sprite", x,map.y_lap, {name="w"..map.road, texture="user/f", layer=0});
end
end

local function CreateWayYForward(size,spin)
local y;
for n = 1,size do
y=map.y_lap+(128*spin);
map.y_lap=y;
map.road=map.road+1;
map.checkpoint=map.checkpoint-1;
if (map.checkpoint == 0) then 
map.checkpoints=map.checkpoints+1; 
map.checkpoint=map.checkpoint_default; 
CheckpointXtriggerOffset(map.checkpoints,map.x_lap,y) 
----
local rotator;
if spin<0 then rotator=4.71239
else rotator=1.5708 end

if map.night==1 then 
	actor('spotlight',map.x_lap+104,y,{active=1,dir=rotator})
	actor('spotlight',map.x_lap-104,y,{active=1,dir=rotator})
end

actor("user_sprite", map.x_lap,y, {name="helper"..map.x_lap..y, texture="font_default", layer=0, frame=0,rotation=rotator});
----
actor("user_sprite", map.x_lap,y, {name="c"..map.checkpoints, texture="user/point2", layer=3});
end
actor("user_sprite", map.x_lap,y, {name="w"..map.road, texture="user/f", rotation=4.71239 , layer=0});
end
end

local function CreateWayYTurn(typeof,spin)
-- type 1 - справа вниз 2- слева вниз 3- слева вверх 4- справа вверх 
local y;
y=map.y_lap+(128*spin);
map.y_lap=y;
map.road=map.road+1;
local rotationof;
if (typeof==1) then rotationof=0; 
elseif (typeof == 2) then rotationof=4.71239
elseif (typeof==3) then rotationof=3.14159
else rotationof=1.5708 end
actor("user_sprite", map.x_lap,y, {name="turn"..map.road, texture="user/r", rotation=rotationof , layer=0});
end

local function CreateWayXTurn(typeof,spin)

local x;
x=map.x_lap+(128*spin);
map.x_lap=x;
map.road=map.road+1;
local rotationof;
if (typeof==1) then rotationof=0; 
elseif (typeof == 2) then rotationof=4.71239
elseif (typeof==3) then rotationof=3.14159
else rotationof=1.5708 end
actor("user_sprite", x,map.y_lap, {name="turn"..map.road, texture="user/r", rotation=rotationof , layer=0});
end

local function CheckpointXspawnOffset(playerresp,x,y)
---
local rotator;
if exists("helper"..x..y)==true then rotator=pget("helper"..x..y,"rotation") 
else rotator=0 end
---
local x_start = x - 32*2;
local pdot = math.random(1,5);

actor("tank",x_start+(pdot-1)*32,y,{name=playerresp.vehname, playername=playerresp.name, skin=playerresp.skin,rotation=rotator})
user.EquipRacer(playerresp.vehname)
end

local function CheckpointYspawnOffset(playerresp,x,y)
---
local rotator;
if exists("helper"..x..y)==true then rotator=pget("helper"..x..y,"rotation") 
else rotator=0 end
---
local y_start = y - 32*2;
local pdot = math.random(1,5);

actor("tank",x,y_start+(pdot-1)*32,{name=playerresp.vehname, playername=playerresp.name, skin=playerresp.skin,rotation=rotator})
user.EquipRacer(playerresp.vehname)
end

local function BuildSpawnPointFor(tank)
local x,y,typeoffset;
if gamer[tank].point>0 and gamer[tank].point~=999 then
x,y = position("c"..gamer[tank].point)

---find type of checkpoint
if pget("c"..gamer[tank].point,"texture")=="user/point" or gamer[tank].point==1 then
typeoffset=1 --y offset
else 
typeoffset=2 --x offset
end
---end of find

else x,y = position("c1") 
typeoffset=1
end

local tank_spawn=object(gamer[tank].playername);

---create spawn point
		if typeoffset==1 then
					CheckpointYspawnOffset(tank_spawn,x,y)
		else
					CheckpointXspawnOffset(tank_spawn,x,y)
		end
---end of create
if options.wathtype==1 then lapviewattach() end
end



local function FillDinamicObjectsBox(xstart,ystart,xend,yend,random_type)
for current_y = ystart,yend do
	for current_x = xstart,xend do
	if (random_type==1) then 
	if (math.random(1,20)<=1) then
	actor("crate", get32(current_x),get32(current_y), {rotation=math.random(1,6)}) end
	else
	actor("crate", get32(current_x),get32(current_y), {}) 
	end
	end
end
end

local function FillWallBox(xstart,ystart,xend,yend)
for current_y = ystart,yend do
	for current_x = xstart,xend do
	if  current_x == xstart or current_x == xend or current_y==ystart or current_y==yend then
	map.walls=map.walls+1
	actor("wall_concrete", get32(current_x),get32(current_y), {name="wall"..map.walls})
	end
	end
end
end

local function FillWallBoxWithDoors(xstart,ystart,xend,yend,cutx,cutxend,cutxypos,cuty,cutyend,cutyxpos)
for current_y = ystart,yend do
	for current_x = xstart,xend do
	if 	(current_x>=cutx and current_x<=cutxend and cutxypos==current_y) or (current_y>=cuty and current_y<=cutyend and cutyxpos==current_x) then
	elseif  current_x == xstart or current_x == xend or current_y==ystart or current_y==yend	then
	map.walls=map.walls+1
	actor("wall_concrete", get32(current_x),get32(current_y), {name="wall"..map.walls})
	end

	end
end
end

local function FillWoodBox(xstart,ystart,xend,yend,random_type)
for current_y = ystart,yend do
	for current_x = xstart,xend do
	if (random_type==1) then 
	if (math.random(1,4)>=3) then
	actor("wood", get32(current_x),get32(current_y), {name="c"..current_x..current_y}) end
	else
	actor("wood", get32(current_x),get32(current_y), {name="c"..current_x..current_y})
	end
	end
end
end

local function CreateRandomResps(xstart,ystart,xend,yend,teamp,num,dirp)
for n = 1,num do
if dirp<=6 then
	actor("respawn_point", get32(math.random(xstart,xend)), get32(math.random(ystart,yend)), {name="rp"..n,team=teamp,dir=dirp})
else
	actor("respawn_point", get32(math.random(xstart,xend)), get32(math.random(ystart,yend)), {name="rp"..n,team=teamp,dir=math.random(1,6)})
end
end
end

local function CreateLap(x,y)
map.x_lap=x;
map.y_lap=y;
map.road=1;
map.checkpoints=map.checkpoints+1;
CheckpointYtriggerOffset(map.checkpoints,x,y)
actor("respawn_point", x, y+32, {name="r1",team=1})
actor("respawn_point", x, y-32, {name="r2",team=1})
actor("respawn_point", x-64, y+32, {name="r3",team=1})
actor("respawn_point", x-64, y-32, {name="r4",team=1})
user.svetik=actor("user_object", x,y+96, {name="svetofor", health=100,max_health=100,texture="user/disable"});
actor("user_sprite", x,y, {name="c1", texture="user/finish", layer=1});
actor("user_sprite", x,y, {name="start"..map.road, texture="user/f", layer=0});
end

function user.MakeCount(t)
if (t==3) then
if user.svetik.texture ~= "user/disable" then return end
user.svetik.texture = "user/red";
music "..\\campaign\\TheRace\\sound\\h.ogg"
pushcmd(function() user.MakeCount(2) end, 1)
elseif (t==2) then 
if user.svetik.texture ~= "user/red" then return end
user.svetik.texture = "user/yellow";
music "..\\campaign\\TheRace\\sound\\h.ogg"
pushcmd(function() user.MakeCount(1) end, 1)
elseif (t==1) then 
if user.svetik.texture ~= "user/yellow" then return end
user.svetik.texture = "user/green";
music "..\\campaign\\TheRace\\sound\\h.ogg"
pushcmd(function() user.MakeCount(0) end, 1)
else 
if user.svetik.texture ~= "user/green" then return end
music "..\\campaign\\TheRace\\sound\\s.ogg"
pushcmd(function() m(options.music) end, 0.7)
user.svetik.texture = "user/disable";
user.svetik.name="svetoforvzriv"
map.start=1;
gamer['tank1'].point=0;
gamer["tank1"].laps=0
if map.lockplayer>0 then pset("player","profile",gamer['tank1'].profile) end
local bot;
for n = 1,3 do
bot=object("bot_tank"..n)
gamer[bot.name].point=0
gamer[bot.name].laps=0
user.CheckPoint(1,0,0,bot) --авто активация первой точки. Дает добро на старт ботов
	if map.withguns>0 then	pushcmd(function() user.RefreshAttackList() end, 3)	end
	if map.currentmap==5 then pset(gamer[bot.name].playername,"active",1)	end
	
end  
if map.currentmap==5 then
	local nameb="bot_tank"..math.random(1,3)
	pset(gamer[nameb].playername, "on_die", "user.KillRemouter()") 
	pset(gamer[nameb].playername, "team", 2) 	
	pset(nameb, "max_health", 900)
	pset(nameb, "health", 900) 
	end
if map.police>0 then 
	for n = 4,10 do
	if exists("punisher"..n)==true then
	local punish;
	punish=object(gamer["punisher"..n].playername)
	punish.active=1
	end
	end
end
pushcmd(function() if exists("svetoforvzriv") then damage(120, "svetoforvzriv") end end, 3) 
end
							end
							
---------------------РЕДАКТОР И УРОВЕНЬ---------------
local function Defaults()
map = nil
map = {}
------------Системное для генерации карты----------
map.x_lap=0
map.y_lap=0 --текущая координата для генерации круга
map.road=0 --количиство дорожных блоков на карте
map.walls=0 -- количество стен на карте
map.checkpoint=5
map.night=0
map.checkpoint_default=5 
map.checkpoints=0 --количество чекпоинтов на карте
---------------------------------------------------
map.currentmap=0 -- текущий ID карты
map.respawns=0 -- вкл \ выкл респавны
map.police=0 -- полиция
map.withguns=0 --оружие у гоньщиков
map.lockplayer=0 --блокировать ли игрока на старте
map.start=0 --начата ли гонка
map.respawn_delai=0.8 --задержка при респавне

gamer = {}

if exists("tank1") == true then
	kill("tank1")
end
for n = 1,3 do
	if exists("bot_tank"..n) == true then
	kill("bot_tank"..n)
	end
end	
for n = 1,10 do
	if exists("punisher"..n) == true then
	kill("punisher"..n)
	end
end	

end

local function LoadLevel(id)
	if id==1 then
		loadmap(campdir.."maps/128x128.map")
		CreateLap(22*32,22*32);
		CreateWayXForward(10,1);
		CreateWayXTurn(1,1);
		CreateWayYForward(10,1);
		CreateWayYTurn(4,1);
		CreateWayXForward(12,-1);
		CreateWayXTurn(3,-1);
		CreateWayYForward(10,-1);
		CreateWayYTurn(2,-1);
		CreateWayXForward(2,1);
		FillWoodBox(22,30,59,60,1)
		FillDinamicObjectsBox(26,15,65,20,1)
		if map.police>0 then CreateRandomResps(22,30,59,60,2,10,9) end 
	elseif id==2 then
		loadmap(campdir.."maps/128x128.map")
		CreateLap(22*32,22*32);
		CreateWayXForward(10,1);
		CreateWayXTurn(1,1);
		CreateWayYForward(5,1);
		CreateWayYTurn(3,1);
		CreateWayXForward(5,1);
		CreateWayXTurn(1,1);
		CreateWayYForward(5,1);
		CreateWayYTurn(4,1);
		CreateWayXForward(17,-1);
		CreateWayXTurn(3,-1);
		CreateWayYForward(11,-1);
		CreateWayYTurn(2,-1);
		FillWoodBox(22,30,59,60,1)
		FillDinamicObjectsBox(26,15,65,20,1)
		if map.police>0 then CreateRandomResps(22,30,59,60,2,10,9) end
	elseif id==3 then
		loadmap(campdir.."maps/128x128.map")
		CreateLap(22*32,22*32);
		CreateWayXForward(10,1);
		CreateWayXTurn(1,1);
		CreateWayYForward(5,1);
		CreateWayYTurn(3,1);
		CreateWayXForward(5,1);
		CreateWayXTurn(1,1);
		CreateWayYForward(5,1);
		CreateWayYTurn(4,1);
		CreateWayXForward(17,-1);
		CreateWayXTurn(3,-1);
		CreateWayYForward(11,-1);
		CreateWayYTurn(2,-1);
		FillWoodBox(22,30,59,60,1)
		if map.police==0 then FillWallBox(22,30,59,60) end
		FillDinamicObjectsBox(26,15,65,20,1)
		if map.police>0 then CreateRandomResps(22,30,59,60,2,10,9) end
	elseif id==4 then
	-- type 1 - справа вниз 2- слева вниз 3- слева вверх 4- справа вверх 
		loadmap(campdir.."maps/500x500.map")
		CreateLap(22*32,22*32);
		CreateWayXForward(40,1);
		CreateWayXTurn(1,1);
		CreateWayYForward(5,1);
		CreateWayYTurn(3,1);
		CreateWayXForward(5,1);
		CreateWayXTurn(1,1);
		CreateWayYForward(5,1);
		CreateWayYTurn(3,1);
		CreateWayXForward(5,1);
		CreateWayXTurn(1,1);
		CreateWayYForward(5,1);
		CreateWayYTurn(3,1);
		CreateWayXForward(5,1);
		CreateWayXTurn(1,1);
		CreateWayYForward(5,1);
		CreateWayYTurn(3,1);
		CreateWayXForward(5,1);
		CreateWayXTurn(1,1);
		CreateWayYTurn(4,1);
		CreateWayXForward(10,-1);
		CreateWayXTurn(3,-1);
		CreateWayYForward(5,-1);
		CreateWayYTurn(1,-1);
		CreateWayXForward(5,-1);
		CreateWayXTurn(3,-1);
		CreateWayYForward(5,-1);
		CreateWayYTurn(1,-1);
		CreateWayXForward(5,-1);
		CreateWayXTurn(3,-1);
		CreateWayYForward(5,-1);
		CreateWayYTurn(1,-1);
		CreateWayXForward(5,-1);
		CreateWayXTurn(3,-1);
		CreateWayYForward(5,-1);
		CreateWayYTurn(1,-1);
		CreateWayXForward(30,-1);
		CreateWayXTurn(2,-1);
		CreateWayYForward(5,1);
		CreateWayYTurn(4,1);
		CreateWayXForward(5,-1);
		CreateWayXTurn(3,-1);
		CreateWayYForward(6,-1);
		CreateWayYTurn(2,-1);
		FillWallBoxWithDoors(22,30,59,60,38,46,30,46,54,22) --почти как в LOSTе ...волшебные цифры :)
		actor("turret_cannon", 55*32, 36*32, {name="t1", health=100 , team=2} )
		actor("turret_cannon", 28*32, 46*32, {name="t2", health=100 , team=2} )
		FillDinamicObjectsBox(26,15,65,20,1)
		if map.police>0 then CreateRandomResps(22,30,59,60,2,10,9) end
	elseif id==5 then
		loadmap(campdir.."maps/64x64.map")
		loadtheme("campaign/TheRace/textures.lua")
		FillWallBox(1,1,64,64)
		FillWallBoxWithDoors(5,5,59,59,35,41,5,36,44,59)
		FillWallBoxWithDoors(10,10,45,45,22,29,10,12,17,10)
		CreateLap(get32(14),get32(14));
		if map.police>0 then CreateRandomResps(2,61,63,63,2,10,9) end
		pushcmd(function() user.CreateRandomWeapon() end, 5)
	elseif id==6 then
		loadmap(campdir.."maps/128x128.map")
		CreateLap(22*32,22*32);
		CreateWayXForward(10,1);
		CreateWayXTurn(1,1);
		CreateWayYForward(10,1);
		CreateWayYTurn(4,1);
		CreateWayXForward(12,-1);
		CreateWayXTurn(3,-1);
		CreateWayYForward(10,-1);
		CreateWayYTurn(2,-1);
		CreateWayXForward(2,1);
		FillWoodBox(22,30,59,60,1)
		FillDinamicObjectsBox(26,15,65,20,1)
		actor("weap_zippo",math.random(25,35)*32, math.random(35,56)*32, {name="zippo", health=100 , team=2} )
		actor("turret_gauss", 24*32, 57*32, {name="t1", health=100 , team=2} )
		if map.police>0 then CreateRandomResps(22,30,59,60,2,10,9) end 
	end
end

function user.new(lap,resp,spolice,map_id,cars,guns,resurection,isplayerlocked,nightmode,freeqsp)
if lap==nil then return message("используй user.new(круги>0,респы(0,1),полиция(0,1),ID карты(1-5),машина(\"спортмашина\" or \"формула\"),оружие(0,1),жизни,запретить фальш-старт(0,1),расстояние до чекпоинта( параметр не обязателен))")
end

--pause(false)
local carbot,classbot,carplayer,classplayer;
if cars=="формула" then
carbot="f1_blue"
classbot="user/f1bot"
carplayer="f1"
classplayer="user/f1"
elseif cars=="спортмашина" then
carbot="car_blue"
classbot="user/car_bot"
carplayer="car_red"
classplayer="user/car"
end

Defaults()

if nightmode=="день" then conf.sv_nightmode = false map.night=0
else 
conf.sv_nightmode = true 
map.night=1
end

if freeqsp ~=nil then
map.checkpoint=freeqsp
map.checkpoint_default=freeqsp
end

map.lockplayer=isplayerlocked;
map.withguns=guns;
map.police=spolice;
map.respawns=resp;
map.freeteam=5;
map.lapsmap=lap;	

map.currentmap=map_id
LoadLevel(map_id)

CreatePlayer("player","tank1",carplayer,classplayer,resurection)

if map.lockplayer>0 then
gamer['tank1'].profile=pget("player","profile")
pset("player","profile","noone_never_enteres")
end
------bot power reduce part-------------
local low = options.botreduce*100
if user.cpower==nil or user.cmax_speed==nil or user.cpower==0 or user.cmax_speed==0 then 
user.cpower=classes[classbot].power[1]
user.cmax_speed=classes[classbot].max_speed[1]
end
classes[classbot].max_speed[1]=user.cmax_speed-low
classes[classbot].power[1] = user.cpower-low
----------------------------------------
for n = 1,3 do
	CreateBot("enemy"..n,"bot_tank"..n,carbot,classbot,resurection)
end	
	if map.police>0 then
----
	local classp;
	if map_id==5 then classp="user/truck"
	else classp="user/car"
	end
----

		for n = 4,10 do
			local bot=CreateBot("enemy"..n,"punisher"..n,"police",classp,3)
			gamer["punisher"..n].inrace=0;
			bot.on_die="kill('enemy"..n.."')"
			bot.on_respawn="user.EraseSpawnPointsP() user.EquipPunisher('punisher"..n.."')"
			bot.team=2
		end			
	end
	loadtheme("campaign/TheRace/logo.lua")
	user.menuservice = service("menu",{title="mytitle",name="menu",names="Игра|Генератор|Настройки|О Аддоне",on_select="user.MainMenu(n)", open=0})
		pushcmd(function() user.MakeCount(3) end, 3)
end

-----ДЛЯ МИССИИ 6-------------	

function user.CreateRandomWeapon()
if exists("notattached_player_w")==true then
	kill("notattached_player_w")
end
if exists("attached_player_w")==false then
	local pos=math.random(1,8)
	local x,y;
	if pos==1 then x=3 y=28
	elseif pos==2 then x=3 y=55
	elseif pos==3 then x=25 y=54
	elseif pos==4 then x=61 y=50
	elseif pos==5 then x=52 y=24
	elseif pos==6 then x=51 y=10
	elseif pos==7 then x=22 y=18
	else x=21 y=40
	end
	actor("weap_cannon", get32(x), get32(y), {name="notattached_player_w", on_pickup="user.AttachRandomedWeapon(who)"})
	pushcmd(function() user.CreateRandomWeapon() end, 5)
end
end

function user.AttachRandomedWeapon(who)
if who.name == "tank1" then
pset("notattached_player_w","name","attached_player_w")
end
end
------------------------------


function user.KillRemouter()
--if who.name ~= 'tank1' then message("Командующий убит, но не вами. Вы проиграли!") user.ShowMenu(1) return end
options.stage=options.stage+1
message("Командующий убит. Победа!")
	for n = 1,3 do
		gamer["bot_tank"..n].inrace=0
		local bot;
		bot=object(gamer["bot_tank"..n].playername)
		bot.active=0
	end
	for n = 4,10 do
		if exists("punisher"..n)==true then
		local punish;
		punish=object(gamer["punisher"..n].playername)
		punish.active=0
		end
	end
user.save() 
user.ShowMenu(1)
end							
							
function user.RefreshAttackList()
for n = 1,3 do
	if gamer["bot_tank"..n].inrace>0 and exists("bot_tank"..n)==true and map.start ~= 0 then
			local randomarrow=math.random(1,4);
			local nameofarrow;
				if randomarrow == 4 then nameofarrow="tank1"
				elseif randomarrow == n then if n<3 then nameofarrow="bot_tank"..randomarrow+1 else nameofarrow="bot_tank"..randomarrow-1 end
				else  nameofarrow="bot_tank"..randomarrow
				end
		if (exists(nameofarrow)==true) then
			ai_attack(gamer["bot_tank"..n].playername, nameofarrow)	end
		local bot=object("bot_tank"..n)
		if gamer["bot_tank"..n].point==999 then user.CheckPoint(1,0,0,bot) 
		else
		user.CheckPoint(gamer["bot_tank"..n].point,0,0,bot)
		end
	end
end
if map.withguns>0 then pushcmd(function() user.RefreshAttackList() end, 2) end
end		

------------------Функции в таблице User---------------------------------------------------				

function user.EquipPunisher(name)
if (map.currentmap==5) then 
	pset(gamer[name].playername,"skin","zil")
end
   	local w=actor("weap_cannon", 0, 0, {})
	equip(name, w)
	
end

function user.EquipRacer(name)
if map.withguns>0 and gamer[name].inrace>0 then
	if exists("gun_"..name)==false then 
	local w=actor("weap_minigun", 0, 0, {name="gun_"..name, respawn_time=1000000})
	equip(name, w)  
	else
	equip(name, "gun_"..name) 
	end
	gamer[name].gun="gun_"..name;
end
end

function user.OnDie(name)
if map.respawns>0 then
	local player = object(name);
	gamer[player.vehname].lifes=gamer[player.vehname].lifes-1 ;
	gamer[player.vehname].dead=1;
		if gamer[player.vehname].lifes>0 then
		message(player.nick.." погиб. Осталось ["..gamer[player.vehname].lifes.."] авто. Он продолжает гонку");
		else 
		message(player.nick.." погиб. Разбил все свои автомобили. Он выбывает из гонки");
		end
	if map.respawns==0 or gamer[player.vehname].lifes==0 then 
	gamer[player.vehname].inrace=0
		if player.name=="player" then
		pushcmd(function() user.ShowMenu(2) end,3)
		end
	kill(player)
	else 
	pushcmd(function() BuildSpawnPointFor(player.vehname) end,map.respawn_delai)
	end 
else kill(name) if name=="player" then pushcmd(function() user.ShowMenu(2) end,3) end
end
end

function user.CheckPoint(id,x,y,kto)
if (map.currentmap ~= 5) then
if (map.start ~= 0 and gamer[kto.name].inrace~=0) then 
local check,xb,yb;
	if gamer[kto.name].point==999 and id==1 then
		gamer[kto.name].point=1;
		gamer[kto.name].laps=gamer[kto.name].laps+1;
		if gamer[kto.name].laps<map.lapsmap then
		message(pget(gamer[kto.name].playername,'nick').." начал "..(gamer[kto.name].laps+1).." круг из "..map.lapsmap)
		else 
			if kto.name~="tank1" then
			message(pget(gamer[kto.name].playername,'nick').." победил в гонке")
			pushcmd(function() user.ShowMenu(3) end,3)
			else
			message("Вы победили в гонке")	
			pushcmd(function() user.ShowMenu(4) end,1)
			options.stage=options.stage+1
			user.save() 
			end
		gamer["tank1"].inrace=0
			for n=1,3 do
			local bot;
				if exists("bot_tank"..n)==true then
				bot=object(gamer["bot_tank"..n].playername)
				bot.active=0
				end
			gamer["bot_tank"..n].inrace=0
			end
			if map.police>0 then 
				for n = 4,10 do
				if exists("punisher"..n)==true then
				local punish;
				punish=object(gamer["punisher"..n].playername)
				punish.active=0
				end
				end
			end
		end
		if kto.name=="tank1" then
			if gamer['tank1'].inrace>0 then
				if options.wathtype<1 then lapviewattach(x,y) 
				else if gamer['tank1'].counter==nil then lapviewattach() end
				end
			end
		else
			check=object("t3c2")
			xb,yb = position(check)
			ai_march(gamer[kto.name].playername,xb,yb)
		end				
	elseif id==map.checkpoints and gamer[kto.name].point==id-1 then
		gamer[kto.name].point=999;
		if kto.name=="tank1" then
		message(id.." чекпоинт пройден!")
			if options.wathtype<1 then   lapviewattach(x,y) 
			else if gamer['tank1'].counter==nil then lapviewattach() end
			end
		else
			check=object("t3c1")
			xb,yb = position(check)
			ai_march(gamer[kto.name].playername,xb,yb)
		end			
	elseif id==gamer[kto.name].point+1 then
		gamer[kto.name].point=id;
		if kto.name=="tank1" then	
		message(id.." чекпоинт пройден!")
			if options.wathtype<1 then lapviewattach(x,y) 
			else if gamer[kto.name].counter==nil then lapviewattach() end
			end
		else 
			check=object("t3c"..id+1)
			xb,yb = position(check)
			ai_march(gamer[kto.name].playername,xb,yb)
		end		
	elseif id==gamer[kto.name].point then
		if kto.name~="tank1" then			
			check=object("t3c"..id+1)
			xb,yb = position(check)
			ai_march(gamer[kto.name].playername,xb,yb)
		end	
	elseif id~=gamer[kto.name].point and gamer[kto.name].point~=nil then
		if kto.name~="tank1" then
			if gamer[kto.name].point==999 then 
			check=object("t3c1")
			elseif exists("t3c"..gamer[kto.name].point) == true then 
			check=object("t3c"..gamer[kto.name].point) 
			end
			xb,yb = position(check)
			ai_march(gamer[kto.name].playername,xb,yb)
		end
	end
else if ((kto.name == 'tank1') and (id > 1) and (gamer[kto.name].inrace~=0)) then message("Вернитесь на старт!") end

end 

end
					
end 
--------------------Функции для организации меню игры------------------------------------------
function user.MainMenu(n)
if n==1 then user.ShowMenu(1) 
elseif n==2 then user.RandomMessageBox()
elseif n==3 then user.OptionsMenu()
else user.AboutBox()
end
end

function user.ShowMenu(t)
local textmes,stage_name,opisanie;
opisanie="";
if t==1 then textmes=" Запуск уровня";
elseif t==2 then textmes="Вы умерли. Запустить заново ?";
elseif t==3 then textmes="Вы проиграли.  Запустить заново ?";
elseif t==4 then textmes="Вы победили.  Продолжить?";
end
if options.stage==1 then stage_name="Гонка новичка" opisanie="Миссия [\""..stage_name.."\"] \n Здесь вам предстоит набраться опыта и освоить основы здешних гоночных правил\n У вас есть 1 попытка, после которой миссия будет провалена.\n Продержитесь 8 кругов и приедте к финишу первым!"
elseif options.stage==2 then stage_name="Погоня" opisanie="Миссия [\""..stage_name.."\"] \n Правила те же, только теперь по вам еще ведут огонь на поражение.\n У вас есть 3 попытки, после которых миссия будет провалена.\n Продержитесь 4 круга и приедте к финишу первым!" 
elseif options.stage==3 then stage_name="Продолжение погони" opisanie="Миссия [\""..stage_name.."\"] \n Оганизаторы решили испытать игроков на прочность.\n Теперь у каждого гонщика в арсенале появился Миниган! \n У вас есть 5 машин, если разобьете все, то миссия будет провалена.\n Продержитесь 8 кругов и приедте к финишу первым!" 
elseif options.stage==4 then stage_name="Скоростной заезд" opisanie="Миссия [\""..stage_name.."\"] \n Вам доверили более скоростную тачку, так что не подведите спонсоров. Опробуйте её на подходящем для скоростных гонок треке\n У вас есть 3 машины, если разобьете все, то миссия будет провалена.\n Продержитесь 8 кругов и приедте к финишу первым!" 
elseif options.stage==5 then stage_name="Скоростной заезд c полицией" opisanie="Миссия [\""..stage_name.."\"] \n Правила те же, только теперь по вам еще ведут огонь на поражение...\n нечего было ездить по территории военной базы, так что стационарки это еще пол беды\n У вас есть 3 машины, если разобьете все, то миссия будет провалена.\n На карте раз в 5 секунд появляется оружие, вы должны успеть его заполучить!" 
elseif options.stage==6 then stage_name="Охота" opisanie="Миссия [\""..stage_name.."\"] \n Организаторы решили сделать что-то вроде охоты, за вами гоняются на 10 тонных махинах\n способных раздавить вас за считаные секунды\n От вас требуется выжить и найти подсадного игрока который управляет всей сворой. Убъете его - сумеете остановить всех и победить!\n У вас есть 3 машины, если разобьете все, то миссия будет провалена.\n На карте раз в 5 секунд появляется оружие, вы должны успеть его заполучить!" 
elseif options.stage==7 then stage_name="Ночная прогулка" opisanie="Миссия [\""..stage_name.."\"] \n Вас решили вернуть заниматься привычным делом - гоняться за первое место\n Правдо в этот раз поездка выпала на ночное время суток\n плюс организаторы как всегда придумали что-то новенькое\n чтобы привлечь публику , вам об их планы конечно не раскрываются\n У вас 2 машины. Цель проехать 4 круга" 
elseif options.stage>stage_total then stage_name="ПОЗДРАВЛЯЕМ" opisanie=stage_name.."\n Вы прошли все уровни компании\n Чтобы запустить еще уровень ,\n нажмите в меню кнопку Генератор\n либо выполните команду в консоле user.Int(\"уровень\")"
end

if t > 1 then pause(true)   end        
if (exists("msbox")==false) then
if options.stage<=stage_total then
	service("msgbox", {name="msbox", text=textmes.."\n"..opisanie,on_select="if n==1 then user.Int("..options.stage..") if "..t.." == 1 then user.menuservice.open=1 end end if "..t.." > 1 then pause(false)  end", option1="Запустить", option2="Закрыть"})
else
	service("msgbox", {name="msbox", text=textmes.."\n"..opisanie,on_select="if "..t.." > 1 then pause(false) end", option1="Закрыть"})
end
end
end

function user.AboutBox()
if (exists("aboutbox")==false) then
service("msgbox", {name="aboutbox", text="Данный проект разрабатывался в память игре MicroMachines\nСуть другая, но идея создания возникла именно после общения с тем проектом\n(c)\n Автор проекта NC22 aka A_K \n (c) Некоторые скины перенесены из MicroMachines 2"})
end
end

function  user.LevelChangeBox(n)
if n~=nil then
if n==1 then options.botreduce=options.botreduce-1 user.save()
elseif n==2 then options.botreduce=options.botreduce+1 user.save()
end
end
local levelname;
if options.botreduce==1 then levelname="Средний"
elseif options.botreduce==2 then levelname="Ниже среднего"
elseif options.botreduce==3 then levelname="Легкий"
elseif options.botreduce==4 then levelname="Сверх легкий"
elseif options.botreduce==5 then levelname="для дошколят"
elseif options.botreduce==0 then levelname="Сложный"
elseif options.botreduce==-1 then levelname="Аццки сложный"
elseif options.botreduce==-2 then levelname="Непобедимый"
elseif options.botreduce<-2 then  levelname="Чтото нереальное"
elseif options.botreduce>5 then  levelname="еще легче =/"
end
local low = options.botreduce*100
if n ~= 3 then
service("msgbox", {text="Настройка уровня сложности\n Вы сейчас играете на уровне сложности \n\n\n\n           ["..levelname.."]\n\n\n\n",on_select="user.LevelChangeBox(n)",option1="Повысить",option2="Понизить",option3="Закрыть"})
	for n = 1,3 do
		if exists("enemy"..n) then
		   local bot = object("enemy"..n)
		   if user.cpower==nil or user.cmax_speed==nil or user.cpower==0 or user.cmax_speed==0 then 
			user.cpower=classes[bot.class].power[1]
			user.cmax_speed=classes[bot.class].max_speed[1]
		   end
		   classes[bot.class].max_speed[1]=user.cmax_speed-low
		   classes[bot.class].power[1] = user.cpower-low
		   bot.class=bot.class
		end
	end	
end
end

function user.ShowLapsTypeBox(n)
if n==nil then
	if options.wathtype>0 then
		service("msgbox", {text="Пробная настройка отображения информации о позиции игрока\n и пройденых кругах",on_select="user.ShowLapsTypeBox(n)",option1="Отменить",option2="Закрыть"})
	else
		service("msgbox", {text="Пробная настройка отображения информации о позиции игрока\n и пройденых кругах",on_select="user.ShowLapsTypeBox(n)",option1="Применить",option2="Закрыть"})
	end
else
	if n==1 then
		if options.wathtype>0 then options.wathtype=0
		else options.wathtype=1 end
		user.save()
	end
end
end

local Refresh=function()
if exists(user.menuservice.name)==true then
--фактически пересоздаем меню если нам надо посмотреть изменения
user.menuservice.open=1
user.menuservice.open=1
end
end

function user.OptionsMenu(n)
user.menuservice.names="Отоб. круга|Сложность|Музыка|Назад"
user.menuservice.on_select="user.OptionsMenu(n)"
Refresh()
 --для обновления меню
if n == nil then return end
if n ==1 then user.ShowLapsTypeBox()
elseif n == 2 then user.LevelChangeBox()
elseif n == 3 then user.MusicBox()
else user.menuservice.names="Игра|Генератор|Настройки|О Аддоне"
     user.menuservice.on_select="user.MainMenu(n)"
	 Refresh()
end
end

function user.MusicBox(n)
if n~=nil then
	local newm;
	if n==1 and options.music-1~=0 then newm=options.music-1 m(newm) options.music=newm user.save()
	elseif n==2 then newm=options.music+1 
		if newm~=options.music then 
				if m(newm)~=0 then options.music=newm
				else m(options.music)
				end
		end
		user.save()
	end
end
service("msgbox", {text="Выберите композицию \n\n Сейчас №"..options.music,on_select="if n<3 then user.MusicBox(n) end",option1="Предыдущая",option2="Следующая",option3="Закрыть"})

end

function user.RandomMessageBox()
--pause(true)
local textmes,lap,resp,spolice,map_id,cars,guns,resurection,isplayerlocked;
lap=math.random(1,8) textmes="Трасса:\n"..lap.." кругов\n"
resp=math.random(0,1) if resp==1 then textmes=textmes.."Респавны: Включены\n" else textmes=textmes.."Респавны: Выключены\n" end
spolice=math.random(0,1) if spolice==1 then textmes=textmes.."Полиция: Включена\n" else textmes=textmes.."Полиция: Выключена\n" end
map_id=math.random(1,3) textmes=textmes.."ID карты: "..map_id.."\n"
cars=math.random(1,2) if cars==1 then cars="спортмашина" textmes=textmes.."Авто: Спортивная машина\n" else cars="формула" textmes=textmes.."Авто: Формула-1\n" end
guns=math.random(0,1) if guns==1 then textmes=textmes.."Оружие: Включено\n" else textmes=textmes.."Оружие: Выключено\n" end
resurection=math.random(3,15)  if resp==1 then textmes=textmes.."Здоровье: "..resurection.."\n" end
isplayerlocked=math.random(0,1) if isplayerlocked==1 then textmes=textmes.."Фальш-старт: Вылючен" else textmes=textmes.."Фальш-старт: Включен" end

if (exists("randomlevelbox")==false) then
service("msgbox", {
name="randomlevelbox",
text=textmes,on_select="if n==1 then user.new("..lap..","..resp..","..spolice..","..map_id..",'"..cars.."',"..guns..","..resurection..","..isplayerlocked..")  user.menuservice.open=1 elseif n==2 then kill('randomlevelbox') user.RandomMessageBox() end", option1="Запустить", option2="Еще раз", option3="Закрыть"})
end
end
------------------------------------------------------------------------------------------



local function ShowLogo()
if exists("rights")==false then
actor("user_sprite", 20,20, {name="rights", texture="gui_logos"})
end
end

function user.Int(n)
if n==nil then return message("используй user.Int(Номер уровня)") end

--user.new(lap,resp,spolice,map_id,cars,guns,resurection,isplayerlocked,freeqsp)
--|количество кругов|респы|полиция|ID карты|машина игрока\соперников|Использовать оружие|Количество попыток|Заблокировать игрока на старте до начала гонки| как часто спавнить чекпоинты на карте ( параметр не обязателен)---

if n==1 then
user.new(8,0,0,1,"спортмашина",0,0,1,"день")
elseif n==2 then 
user.new(4,1,1,2,"спортмашина",0,3,1,"день")
elseif n==3 then
user.new(8,1,0,3,"спортмашина",1,5,1,"день")
elseif n==4 then
user.new(2,0,0,4,"формула",0,0,1,"день",10)
elseif n==5 then
user.new(2,1,1,4,"формула",0,3,1,"день",10)
elseif n==6 then
user.new(8,1,1,5,"спортмашина",0,3,1,"день")
elseif n==7 then
user.new(4,1,0,6,"спортмашина",0,2,1,"ночь")
end
		
end
-------------------Блок сохранения и загрузки
 function user.load()
 local text=""
 local char=""
 local n=1
 
 local file = io.open ("user_data.dat","r") 
 if file==nil then 
	return(nil) 
 end
 local content = file:read("*a")
 file:close() 

 for i=1,string.len(content) do
  char=(string.char(string.byte(content,i) ))
  
  if char~="|" then 
    text=text..char
  else
	if n==1 then 
	options.wathtype=tonumber(text)
	elseif n==2 then 
	options.botreduce=tonumber(text)
	elseif n==3 then
	options.stage=tonumber(text)
	elseif n==4 then
	options.music=tonumber(text)
	end
   n=n+1
   text=""
  end
 
 end
	return 1
end 


function user.save()
 local file = io.open ("user_data.dat","w") 

 file:write(tostring(options.wathtype).."|") 
 file:write(tostring(options.botreduce).."|") 
 file:write(tostring(options.stage).."|") 
 file:write(tostring(options.music).."|") 
 
 file:close() 
end

------------------Инициализация-----При первом запуске-------------------------------
loadtheme("campaign/TheRace/logo.lua")
loadmap(campdir.."maps/128x128.map")
ShowLogo()
user.menuservice = service("menu",{title="mytitle",name="menu",names="Игра|Генератор|Настройки|О Аддоне",on_select="user.MainMenu(n)"})

if user.load()==nil then
message('сохранение не найдено. Используются начальные настройки')
end

if (m(options.music)==0) then options.music=1 m(1) end
pushcmd(function() user.menuservice.open=1 end,0.1)		

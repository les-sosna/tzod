reset()


user.Vorota1Opened=0

-----для создания движущихся объектов-----------------
finished = {}
finished[0] = {}
cy = {}
cy[0] = {}
cx = {}
cx[0] = {}
----Функция дла создания массива объектов которые используются для одной стены
---1. name - тип объекта, будь то кирпичная стена, бетон и т.д.
---2. prefix - префик к названию массива объектов. Используется в дальнейшем в MoveWall первым параметром
---3. направление от точки старта вверх вниз влево вправо по отношению к сторонам экрана
---4. x_start,y_start начальная точка от которой создавать стену
---5. Длинна стены
function CreateMoveWall(name,prefix,orientation,x_start,y_start,dlinna,d)
if d==nil then d=1 end
	for n=1,dlinna do
		local x_add,y_add=GetOrientationByName(orientation);
		actor(name,user.get32(x_start),user.get32(y_start),{name=prefix..n})
		x_start=x_start+(x_add*d)
		y_start=y_start+(y_add*d)
	end
return prefix;
end

------функция приводящая объект в движение по определенному направлению
---Использование Параметры
-- 1 . Название Стены
-- 2 . Сколько элементов у стены (можно доработать скрипт чтобы не спрашивало но мне уже лень)
-- 3 . Направление движения
-- 4 . Задержка при перемещении
-- 5 . точка завершения пути. Соответствует точке на карте по Х или по У в зависимости от того в какую сторону вы создали стену.
function MoveWall(prefix,elements,orientation,delai,endpoint)
finished[prefix]=1
local x,y,brake;
local x_add,y_add=GetOrientationByName(orientation);
	for n=1,elements do
		x,y=position(prefix..n)
		x=x+x_add
		y=y+y_add
		setposition(prefix..n,x,y)
	if x-user.get32(endpoint)==0 or y-user.get32(endpoint)==0 then
	brake=true
	end
	end
	if brake~=true then
		pushcmd(function() MoveWall(prefix,elements,orientation,delai,endpoint) end, delai/16)
	else finished[prefix]=0
		OnWallClose(prefix)
	end
end

----пока можно крутить только ящики, поэтому особого применения не нашел
function user.rotateWall(prefix,fixedelement,startangle,endangle,x,y)
finished[prefix]=1
local xm,ym,brake;
local n=1;
if x ==nil or y ==nil then
x,y=position(prefix..fixedelement)
local k=1;
	while (exists(prefix..k)) do
	pset(prefix..k,'max_health',0)
	k=k+1
	end
end
 local wall = object(prefix.."1")
	if endangle==startangle then	
	local k=1;
	while (exists(prefix..k)) do
		pset(prefix..k,"rotation",0)
		setposition(prefix..k,cx[prefix..k],cy[prefix..k])
		--cx[prefix..n],cy[prefix..n]=position(prefix..k)
		k=k+1
	end
	return 	end
	
 while (exists(prefix..n)) do

		if cx[prefix..n]== nil or cy[prefix..n]==nil then 
		cx[prefix..n],cy[prefix..n]=position(prefix..n)
		end
	local newx = (cx[prefix..n]-x)*math.cos(math.rad(startangle)) - (cy[prefix..n]-y)*math.sin(math.rad(startangle)) +x
	local newy = (cy[prefix..n]-y)*math.cos(math.rad(startangle)) + (cx[prefix..n]-x)*math.sin(math.rad(startangle)) +y
	setposition(prefix..n,newx,newy)
	pset(prefix..n,"rotation",math.rad(startangle))
	n=n+1

 end
 if startangle>endangle then
 startangle=startangle-1
 else
 startangle=startangle+1
 end
pushcmd(function() user.rotateWall(prefix,fixedelement,startangle,endangle,xm,ym) end, 0.1/16)	
 
end
function user.test()
 user.rotateWall('yashiki',1,90,270)
end

function user.rotate()
user.rotator=user.rotator+1
if user.rotator==360 then user.rotator=0 end
local xm=user.get32(26)
local ym = user.get32(16)
local newx = (xm-user.get32(26))*math.cos(math.rad(user.rotator)) - (ym-user.get32(10))*math.sin(math.rad(user.rotator)) + user.get32(26)
local newy = (ym-user.get32(10))*math.cos(math.rad(user.rotator)) + (xm-user.get32(26))*math.sin(math.rad(user.rotator)) + user.get32(10)
user.myaso.rotation=math.rad(user.rotator)
--local newx = 26*math.cos(user.rotator) - 16*math.sin(user.rotator)
--local newy = 16*math.cos(user.rotator) + 26*math.sin(user.rotator)
setposition(user.myaso,newx,newy)
--message(newx.."|"..newy)
pushcmd(function() user.rotate() end,0.1)
end


--событие при ззавершении движения любой стены
function OnWallClose(name)

if IsOpeningInProcess('verh')~=1 and IsOpeningInProcess('niz')~=1 then 
if user.Vorota1Opened==1 then user.Vorota1Opened=0 return
else user.Vorota1Opened=1 return end
end
	if name=='verh' or name=='niz' then
		if user.Vorota1Opened>0 then pushcmd(function() if user.Vorota1Opened==1 then user.State('close') end end, 3)
		end
	end
end

----вспомогательная функция для определения направления движения----------------
function GetOrientationByName(name)
local x_add,y_add;
if name=="влево" then x_add=-1; y_add=0
elseif name=="вправо" then x_add=1; y_add=0
elseif name=="вниз" then y_add=1; x_add=0
elseif name=="вверх" then y_add=-1; x_add=0
end
return x_add,y_add;
end

------закрывается\движется ли еще "дверь" с именем
function IsOpeningInProcess(name)
if finished[name]==nil then return 0 
elseif finished[name]==1 then return 1
elseif finished[name]==0 then return 0
end
end

function user.get32(num)
return ((num-1) * 32) + 16;
end
-----для создания движущихся объектов----END----------

-----Вращение вокруг точки например дверей ангара и др.
function rotate(name,angle,startx,starty)
local x,y = position(name)
local newx = (startx-x)*math.cos(angle) - (starty-y)*math.sin(angle) + x
local newy = (starty-y)*math.cos(angle) + (startx-x)*math.sin(angle) + y
setposition(name,newx,newy)
pset(name,"rotation",math.rad(angle))
end
------Игровые функции----------------------------

function user.FillWallBoxWithDoors(xstart,ystart,xend,yend,cutx,cutxend,cutxypos,cuty,cutyend,cutyxpos)
for current_y = ystart,yend do
	for current_x = xstart,xend do
	if 	(current_x>=cutx and current_x<=cutxend and cutxypos==current_y) or (current_y>=cuty and current_y<=cutyend and cutyxpos==current_x) then
	elseif  current_x == xstart or current_x == xend or current_y==ystart or current_y==yend	then
	walls=walls+1
	actor("wall_concrete", user.get32(current_x),user.get32(current_y), {name="wall"..walls})
	end

	end
end
end

------Игровой пример. Закодим например ворота----------------------------
---их состояние---
function user.State(name)
if (name=='close') then
	if IsOpeningInProcess('verh')~=1 and IsOpeningInProcess('niz')~=1 then 
	MoveWall('verh',5,'вниз',0.1,8) 
	MoveWall('niz',5,'вверх',0.1,9)
	end
elseif (name=='open') then
	if IsOpeningInProcess('verh')~=1 and IsOpeningInProcess('niz')~=1 then  
	MoveWall('verh',5,'вверх',0.1,2)
	MoveWall('niz',5,'вниз',0.1,15)
	end
end
end


user.dead = {}
user.dead[0] = {}
------Меню для настройки ботов------------------
function user.Spawn()
for n=1,user.num-1 do
if exists("robot"..n)==false and user.dead['robot'..n]~=1 then 
local tank = object("bottank"..n) 
service("ai",{name="robot"..n,on_die=" user.dead['robot"..n.."']=1 kill(\"robot"..n.."\")", active=0})
tank.playername="robot"..n
end
end
user.menuservice.names="Запустить мозги"
user.menuservice.on_select="user.Launch()"
user.menuservice.open=1
message("Для того чтобы боты запустились выберите в меню соответствующий пункт :)")
end

function user.Launch()
for n=1,user.num-1 do
if exists("robot"..n)==true then 
local tankb = object("robot"..n) 
tankb.active=1
end
end
user.menuservice.names="Выкинуть ботов"
user.menuservice.on_select="user.Offline()"
user.menuservice.open=1
message("brain's enabled")
end

function user.Offline()
for n=1,user.num-1 do
if exists("robot"..n)==true then 
local tank = object("bottank"..n) 
tank.playername=""
kill("robot"..n)
end
end
user.menuservice.names="Посадить ботов"
user.menuservice.on_select="user.Spawn()"
user.menuservice.open=1
end
--------------------------------------------

local TeleportTurret= function()
setposition(user.rocket,user.get32(math.random(2,20)),user.get32(math.random(15,20)))
pushcmd(function() user.TeleportTurret() end,1)
end
----скрипт карты---------
loadmap("campaign/TheRace/maps/64x64.map")
conf.sv_nightmode = false;
walls=0
rotat=0

user.menuservice = service("menu",{name="menu",names="Посадить ботов",on_select="user.Spawn()"})

------Спавним и рассаживаем игроков по танкам без точек респавна
user.num=1;
local nm=0
for current_y = 20,30 do
	for current_x = 8,24 do
		if  current_x == 8 or current_x == 24 or current_y==20 or current_y==30 then
		if (nm==3) then 
				nm=0
				actor('tank',user.get32(current_x),user.get32(current_y),{name="bottank"..user.num}) 
				user.num=user.num+1		
		end
		nm=nm+1
		end
	end
end


user.player=service("player_local",{name="ya"})
actor('tank',user.get32(9),user.get32(20),{name="playertank",playername="ya"})

--actor('tank',user.get32(9),user.get32(20),{})
---Тестирование движка---
---назначение класса "на горячую" для игрока
--classes["user/torm"] = tcopy(classes.default)
--classes["user/torm"].max_speed[1]=2000;
--classes["user/torm"].power[1] = 2300;
--pushcmd(function() user.player.class="user/torm" end,5)
-- на боте
--user.testingbot=service("ai",{name="bete"})
--actor('tank',user.get32(9),user.get32(18),{name="ete",playername="bete"})
--pushcmd(function() user.testingbot.class="user/torm" end,5)
---перемещение стацки
--user.rocket=actor('turret_rocket',user.get32(2),user.get32(2),{})
--pushcmd(function() user.TeleportTurret() end,2)
---26102616
user.rotator=0
user.myaso=actor('tank',user.get32(26),user.get32(16),{})
---------------------------------------
local symbols={["byNC22"]=0,["А"]=160,["Б"]=161,["В"]=162,["Г"]=163,
["Д"]=164,["Е"]=165,["Ж"]=166,["З"]=167,["И"]=168,
["Й"]=169,["К"]=170,["Л"]=171,["М"]=172,["Н"]=173,
["О"]=174,["П"]=175,["Р"]=176,["С"]=177,["Т"]=178,
["У"]=179,["Ф"]=180,["Х"]=181,["Ц"]=182,["Ч"]=183,
["Ш"]=184,["Щ"]=185,["Ъ"]=186,["Ы"]=187,["Ь"]=188,
["Э"]=189,["Ю"]=190,["Я"]=191,["а"]=192,["б"]=193,
["в"]=194,["г"]=195,["д"]=196,["е"]=197,["ж"]=198,
["з"]=199,["и"]=200,["й"]=201,["к"]=202,["л"]=203,
["м"]=204,["н"]=205,["о"]=206,["п"]=207,["р"]=208,
["с"]=209,["т"]=210,["у"]=211,["ф"]=212,["х"]=213,
["ц"]=214,["ч"]=215,["ш"]=216,["щ"]=217,["ъ"]=218,
["ы"]=219,["ь"]=220,["э"]=221,["ю"]=222,["я"]=223,
["Ё"]=165,["ё"]=197,[" "]=0,  ["!"]=1,  ["\""]=2,
["#"]=3,  ["$"]=4,  ["%"]=5,  ["&"]=6,  ["'"]=7,
["("]=8,  [")"]=9,  ["*"]=10, ["+"]=11, [","]=12,
["-"]=13, ["."]=14, ["/"]=15, ["0"]=16, ["1"]=17,
["2"]=18, ["3"]=19, ["4"]=20, ["5"]=21, ["6"]=22,
["7"]=23, ["8"]=24, ["9"]=25, [":"]=26, [";"]=27, 
["<"]=28, ["="]=29, [">"]=30, ["?"]=31, ["@"]=32,
["A"]=33,["B"]=34,["C"]=35,["D"]=36,
["E"]=37,["F"]=38,["G"]=39,["H"]=40,["I"]=41,
["J"]=42,["K"]=43,["L"]=44,["M"]=45,["N"]=46,
["O"]=47,["P"]=48,["Q"]=49,["R"]=50,["S"]=51,
["T"]=52,["U"]=53,["V"]=54,["W"]=55,["X"]=56,
["Y"]=57,["Z"]=58,["["]=59,["\\"]=60,["]"]=61,
["^"]=62,["_"]=63,["'"]=64,["a"]=65,["b"]=66,
["c"]=67,["d"]=68,["e"]=69,["f"]=70,["g"]=71,
["h"]=72,["i"]=73,["j"]=74,["k"]=75,["l"]=76,
["m"]=77,["n"]=78,["o"]=79,["p"]=80,["q"]=81,
["r"]=82,["s"]=83,["t"]=84,["u"]=85,["v"]=86,
["w"]=87,["x"]=88,["y"]=89,["z"]=90,["{"]=91,
["|"]=92,["}"]=93,["~"]=94
}
--"([А-я ,!ёЁ:.\"])"
local  print=function(text,x,y,name,font)
	local size=0.45
	if font==nil then font='font_default'
	else size=0.2 end
	local curx,cury; local n=0;		
	if name==nil then 
			name=""
			n=""
	end
    local t = string.gsub(text, "([^<])",
	function(ch) 
	if curx == nil or cury==nil then 
		curx=x
		cury=y
			if x==nil or y==nil then 
				message('введите правильно user.print(\'текст\',расстояние_между_символами,начало_по_х,по_у,имя)')
			end
	end
	if name~="" then n=n+1 end
	actor('user_sprite',((curx-1) * 32) + 16,((cury-1) * 32) + 16,{name=name.."dot"..n, texture=font, frame=symbols[ch]}) 
	curx=curx+size
	end)
   -- return t
end

function user.erase(name)
local n=1

while (exists(name.."dot"..n)) do
kill(name..n)
n=n+1
end

end

-----------------------------------------
print('Тестовая фраза 1: Эх, взъярюсь, толкну флегматика: дал бы щец жарчайших, Пётр!',10,18,'text2')
print('Тестовая фраза small text: Эх, взъярюсь, толкну флегматика: дал бы щец жарчайших, Пётр!',10,18.5,'text4','font_small')
print('Тестовая фраза 3: The quick brown fox jumps over the lazy dog. !@$#$^*&*()~',10,19,'text5','font_small')
print('Тестовая фраза 2: Хрюкнул ёж «Тыща», а ведь село Фершампенуаз - это центр Нагайбакского района Челябинской области.',10,17,'text')
print('Тестовая фраза 3: The quick brown fox!@$#$^*&*()~ jumps over the lazy dog.',10,16,'text3')
for n=1,4 do
actor("trigger", user.get32(13),user.get32(6+n),{on_enter="if user.Vorota1Opened>0 then pushcmd(function() if user.Vorota1Opened==1 then user.State('close') end end, 0.5) else user.State('open') end",radius=2})
end
for n=1,4 do
actor("trigger", user.get32(15),user.get32(6+n),{on_enter="if user.Vorota1Opened>0 then pushcmd(function() if user.Vorota1Opened==1 then user.State('close') end end, 0.5) else user.State('open') end",radius=2})
end
CreateMoveWall("crate","yashiki","вверх",24,10,8,1.05)
CreateMoveWall("wall_concrete","verh","вверх",14,8,5)
CreateMoveWall("wall_concrete","niz","вниз",14,9,5)
user.FillWallBoxWithDoors(6,6,13,11,0,0,0,7,10,13)

pushcmd(function() user.menuservice.open=1 end,0.1)
-----END-------------------------------
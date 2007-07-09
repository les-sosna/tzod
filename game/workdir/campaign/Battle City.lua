-- Battle City

reset()

conf.sv_timelimit = 0
conf.sv_fraglimit = 0
conf.sv_nightmode = false


classes["user/enemy"] = tcopy(classes.default)
classes["user/enemy"].health = 50;
classes["user/enemy"].power[1] = 700;


function user.Next()
	user.stage = user.stage + 1
	if user.stage > 5 then
		loadmap( "campaign/Battle City/maps/victory.map" )
	else
		user.enemies = 10;
		loadmap( "campaign/Battle City/maps/" .. tostring(user.stage) .. ".map" )
		
		service( "player_local", { 
			name="player1", 
			skin="yellow", 
			team=1, 
			on_die="user.OnDiePlayer()", 
			class="default", 
			nick="Player1" } )
			
		for n = 1,3 do
			service( "ai", { 
				name="enemy"..n, 
				skin="red", 
				team=2, 
				on_die="user.OnDieEnemy"..n.."()", 
				class="user/enemy",
				level=1 } )
		end
	end
end

function user.Defeat()
	message("Поражение!")
	pushcmd( function()	freeze(true) end, 1.0 )
	pushcmd( function() 
		loadmap("campaign/Battle City/maps/defeat.map")
		pushcmd( function() damage(10, "mine1") end, 2 )
		pushcmd( function() damage(10, "mine6") end, 2.2 )
		pushcmd( function() damage(10, "mine8") end, 2.4 )
		pushcmd( function() damage(10, "mine2") end, 2.6 )
		pushcmd( function() damage(10, "mine7") end, 2.8 )
		pushcmd( function() damage(10, "mine3") end, 3.0)
		pushcmd( function() damage(10, "mine9") end, 3.1 )
		pushcmd( function() damage(10, "mine4") end, 3.2 )
		pushcmd( function() damage(10, "mine5") end, 3.3 )
		pushcmd( function() damage(10, "mine10") end, 3.4 )
	end, 2 )
end

function user.OnDestroyBase()
	actor("user_object", 448, 832, { texture="user/base1", health=0, max_health=0 })
	user.Defeat()
end

function user.Win()
	message("Победа!");
	pushcmd( function()	freeze(true) end, 0.5 )
	pushcmd( user.Next, 2 )
end

function user.OnDieEnemy1()
	user.enemies = user.enemies - 1
	if user.enemies < 3 then
		print("test")
		kill("enemy1")
	end
	if 0 == user.enemies then user.Win() end
end

function user.OnDieEnemy2()
	user.enemies = user.enemies - 1
	if user.enemies < 3 then
		print("test")
		kill("enemy2")
	end
	if 0 == user.enemies then user.Win() end
end

function user.OnDieEnemy3()
	user.enemies = user.enemies - 1
	if user.enemies < 3 then
		print("test")
		kill("enemy3")
	end
	if 0 == user.enemies then user.Win() end
end

function user.OnDiePlayer()
	user.life = user.life - 1
	if user.life > 0 then
		message("У вас осталось " .. user.life .. " жизней")
	else
		user.Defeat()
	end	
end

-------------------------------------------------------------------------------

loadtheme("campaign/Battle City/textures.lua")

conf.sv_nightmode = false;

user.stage   = 0;
user.life    = 5;

user.Next()



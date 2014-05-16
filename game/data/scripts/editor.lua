local fmod = math.fmod
local pi = math.pi

local function rotate(n)
	return function(o)
		local dir = o.dir + 3*pi/n
		dir = dir - fmod(dir, 2*pi/n)
		o.dir = fmod(dir, 2*pi)	
	end
end

local function corner0to4(o)
	o.corner = fmod(o.corner + 1, 5)
end

editor_actions = {
	respawn_point  = rotate(8),
	spotlight      = rotate(16),
	wall_brick     = corner0to4,
	wall_concrete  = corner0to4,
	turret_rocket  = rotate(16),
	turret_cannon  = rotate(16),
	turret_minigun = rotate(4),
	turret_gauss   = rotate(4),
}


--- vehicle classes ---
-- written by Insert --


--===========--
--= default =--
--===========--

classes.default =
{
  -- game properties --

  display     = "Стандарт",
  health      = 400,
  percussion  = 1,
  fragility   = 1,


  -- physical properties --

  width     = 37,
  length    = 37.5,

  mass      = 1.0,
  inertia   = 700.0,

  dry_fric  = {450, 5000, 28},   -- dry friction: x,y,angular
  vis_fric  = {2.0,  2.5,  0},   -- viscous friction: x,y,angular

  power     = {900, 40.0},        -- engine power: linear,angular
  max_speed = {200,  3.4},        -- max speed: linear, angular
}


--=========--
--= heavy =--
--=========--

classes.heavy =
{
  -- game properties --

  display     = "Тяжелый",
  health      = 600,
  percussion  = 1.5,
  fragility   = 0.8,


  -- physical properties --

  width     = 40,
  length    = 40,

  mass      = 2.0,
  inertia   = 1000.0,

  dry_fric  = {450, 5000, 27},   -- dry friction: x,y,angular
  vis_fric  = {2.0,  2.5,  9},   -- viscous friction: x,y,angular

  power     = {1800, 48},     -- engine power: linear,angular
  max_speed = {180, 3.5},     -- max speed: linear, angular
}


-- end of file

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

  bounds = {
    {  19.0,  18.5 },  -- 1      3-----4
    { -18.5,  18.5 },  -- 2      |  .  |___ x
    { -18.5, -18.5 },  -- 3      |  '  |
    {  19.0, -18.5 },  -- 4      2-----1
  },

  mass      = 1.0,
  inertia   = 700.0,

  dry_fric  = {450, 5000, 30},   -- dry friction: x,y,angular
  vis_fric  = {2.0,  2.5,  0},   -- viscous friction: x,y,angular

  power     = {1000, 50},        -- engine power: linear,angular
  max_speed = {200, 3.5},        -- max speed: linear, angular
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

  bounds = {
    {  20.0,  20.0 },  -- 1      3-----4
    { -20.0,  20.0 },  -- 2      |  .  |___ x
    { -20.0, -20.0 },  -- 3      |  '  |
    {  20.0, -20.0 },  -- 4      2-----1
  },

  mass      = 2.0,
  inertia   = 1000.0,

  dry_fric  = {450, 5000, 30},   -- dry friction: x,y,angular
  vis_fric  = {2.0,  2.5, 10},   -- viscous friction: x,y,angular

  power     = {1800, 50},     -- engine power: linear,angular
  max_speed = {180, 3.5}         -- max speed: linear, angular
}


-- end of file

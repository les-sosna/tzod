-- Textures for standard theme --
---- Copyright ® 2007 Insert ----

--  default values are:
--   left     = 0
--   top      = 0
--   right    = [width of image]
--   bottom   = [height of image]
--   xpivot   = [width of image]/2
--   ypivot   = [height of image]/2
--   xframes  = 1,
--   yframes  = 1,
--   xscale   = 1,
--   yscale   = 1,


return {
--------------------------------------
{
   file="textures/background/back01.tga",
   content={
     background={xpivot=0, ypivot=0},
   }
},
{
   file="textures/background/scoretbl.tga",
   content={
     scoretbl={xpivot=0, ypivot=0},
   }
},
{
   file="textures/editor/grid.tga",
   content={
     grid={xpivot=0, ypivot=0},
   }
},
{
   file="textures/editor/labels.tga",
   content={
     editor_respawn={left=0, top=0, right=32, bottom=32},
     editor_item={left=64, top=0, right=128, bottom=64},
   }
},
{
   file="textures/editor/trigger.tga",
   content={
     editor_trigger={},
   }
},
{
   file="textures/walls/walls.tga",
   content={
     brick_wall={left=0, top=0, right=128, bottom=64, xframes=4, yframes=2},
     brick_lt={left=128, top=0, right=160, bottom=128, yframes=4},
     brick_rt={left=160, top=0, right=192, bottom=128, yframes=4},
     brick_rb={left=192, top=0, right=224, bottom=128, yframes=4},
     brick_lb={left=224, top=0, right=256, bottom=128, yframes=4},
     concrete_wall={left=0, top=64, right=32, bottom=96},
     concrete_wall1={left=32, top=64, right=64, bottom=96},
     concrete_wall2={left=64, top=64, right=96, bottom=96},
     concrete_wall3={left=96, top=64, right=128, bottom=96},
     concrete_lt={left=0,  top=96, right=32,  bottom=128},
     concrete_rt={left=32, top=96, right=64,  bottom=128},
     concrete_rb={left=64, top=96, right=96,  bottom=128},
     concrete_lb={left=96, top=96, right=128, bottom=128},
   }
},
{
   file="textures/landscape/wood.tga",
   content={
     wood={left=16, top=16, right=112, bottom=112, xframes=3, yframes=3},
   }
},
{
   file="textures/landscape/water.tga",
   content={
     water={left=0, top=0, right=96, bottom=96, xframes=3, yframes=3},
   }
},
{
   file="textures/turrets/turrets.tga",
   content={
     -- weapons --
     turret_cannon={left=0, top=384, right=64, bottom=448},
     turret_rocket={left=64, top=384, right=128, bottom=448},
     turret_gauss={left=128, top=384, right=192, bottom=448},
     turret_mg={left=192, top=384, right=256, bottom=448},
     -- platforms --
     turret_platform={left=320, top=320, right=384, bottom=384},
     turret_mg_wake={left=0, top=0, right=256, bottom=384, xframes=4, yframes=6},
     turret_gauss_wake={left=256, top=0, right=512, bottom=256, xframes=4, yframes=4},
   }
},
{
   file="textures/particles/particles.tga",
   content={
     particle_brick={left=0, top=0, right=128, bottom=128, xframes=8, yframes=8},
     particle_smoke={left=128, top=0, right=256, bottom=128, xframes=4, yframes=4},
     particle_yellow={left=32, top=192, right=33, bottom=224, xframes=1, yframes=32},
     particle_fire={left=0, top=184, right=64, bottom=192, xframes=16, yframes=2},
     particle_green={left=0, top=176, right=64, bottom=184, xframes=16, yframes=2},
     particle_fire2={left=0, top=240, right=128, bottom=256, xframes=16, yframes=2},
     particle_white={left=0, top=192, right=64, bottom=200, xframes=16, yframes=2},
     particle_gauss1={left=96, top=183, right=112, bottom=190},
     particle_gauss2={left=96, top=195, right=112, bottom=202},
     particle_gausshit={left=72, top=184, right=84, bottom=192},
     particle_gaussfire={left=56, top=208, right=120, bottom=240, xpivot=28},
     particle_1={left=0, top=128, right=160, bottom=144, xframes=10, yframes=1},
     particle_2={left=0, top=144, right=80, bottom=160, xframes=5, yframes=1},
     particle_3={left=0, top=160, right=80, bottom=168, xframes=10, yframes=1},
     particle_trace={left=96, top=160, right=116, bottom=180, xframes=1, yframes=4},
     particle_trace2={left=116, top=160, right=136, bottom=180, xframes=1, yframes=4},
     particle_fire3={left=192, top=128, right=256, bottom=192, xframes=1, yframes=4, xpivot=6, ypivot=8},
     particle_fire4={left=128, top=192, right=256, bottom=256, xframes=2, yframes=2, xpivot=6, ypivot=16},
     particle_plazma_fire={left=0, top=192, right=48, bottom=240, xpivot=24, ypivot=24},
   }
},
{
   file="textures/indicators/indicators.tga",
   content={
     indicator_health={left=0, top=6, right=48, bottom=11, ypivot=0},
     indicator_fuel={left=0, top=0, right=48, bottom=6, ypivot=0},
     indicator_ammo={left=0, top=12, right=48, bottom=17, ypivot=0},
     indicator_crosshair1={left=48, top=16, right=64, bottom=32},
     indicator_crosshair2={left=49, top=1, right=63, bottom=15},
   }
},
{
   file="textures/indicators/damage.tga",
   content={
     indicator_damage={xframes=2},
   }
},
{
   file="textures/projectiles/projectiles.tga",
   content={
     projectile_ac={left=0, top=0, right=4, bottom=4, xpivot=2, ypivot=2},
     projectile_cannon={left=16, top=8, right=25, bottom=13, xpivot=5, ypivot=3},
     projectile_rocket={left=8, top=0, right=24, bottom=8, xpivot=10, ypivot=4},
     projectile_disk={left=0, top=8, right=16, bottom=24, xpivot=8, ypivot=8},
     projectile_bullet={left=18, top=19, right=29, bottom=21},
     projectile_plazma={left=0, top=32, right=64, bottom=48, xpivot=48, ypivot=8},
   }
},
{
   file="textures/projectiles/bfg1.tga",
   content={
     projectile_bfg={xframes=2, yframes=2, xpivot=64, ypivot=64},
   }
},
{
   file="textures/projectiles/fire.tga",
   content={
     projectile_fire={xframes=2, yframes=2},
   }
},
{
   file="textures/items/powerups.tga",
   content={
     pu_health={left=0, top=192, right=64, bottom=256, xframes=4, yframes=4},
     pu_shock={left=0, top=128, right=128, bottom=192, xframes=8, yframes=4},
     pu_inv={left=128, top=0, right=224, bottom=96, xframes=4, yframes=4},
     pu_booster={left=0, top=0, right=128, bottom=128, xframes=4, yframes=4},
   }
},
{
   file="textures/items/weapons.tga",
   content={
     weap_gauss={left=0, top=0, right=48, bottom=32},
     weap_ac={left=60, top=40, right=104, bottom=80},
     weap_ak47={left=32, top=96, right=64, bottom=128},
     weap_bfg={left=0, top=32, right=48, bottom=64},
     weap_cannon={left=0, top=64, right=48, bottom=96},
     weap_mg1={left=0, top=96, right=32, bottom=112},
     weap_mg2={left=0, top=112, right=32, bottom=128},
     weap_ripper={left=64, top=104, right=104, bottom=128},
     weap_plazma={left=64, top=80, right=112, bottom=104},
     weap_ram={left=64, top=0, right=120, bottom=40},
     weap_zippo={left=128, top=0, right=176, bottom=32},
   }
},
{
   file="textures/items/mine.tga",
   content={
     item_mine={},
   }
},
{
   file="textures/effects/explosions.tga",
   content={
     explosion_big={left=0, top=0, right=768, bottom=384, xframes=6, yframes=3},
     explosion_plazma={left=768, top=256, right=1008, bottom=352, xframes=5, yframes=2},
     explosion_e={left=0, top=384, right=512, bottom=448, xframes=8, yframes=1},
     explosion_o={left=0, top=448, right=512, bottom=512, xframes=8, yframes=1},
     explosion_s={left=768, top=0, right=896, bottom=256, xframes=2, yframes=4},
     explosion_g={left=896, top=0, right=1024, bottom=256, xframes=2, yframes=4},
     minigun_fire={left=768, top=0, right=896, bottom=128, xframes=2, yframes=2, xscale=0.5, yscale=0.6},
   }
},
{
   file="textures/effects/shield.tga",
   content={
     shield={right=224, bottom=224, xframes=4, yframes=4},
   }
},
{
   file="textures/effects/booster.tga",
   content={
     booster={xscale=1.5, yscale=1.5},
   }
},
{
   file="textures/effects/lightning.tga",
   content={
     lightning={},
   }
},
{
   file="textures/effects/shine.tga",
   content={
     shine={},
   }
},
{
   file="textures/effects/decals.tga",
   content={
     cat_track={left=0, top=0, right=8, bottom=8},
     bigblast={left=64, top=0, right=128, bottom=64, xscale=2, yscale=2},
     smallblast={left=64, top=0, right=128, bottom=64},
   }
},
{
   file="textures/objects/light.tga",
   content={
     spotlight={},
   }
},
{
   file="textures/objects/crate.tga",
   content={
     crate01={},
   }
},


--- GUI ---
{
   file="textures/gui/fonts.tga",
   content={
     font_small={left=100, right=212, top=0, bottom=140, xframes=16, yframes=14, xpivot=0, ypivot=0},
     font_default={left=212, top=0, right=512, bottom=240, xframes=20, yframes=12, xpivot=0, ypivot=0},
     font_digits_red={left=0, top=-64, right=95, bottom=128, xframes=5, yframes=6, xpivot=0, ypivot=0},
     font_digits_green={left=0, top=64, right=95, bottom=256, xframes=5, yframes=6, xpivot=0, ypivot=0},
   }
},
{
   file="textures/gui/controls.tga",
   content={
     ["ui/window"]={left=50, top=146, right=94, bottom=190, xpivot=0, ypivot=0},
     ["ui/scroll_back_vert"]={left=66, top=146, right=78, bottom=190, xframes=2, yframes=1, xpivot=0, ypivot=0},
     ["ui/scroll_back_hor"]={left=50, top=162, right=94, bottom=174, xpivot=0, ypivot=0},
     ["ui/selection"]={left=2, top=226, right=6, bottom=230, xpivot=0, ypivot=0},
     ["ui/button"]={left=0, right=96, top=0, bottom=96, xframes=1, yframes=4, xpivot=0, ypivot=0},
     ["ui/checkbox"]={left=0, right=96, top=96, bottom=144, xframes=4, yframes=2, xpivot=0, ypivot=0},
     ["ui/scroll_up"]={left=0, right=48, top=144, bottom=156, xframes=4, yframes=1, xpivot=0, ypivot=0},
     ["ui/scroll_vert"]={left=0, right=48, top=156, bottom=180, xframes=4, yframes=1, xpivot=0, ypivot=0},
     ["ui/scroll_down"]={left=0, right=48, top=180, bottom=192, xframes=4, yframes=1, xpivot=0, ypivot=0},
     ["ui/scroll_left"]={left=96, right=108, top=144, bottom=192, xframes=1, yframes=4, xpivot=0, ypivot=0},
     ["ui/scroll_hor"]={left=108, right=132, top=144, bottom=192, xframes=1, yframes=4, xpivot=0, ypivot=0},
     ["ui/scroll_right"]={left=132, right=144, top=144, bottom=192, xframes=1, yframes=4, xpivot=0, ypivot=0},
     ["ui/list"]={left=66, right=70, top=194, bottom=198, xframes=1, yframes=1, xpivot=0, ypivot=0},
     ["ui/combo_list"]={left=66, right=70, top=194, bottom=198, xframes=1, yframes=1, xpivot=0, ypivot=0},
     ["ui/edit"]={left=66, right=70, top=202, bottom=206, xframes=2, yframes=1, xpivot=0, ypivot=0},
     ["ui/console"]={left=66, right=70, top=202, bottom=206, xframes=1, yframes=1, xpivot=0, ypivot=0},
     ["ui/combo"]={left=66, right=70, top=194, bottom=198, xframes=4, yframes=1, xpivot=0, ypivot=0},
     ["ui/listsel"]={left=2, right=30, top=194, bottom=206, xpivot=0, ypivot=0},
     ["ui/editcursor"]={left=34, right=40, top=193, bottom=207, xpivot=0, ypivot=0},
     ["ui/editsel"]={left=42, right=46, top=202, bottom=206, xpivot=0, ypivot=0},
     ["ui/bar"]={left=0, right=1, top=226, bottom=228, xpivot=0, ypivot=0},
   }
},
{
   file="textures/gui/splash.tga",
   content={
     gui_splash={xpivot=0, ypivot=0},
   }
},
{
   file="textures/gui/logos.tga",
   content={
     gui_logos={xpivot=0, ypivot=0},
   }
},
{
   file="textures/gui/cursor.tga",
   content={
     cursor={right=128, bottom=64, xframes=4, yframes=2, xpivot=1, ypivot=1},
   }
},
--------------------------------------

}
-- end of file --

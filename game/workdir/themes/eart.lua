-- Eart theme by Paradox --

return {
------------------------------------

{
  file="themes/eart/bg.tga",
  wrap=true,
  content={
    background={},
  }
},
{
   file="themes/eart/walls.tga",
   wrap=true,
   content={
     brick_wall={left=0, top=0, right=128, bottom=64, xframes=4, yframes=2},
     brick_lt={left=128, top=0, right=160, bottom=128, yframes=4},
     brick_rt={left=160, top=0, right=192, bottom=128, yframes=4},
     brick_rb={left=192, top=0, right=224, bottom=128, yframes=4},
     brick_lb={left=224, top=0, right=256, bottom=128, yframes=4},
     concrete_wall={left=0, top=64, right=128, bottom=96, xframes=4, yframes=1},
     concrete_lt={left=0,  top=96, right=32,  bottom=128},
     concrete_rt={left=32, top=96, right=64,  bottom=128},
     concrete_rb={left=64, top=96, right=96,  bottom=128},
     concrete_lb={left=96, top=96, right=128, bottom=128},
   }
},
{
  file="themes/grass/water.tga",
  content={
     water={left=0, top=0, right=96, bottom=96, xframes=3, yframes=3},
  }
},
{
  file="themes/eart/mine.tga",
  content={
    item_mine={},
  }
},
{
  file="themes/eart/decals.tga",
  wrap=true,
  content={
    cat_track={left=0, top=0, right=8, bottom=8, xpivot=4, ypivot=4},
  }
}
------------------------------------
} -- end of file

-- Metal theme by Paradox --

return {
------------------------------------
{
  file="themes/metal/bg.tga",
  wrap=true,
  content={
    background={},
  }
},
{
   file="themes/metal/walls.tga",
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
  file="themes/metal/wood.tga",
  content={
    wood={left=16, top=16, right=112, bottom=112, xframes=3, yframes=3},
  }
},
{
  file="themes/metal/water.tga",
  content={
     water={left=0, top=0, right=96, bottom=96, xframes=3, yframes=3},
  }
},
------------------------------------
} -- end of file

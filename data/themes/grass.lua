-- Grass theme by Insert --

return {
------------------------------------

{
  file="themes/grass/bg.tga",
  wrap=true,
  content={
    background={},
  }
},
{
  file="themes/grass/walls.tga",
  wrap=true,
  content={
    brick_wall={left=0, top=0, right=128, bottom=64, xframes=4, yframes=2},
    concrete_wall={left=0, top=80, right=128, bottom=112, xframes=4, yframes=1},
  }
},
{
  file="themes/grass/water.tga",
  content={
     water={left=0, top=0, right=96, bottom=96, xframes=3, yframes=3},
  }
},
{
  file="themes/grass/mine.tga",
  content={
    item_mine={},
  }
},
{
  file="themes/grass/decals.tga",
  wrap=true,
  content={
    cat_track={left=0, top=0, right=8, bottom=8, xpivot=4, ypivot=4},
  }
}
------------------------------------
} -- end of file

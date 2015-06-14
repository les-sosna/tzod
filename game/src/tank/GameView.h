#include <math/MyMath.h>
#include <stddef.h>

class DefaultCamera;
class DrawingContext;
class World;
class WorldView;

RectRB GetCameraViewport(int screenW, int screenH, size_t camCount, size_t camIndex);
void RenderGame(DrawingContext &dc, const World &world, const WorldView &worldView,
                int width, int height, const DefaultCamera &defaultCamera);

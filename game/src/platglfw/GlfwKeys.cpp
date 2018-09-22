#include "inc/platglfw/GlfwKeys.h"
#include <plat/Keys.h>
#include <GLFW/glfw3.h>

Plat::Key MapGlfwKeyCode(int platformKey)
{
	switch(platformKey)
	{
#define GEN_KEY_ENTRY(platformKey, uiKey) case platformKey: return uiKey;
#include "GlfwKeys.gen"
#undef GEN_KEY_ENTRY
		default:
			break;
	}
	return Plat::Key::Unknown;
}

int UnmapGlfwKeyCode(Plat::Key key)
{
	switch(key)
	{
#define GEN_KEY_ENTRY(platformKey, uiKey) case uiKey: return platformKey;
#include "GlfwKeys.gen"
#undef GEN_KEY_ENTRY
		default:
			break;
	}
	return -1;
}

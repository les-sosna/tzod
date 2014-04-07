// Application.cpp

#include "globals.h"
#include "constants.h"
#include "Application.h"
#include "config/Config.h"

#include <GLFW/glfw3.h>

AppBase::AppBase()
{
    if( !glfwInit() )
        throw std::runtime_error("Failed to initialize OpenGL");
}

AppBase::~AppBase()
{
    glfwTerminate();
}

int AppBase::Run()
{
    g_appWindow = glfwCreateWindow(g_conf.r_width.GetInt(),
                                   g_conf.r_height.GetInt(),
                                   TXT_VERSION,
                                   /*g_conf.r_fullscreen.Get() ? glfwGetPrimaryMonitor() :*/ nullptr,
                                   nullptr);
    glfwMakeContextCurrent(g_appWindow);
	if( Pre() )
	{
		for(;;)
		{
            glfwSwapBuffers(g_appWindow);
            glfwPollEvents();
            
            if (glfwWindowShouldClose(g_appWindow))
                break;
            Idle();
        }
	}
    glfwDestroyWindow(g_appWindow);
    g_appWindow = nullptr;
	Post();
	return -1;
}

// end of file

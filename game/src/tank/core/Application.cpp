// Application.cpp

#include "stdafx.h"
#include "Application.h"


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
	if( Pre() )
	{
        g_appWindow = glfwCreateWindow(800, 600, "hello tzod", nullptr, nullptr);
        glfwMakeContextCurrent(g_appWindow);
		for(;;)
		{
            glfwSwapBuffers(g_appWindow);
            glfwPollEvents();
            
            if (glfwWindowShouldClose(g_appWindow))
                break;
            Idle();
        }
        glfwDestroyWindow(g_appWindow);
        g_appWindow = nullptr;
	}
	Post();
	return -1;
}

// end of file

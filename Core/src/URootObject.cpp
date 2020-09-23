#include <URootObject.h>

#include <Application.h>

using namespace dx12demo::core;

static Application* gs_Application;

void URootObject::SetApp(Application* app)
{
	gs_Application = app;
}

Application& URootObject::GetApp() const
{
	return URootObject::GetApplication();
}

Application& URootObject::GetApplication()
{
	return *gs_Application;
}
#include "TriangleApplication.h"

TriangleApplication::TriangleApplication()
{
}

TriangleApplication::~TriangleApplication()
{
}

void TriangleApplication::Run()
{
	MainLoop();
}

void TriangleApplication::MainLoop()
{
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}
}

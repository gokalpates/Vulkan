#include <iostream>

#include "TriangleApplication.h"

int main()
{
	try
	{
		TriangleApplication app;
		app.Run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}
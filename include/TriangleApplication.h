#pragma once

#include "Application.h"

class TriangleApplication : public Application
{
public:
	TriangleApplication();
	~TriangleApplication();

	void Run();
private:
	void MainLoop();
	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	void DrawFrames();
};


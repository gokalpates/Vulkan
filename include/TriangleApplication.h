#pragma once

#include "Application.h"

class TriangleApplication : public Application
{
public:
	TriangleApplication();
	~TriangleApplication();

	void Run();
private:
	void Initialise();
	void Destroy();

	void MainLoop();
	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	void DrawFrames();

	void CreateCommandBuffers();
	void CreateSyncObjects();
	void DestroySyncObjects();

	static const int maxFramesInFlight;

	int currentFrame;

	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
};


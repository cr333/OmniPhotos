#pragma once


class Application
{
public:
	Application() = default;
	virtual ~Application() = default;

	bool shouldShutdown = false;

	// Every app has to init itself.
	virtual int init() = 0;

	// Main execution loop. The app can do whatever she wants.
	virtual void run() = 0;
};

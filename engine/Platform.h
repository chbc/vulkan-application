#pragma once
class Platform
{
public:
	bool init();
	void processInput(bool& stillRunning);
	void processFrameEnd();
	void release();
};

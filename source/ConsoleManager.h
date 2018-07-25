//  ***************************************************************
//  ConsoleManager - Creation date: 07/23/2018
//  -------------------------------------------------------------
//  Robinson Technologies Copyright (C) 2018 - All Rights Reserved
//
//  ***************************************************************
//  Programmer(s):  Seth A. Robinson (seth@rtsoft.com)
//  ***************************************************************

#ifndef ConsoleManager_h__
#define ConsoleManager_h__

#include "Manager/OpenCVManager.h"

enum eAppMode
{
	APP_MODE_RUNNING,
	APP_MODE_WAITING_FOR_EMULATOR
};

class ConsoleManager
{
public:
	ConsoleManager();
	virtual ~ConsoleManager();
	bool Init();
	void KillEmulator();
	void DrawLoadingScreen();
	void RunGame();
	void RemoveMessage(int delayMS);
	void OnLoadedRomFromQR(QRCodeInfo *pQRInfo);
	void OnNoCardInserted();
	void Draw();
	void Update();

	bool ShouldDrawToScreen() { return m_mode == APP_MODE_RUNNING; }

protected:
	

private:

	vector<string> m_roms;
	eAppMode m_mode;
	uint32 m_loadedRomHash;
	unsigned int m_multiPartHash;
	Surface m_surf; //for testing
	string m_status;
	unsigned int m_removeMessageTimer;

};

#endif // ConsoleManager_h__#pragma once

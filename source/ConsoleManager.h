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
	void RunGame();
	void OnLoadedRomFromQR(QRCodeInfo *pQRInfo);
	void OnNoCardInserted();
	void Update();

	bool ShouldDrawToScreen();
	

	bool m_pauseCapture;
	bool m_bAllowRenderingToScreen;

private:

	
	void ScheduleCommand(string command, int delayBeforeActionMS);
	void SetStatus(string status);

	vector<string> m_roms;
	eAppMode m_mode;
	uint32 m_loadedRomHash;
	unsigned int m_multiPartHash;
	SurfaceAnim m_surf; //for testing
	Entity *m_pCamEnt;
	Entity *m_pBG, *m_pCart, *m_pStatusEnt;
	CL_Vec2f m_cartNormalPos;
	
};

#endif // ConsoleManager_h__#pragma once

/*
 *  App.h
 *  Created by Seth Robinson on 3/6/09.
 *  For license info, check the license.txt file that should have come with this.
 *
 */

#pragma once

#include "BaseApp.h"
#include "Manager/OpenCVManager.h"
#include "RomUtils.h"
#include "ConsoleManager.h"


class App: public BaseApp
{
public:
	
	App();
	virtual ~App();
	
	virtual bool Init();
	virtual void Kill();
	virtual void Draw();
	virtual void OnScreenSizeChange();
	virtual void OnEnterBackground();
	virtual void OnEnterForeground();
	virtual bool OnPreInitVideo();
	virtual void Update();
	void OnExitApp(VariantList *pVarList);
	
	
	//we'll wire these to connect to some signals we care about
	void OnAccel(VariantList *pVList);
	void OnArcadeInput(VariantList *pVList);
	OpenCVManager * GetOpenCV() { return &m_openCV; }
	RomUtils * GetRomUtils() { return &m_romUtils; }

	CL_Vec2i m_captureSize;
	ConsoleManager * GetConsoleManager() { return &m_consoleManager; }

	int m_cameraFPS;
	int m_lostFocusFPS;


private:

	
	bool m_bDidPostInit;
	OpenCVManager m_openCV;
	RomUtils m_romUtils;
	unsigned int m_ignoreTimer;
	ConsoleManager m_consoleManager;
	
	int m_cameraBrightness;
	int m_cameraSharpness;
	int m_cameraContrast;
	int m_cameraSaturation;
	
};


App * GetApp();
const char * GetAppName();
const char * GetBundlePrefix();
const char * GetBundleName();

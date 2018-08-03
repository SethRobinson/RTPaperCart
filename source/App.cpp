/*
 *  App.cpp
 *  Created by Seth Robinson on 3/6/09.
 *  For license info, check the license.txt file that should have come with this.
 *
 */ 
#include "PlatformPrecomp.h"
#include "App.h"

#include "Entity/CustomInputComponent.h" //used for the back button (android)
#include "Entity/FocusInputComponent.h" //needed to let the input component see input messages
#include "Entity/ArcadeInputComponent.h" 

MessageManager g_messageManager;
MessageManager * GetMessageManager() {return &g_messageManager;}

FileManager g_fileManager;
FileManager * GetFileManager() {return &g_fileManager;}

#include "Audio/AudioManager.h"
AudioManager g_audioManager; //to disable sound, this is a dummy

AudioManager * GetAudioManager(){return &g_audioManager;}

App *g_pApp = NULL;

BaseApp * GetBaseApp() 
{
	if (!g_pApp)
	{
		g_pApp = new App;
	}
	return g_pApp;
}

App * GetApp() 
{
	assert(g_pApp && "GetBaseApp must be called used first");
	return g_pApp;
}

App::App()
{
	m_captureSize = CL_Vec2i(1000, 1000);
	m_cameraFPS = 15;
	m_lostFocusFPS = 2;


#ifdef WINAPI
	//m_cameraFPS = 120;

#endif

	m_bDidPostInit = false;

	m_cameraBrightness = 0;
	m_cameraSharpness = 0;
	m_cameraContrast = 0;
	m_cameraSaturation = 0;
}

App::~App()
{
}


bool App::Init()
{
	
	if (m_bInitted)	
	{
		return true;
	}
	
	if (!BaseApp::Init()) return false;
	m_ignoreTimer = 0;


	//read command line stuff
	m_romUtils.Init();


	vector<string> parms = GetApp()->GetCommandLineParms();
	for (int i = 0; i < parms.size(); i++)
	{
		//LogMsg("Got command line parm %s", parms[i].c_str());

		if (i < parms.size())
		{
			if (parms[i] == "?" || parms[i] == "/?" || parms[i] == "--h")
			{
			
				LogMsg("Command line options:\n\n");
				LogMsg("-w <capture width>\n");
				LogMsg("-h <capture height>\n");
				LogMsg("-fps <capture fps>\n");
				LogMsg("-backgroundfps <background capture fps>\n");
				LogMsg("-convert <filename to be converted to a QR code.  rtpack and html will be created\n\n");
				exit(0);
				return false;
			}
			//we can safely assume there is a parm after this one
			if (parms[i] == "-w")
			{
				m_captureSize.x = StringToInt(parms[i + 1]);
			}
			if (parms[i] == "-h")
			{
				m_captureSize.y = StringToInt(parms[i + 1]);
			}

			if (parms[i] == "-fps")
			{
				m_cameraFPS = StringToInt(parms[i + 1]);
			}
			if (parms[i] == "-backgroundfps")
			{
				m_lostFocusFPS = StringToInt(parms[i + 1]);
			}

			if (parms[i] == "-contrast")
			{
				m_cameraContrast = StringToInt(parms[i + 1]);
			}

			if (parms[i] == "-sharpness")
			{
				m_cameraSharpness = StringToInt(parms[i + 1]);
			}

			if (parms[i] == "-saturation")
			{
				m_cameraSaturation = StringToInt(parms[i + 1]);
			}
			if (parms[i] == "-brightness")
			{
				m_cameraBrightness = StringToInt(parms[i + 1]);
			}

			if (parms[i] == "-convert")
			{
				LogMsg("Converting file %s to QR code...", parms[i + 1].c_str());
				m_romUtils.WriteRomAsQRCode(parms[i + 1]);
				LogMsg("Finished.  Look for an .html file.");
				exit(0);
				return false;

			}

		}
		else
		{
			LogMsg("Bad parms?");
		}

	}


	if (GetEmulatedPlatformID() == PLATFORM_ID_IOS || GetEmulatedPlatformID() == PLATFORM_ID_WEBOS)
	{
		//SetLockedLandscape( true); //if we don't allow portrait mode for this game
		//SetManualRotationMode(true); //don't use manual, it may be faster (33% on a 3GS) but we want iOS's smooth rotations
	}



	LogMsg("The Save path is %s", GetSavePath().c_str());
	LogMsg("Region string is %s", GetRegionString().c_str());

#ifdef _DEBUG
	LogMsg("Built in debug mode");
#endif
#ifndef C_NO_ZLIB
	//fonts need zlib to decompress.  When porting a new platform I define C_NO_ZLIB and add zlib support later sometimes
	if (!GetFont(FONT_SMALL)->Load("interface/font_trajan.rtfont")) return false;
#endif

	GetBaseApp()->SetFPSVisible(true);
	return true;
}

void App::Kill()
{
	BaseApp::Kill();
}

void App::OnExitApp(VariantList *pVarList)
{
	LogMsg("Exiting the app");
	OSMessage o;
	o.m_type = OSMessage::MESSAGE_FINISH_APP;
	GetBaseApp()->AddOSMessage(o);
}

string RunLinuxShell(string command)
{

	string temp;

	temp = "\r\nRunning " + command + " ...\r\n";

#ifndef WINAPI

	system(command.c_str());
	return temp;

#else
	return "Doesn't work in windows, can't run " + command;

#endif
}
void ForceUpdateOfScreen();

void App::Update()
{
	
	//game can think here.  The baseApp::Update() will run Update() on all entities, if any are added.  The only one
	//we use in this example is one that is watching for the Back (android) or Escape key to quit that we setup earlier.

	BaseApp::Update();

	if (!m_bDidPostInit)
	{
		//stuff I want loaded during the first "Update"
		m_bDidPostInit = true;

		m_consoleManager.Init();
	
		//for android, so the back key (or escape on windows) will quit out of the game
		Entity *pEnt = GetEntityRoot()->AddEntity(new Entity);
		EntityComponent *pComp = pEnt->AddComponent(new CustomInputComponent);
		//tell the component which key has to be hit for it to be activated
		pComp->GetVar("keycode")->Set(uint32(VIRTUAL_KEY_BACK));
		//attach our function so it is called when the back key is hit
		pComp->GetFunction("OnActivated")->sig_function.connect(1, boost::bind(&App::OnExitApp, this, _1));

		//nothing will happen unless we give it input focus
		pEnt->AddComponent(new FocusInputComponent);

		m_openCV.SetCaptureFPS(m_cameraFPS);
		m_openCV.SetCaptureSize(m_captureSize.x, m_captureSize.y);
		
		if (m_cameraBrightness != 0) m_openCV.SetBrightness(m_cameraBrightness);
		if (m_cameraContrast != 0) m_openCV.SetContrast(m_cameraContrast);
		if (m_cameraSaturation != 0) m_openCV.SetSaturation(m_cameraSaturation);
		if (m_cameraSharpness != 0) m_openCV.SetSharpness(m_cameraSharpness);
		m_openCV.InitCamera();
	}

	m_consoleManager.Update();

}

void App::Draw()
{


	if (!m_consoleManager.ShouldDrawToScreen())
	{
		//presumable the game is running because we've shelled out, so we don't want to screw with the screen until we detect the catr
		//has been removed or changed
		return;
	}


	//Use this to prepare for raw GL calls
	PrepareForGL();
#ifdef _DEBUG
	//LogMsg("Doing draw");
#endif
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	CLEAR_GL_ERRORS() //honestly I don't know why I get a 0x0502 GL error when doing the FIRST gl action that requires a context with emscripten only

					  //the base handles actually drawing the GUI stuff over everything else, if applicable, which in this case it isn't.
	BaseApp::Draw();

}


void App::OnScreenSizeChange()
{
	BaseApp::OnScreenSizeChange();
}

void App::OnEnterBackground()
{
	//save your game stuff here, as on some devices (Android <cough>) we never get another notification of quitting.
	LogMsg("Entered background");
	BaseApp::OnEnterBackground();
}

void App::OnEnterForeground()
{
	LogMsg("Entered foreground");
	BaseApp::OnEnterForeground();
}

const char * GetAppName() {return "PaperCart";}

//the stuff below is for android/webos builds.  Your app needs to be named like this.

//note: these are put into vars like this to be compatible with my command-line parsing stuff that grabs the vars

const char * GetBundlePrefix()
{
	const char * bundlePrefix = "com.rtsoft.";
	return bundlePrefix;
}

const char * GetBundleName()
{
	const char * bundleName = "RTPaperCart";
	return bundleName;
}

bool App::OnPreInitVideo()
{
	//only called for desktop systems
	//override in App.* if you want to do something here.  You'd have to
	//extern these vars from main.cpp to change them...

	//SetEmulatedPlatformID(PLATFORM_ID_WINDOWS);
	
	SetPrimaryScreenSize(1920, 1080);
//	SetupScreenInfo(1920, 1080, ORIENTATION_DONT_CARE);


	//g_winVideoScreenY = 768;
	return true; //no error
}

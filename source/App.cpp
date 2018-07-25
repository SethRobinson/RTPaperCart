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
//#include "util/TextScanner.h"

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
	m_captureSize = CL_Vec2i(800, 800);
	m_cameraFPS = 30;

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

#define kFilteringFactor 0.1f
#define C_DELAY_BETWEEN_SHAKES_MS 500

//testing accelerometer readings. To enable the test, search below for "ACCELTEST"
//Note: You'll need to look at the  debug log to see the output. (For android, run PhoneLog.bat from RTPaperCart/android)
void App::OnAccel(VariantList *pVList)
{
	
	if ( int(pVList->m_variant[0].GetFloat()) != MESSAGE_TYPE_GUI_ACCELEROMETER) return;

	CL_Vec3f v = pVList->m_variant[1].GetVector3();

	LogMsg("Accel: %s", PrintVector3(v).c_str());

	v.x = v.x * kFilteringFactor + v.x * (1.0f - kFilteringFactor);
	v.y = v.y * kFilteringFactor + v.y * (1.0f - kFilteringFactor);
	v.z = v.z * kFilteringFactor + v.z * (1.0f - kFilteringFactor);

	// Compute values for the three axes of the acceleromater
	float x = v.x - v.x;
	float y = v.y - v.x;
	float z = v.z - v.x;

	//Compute the intensity of the current acceleration 
	if (sqrt(x * x + y * y + z * z) > 2.0f)
	{
		Entity *pEnt = GetEntityRoot()->GetEntityByName("jumble");
		if (pEnt)
		{
			//GetAudioManager()->Play("audio/click.wav");
            VariantList vList(CL_Vec2f(), pEnt);
			pEnt->GetFunction("OnButtonSelected")->sig_function(&vList);
		}
		LogMsg("Shake!");
	}
}


//test for arcade keys.  To enable this test, search for TRACKBALL/ARCADETEST: below and uncomment the stuff under it.
//Note: You'll need to look at the debug log to see the output.  (For android, run PhoneLog.bat from RTPaperCart/android)

void App::OnArcadeInput(VariantList *pVList)
{

	int vKey = pVList->Get(0).GetUINT32();
	eVirtualKeyInfo keyInfo = (eVirtualKeyInfo) pVList->Get(1).GetUINT32();
	
	string pressed;

	switch (keyInfo)
	{
		case VIRTUAL_KEY_PRESS:
			pressed = "pressed";
			break;

		case VIRTUAL_KEY_RELEASE:
			pressed = "released";
			break;

		default:
			LogMsg("OnArcadeInput> Bad value of %d", keyInfo);
	}
	

	string keyName = "unknown";

	switch (vKey)
	{
		case VIRTUAL_KEY_DIR_LEFT:
			keyName = "Left";
			break;

		case VIRTUAL_KEY_DIR_RIGHT:
			keyName = "Right";
			break;

		case VIRTUAL_KEY_DIR_UP:
			keyName = "Up";
			break;

		case VIRTUAL_KEY_DIR_DOWN:
			keyName = "Down";
			break;

	}
	
	LogMsg("Arcade input: Hit %d (%s) (%s)", vKey, keyName.c_str(), pressed.c_str());
}


void AppInput(VariantList *pVList)
{

	//0 = message type, 1 = parent coordinate offset, 2 is fingerID
	eMessageType msgType = eMessageType( int(pVList->Get(0).GetFloat()));
	CL_Vec2f pt = pVList->Get(1).GetVector2();
	//pt += GetAlignmentOffset(*m_pSize2d, eAlignment(*m_pAlignment));

	uint32 fingerID = 0;
	if ( msgType != MESSAGE_TYPE_GUI_CHAR && pVList->Get(2).GetType() == Variant::TYPE_UINT32)
	{
		fingerID = pVList->Get(2).GetUINT32();
	}

	CL_Vec2f vLastTouchPt = GetBaseApp()->GetTouch(fingerID)->GetLastPos();

	switch (msgType)
	{
	case MESSAGE_TYPE_GUI_CLICK_START:
		LogMsg("Touch start: X: %.2f YL %.2f (Finger %d)", pt.x, pt.y, fingerID);
		break;
	case MESSAGE_TYPE_GUI_CLICK_MOVE:
		LogMsg("Touch move: X: %.2f YL %.2f (Finger %d)", pt.x, pt.y, fingerID);
		break;
	case MESSAGE_TYPE_GUI_CLICK_END:
		LogMsg("Touch end: X: %.2f YL %.2f (Finger %d)", pt.x, pt.y, fingerID);
		break;
	}	
}

string RunLinuxShell(string command)
{

	string temp;

	temp = "\r\nRunning " + command + " ...\r\n";

#ifndef WINAPI

	system(command.c_str());
// 	
// 	FILE *fpipe;
// 
// 	char line[256];
// 
// 	if (!(fpipe = (FILE*)popen(command.c_str(), "r")))
// 	{  // If fpipe is NULL
// 		return ("Problems with pipe, can't run shell");
// 	}
// 
// 	while (fgets(line, sizeof(line), fpipe))
// 	{
// 		temp += string(line) + "\r";
// 	}
// 	pclose(fpipe);
// 

	return temp;

#else
	//GetApp()->m_gameServer.InitiateShutdown( (m_shutdownAfterCompile*1000*60)+1000);
	//m_shutdownAfterCompile = -1;
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
		
		//for android, so the back key (or escape on windows) will quit out of the game
		Entity *pEnt = GetEntityRoot()->AddEntity(new Entity);
		EntityComponent *pComp = pEnt->AddComponent(new CustomInputComponent);
		//tell the component which key has to be hit for it to be activated
		pComp->GetVar("keycode")->Set(uint32(VIRTUAL_KEY_BACK));
		//attach our function so it is called when the back key is hit
		pComp->GetFunction("OnActivated")->sig_function.connect(1, boost::bind(&App::OnExitApp, this, _1));

		//nothing will happen unless we give it input focus
		pEnt->AddComponent(new FocusInputComponent);

		//ACCELTEST:  To test the accelerometer uncomment below: (will print values to the debug output)
		//SetAccelerometerUpdateHz(25); //default is 0, disabled
		//GetBaseApp()->m_sig_accel.connect(1, boost::bind(&App::OnAccel, this, _1));

		//TRACKBALL/ARCADETEST: Uncomment below to see log messages on trackball/key movement input
		pComp = pEnt->AddComponent(new ArcadeInputComponent);
		GetBaseApp()->m_sig_arcade_input.connect(1, boost::bind(&App::OnArcadeInput, this, _1));
	
		//these arrow keys will be triggered by the keyboard, if applicable
		AddKeyBinding(pComp, "Left", VIRTUAL_KEY_DIR_LEFT, VIRTUAL_KEY_DIR_LEFT);
		AddKeyBinding(pComp, "Right", VIRTUAL_KEY_DIR_RIGHT, VIRTUAL_KEY_DIR_RIGHT);
		AddKeyBinding(pComp, "Up", VIRTUAL_KEY_DIR_UP, VIRTUAL_KEY_DIR_UP);
		AddKeyBinding(pComp, "Down", VIRTUAL_KEY_DIR_DOWN, VIRTUAL_KEY_DIR_DOWN);
		AddKeyBinding(pComp, "Fire", VIRTUAL_KEY_CONTROL, VIRTUAL_KEY_GAME_FIRE);

		//INPUT TEST - wire up input to some functions to manually handle.  AppInput will use LogMsg to
		//send them to the log.  (Each device has a way to view a debug log in real-time)
		GetBaseApp()->m_sig_input.connect(&AppInput);

		/*
		//file handling test, if TextScanner.h is included at the top..

		TextScanner t;
		t.m_lines.push_back("Testing 123");
		t.m_lines.push_back("Heck yeah!");
		t.m_lines.push_back("Whoopsopsop!");

		LogMsg("Saving file...");
		t.SaveFile("temp.txt");


		TextScanner b;
		b.LoadFile("temp.txt");
		b.DumpToLog();
		*/


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

#ifndef WINAPI
	if (!m_consoleManager.ShouldDrawToScreen())
	{

		//on raspberry, we do nothing.  On windows, we'll do something slightly different as we don't really shell out to retropie
		return;
	}

#endif
	//Use this to prepare for raw GL calls
	PrepareForGL();
#ifdef _DEBUG
	//LogMsg("Doing draw");
#endif
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	CLEAR_GL_ERRORS() //honestly I don't know why I get a 0x0502 GL error when doing the FIRST gl action that requires a context with emscripten only

	//draw our game stuff
	//DrawFilledRect(10.0f,10.0f,GetScreenSizeXf()/3,GetScreenSizeYf()/3, MAKE_RGBA(255,255,0,255));
	//DrawFilledRect(0,0,64,64, MAKE_RGBA(0,255,0,100));

	//after our 2d rect call above, we need to prepare for raw GL again. (it keeps it in ortho mode if we don't for speed)
		PrepareForGL();
	//RenderSpinningTriangle();
//	RenderGLTriangle();
	//let's blit a bmp, but first load it if needed

	m_consoleManager.Draw();

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

const char * GetAppName() {return "Paper VCS";}

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
#if defined (_DEBUG) && defined(WINAPI)
	SetupScreenInfo(1024, 768, ORIENTATION_DONT_CARE);
#endif


	//g_winVideoScreenY = 768;
	return true; //no error
}

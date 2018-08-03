#include "PlatformPrecomp.h"
#include "ConsoleManager.h"
#include "Manager/QRGenerateManager.h"
#include "App.h"
#include "Entity/EntityUtils.h"

string RunLinuxShell(string command);


ConsoleManager::ConsoleManager()
{
	m_loadedRomHash = 0;
	m_mode = APP_MODE_RUNNING;
	m_multiPartHash = 0;
	m_pauseCapture = false;
	m_bAllowRenderingToScreen = true;

}

ConsoleManager::~ConsoleManager()
{
}


	
//just a thing to let me schedule calling things.  This should all really be in a component, whatever
void HandleInstruction(VariantList *pVList)
{
	string command = pVList->Get(0).GetString();

	LogMsg("Got static call: %s", command.c_str());
	if (command == "stop_capture")
	{
		GetApp()->GetConsoleManager()->m_pauseCapture = true;
	}
	if (command == "start_capture")
	{
		GetApp()->GetConsoleManager()->m_pauseCapture = false;
	}

	if (command == "run")
	{
		//m_mode = APP_MODE_RUNNING;
		string ret = RunLinuxShell("nohup /opt/retropie/supplementary/runcommand/runcommand.sh 0 _SYS_ atari2600 /home/pi/proton/RTPaperCart/bin/decoded_rom.a26 &");
		LogMsg(ret.c_str());
	}

	if (command == "stop_render")
	{
		GetApp()->GetConsoleManager()->m_bAllowRenderingToScreen = false;
	}
	if (command == "start_render")
	{
		GetApp()->GetConsoleManager()->m_bAllowRenderingToScreen = true;
	}
	

}

void ConsoleManager::ScheduleCommand(string command, int delayBeforeActionMS)
{
	VariantList vList;
	vList.Get(0).Set(command);
	GetMessageManager()->CallStaticFunction(HandleInstruction, delayBeforeActionMS, &vList);
}

bool ConsoleManager::Init()
{

	//to test stuff
	/*
	LogMsg("testing");
	QRGenerateManager qr;
	unsigned int leng;
	char *pCrap = (char*)LoadFileIntoMemoryBasic("alice.txt", &leng);
	pCrap[2900] = 0;
	qr.MakeQRWithText(pCrap, "hi_text.html");
	*/

	//Setup GUI, we're using Proton's GUI stuff

	//setup background image
	m_pBG = CreateOverlayEntity(GetApp()->GetEntityRoot(), "BG", "interface/bg.rttex", 0, 0);

	//Add cart, and the camera image as a child of it

	m_cartNormalPos = CL_Vec2f(1450, 511);
	m_pCart = CreateOverlayEntity(m_pBG, "Cart", "interface/cart.rttex", m_cartNormalPos.x, m_cartNormalPos.y);
	m_pCamEnt = CreateOverlayEntity(m_pCart, "Camera", "", 28, 163);

	OverlayRenderComponent *pRender = (OverlayRenderComponent*)m_pCamEnt->GetComponentByName("OverlayRender");
	pRender->SetSurface(&m_surf, false);
	AddFocusIfNeeded(m_pBG);
	SetSize2DEntity(m_pCamEnt, CL_Vec2f(GetApp()->m_captureSize));
	EntitySetScaleBySize(m_pCamEnt, CL_Vec2f(381, 351), false);
	CL_Vec2f vOriginalCamScale = GetScale2DEntity(m_pCamEnt);

	//disable image capture for a bit so the anim is smoother
	ScheduleCommand("stop_capture", 200);
	ScheduleCommand("start_capture", 1200);

	//anim effect
	ZoomFromPositionEntity(m_pCart, CL_Vec2f(m_cartNormalPos.x, -300), 1200, INTERPOLATE_EASE_TO, 200);

	
	
	m_pStatusEnt = CreateTextLabelEntity(m_pBG, "Status", GetScreenSizeXf() / 2, 200, "");
	SetStatus("Insert cart");
	SetScale2DEntity(m_pStatusEnt, CL_Vec2f(3, 3));
	GetApp()->GetFont(FONT_SMALL)->SetSmoothing(false);
	SetAlignmentEntity(m_pStatusEnt, ALIGNMENT_CENTER);

	return true;
}

void ConsoleManager::SetStatus(string status)
{
	
	SetTextEntity(m_pStatusEnt, "`b" + status + "``");
}

void ConsoleManager::KillEmulator()
{
	LogMsg("Killing emu...");
	RunLinuxShell("pkill retroarch");
}

void ConsoleManager::RunGame()
{

	if (m_mode == APP_MODE_WAITING_FOR_EMULATOR)
	{
		//we want to run a game but we already are?!
		LogMsg("Detected card while emu is running");
		KillEmulator();
	}

	SetStatus("`2Running cart``");
	m_pauseCapture = true;
	int loadingAnimMS = 3700;
	 
	ZoomToPositionEntity(m_pCart, CL_Vec2f(650,250), loadingAnimMS, INTERPOLATE_SMOOTHSTEP, 300);
	ScheduleCommand("stop_capture", 0);
	ScheduleCommand("start_capture", loadingAnimMS+300);
	ScheduleCommand("stop_render", loadingAnimMS);
	ScheduleCommand("run", 0);
	GetApp()->GetOpenCV()->SetCaptureFPS(GetApp()->m_lostFocusFPS);
	m_mode = APP_MODE_WAITING_FOR_EMULATOR;

}

void ConsoleManager::OnLoadedRomFromQR(QRCodeInfo *pQRInfo)
{
	uint32 romHash = HashString(pQRInfo->data.c_str(), (int32)pQRInfo->data.length());


	if (romHash != m_loadedRomHash)
	{
		m_loadedRomHash = romHash;
		//load this instead
		
		//get extra meta data about this rom
		RomPieceHeader romHeader;
		GetApp()->GetRomUtils()->ReadRomPieceHeaderOverRTPackHeader((byte*) pQRInfo->data.c_str(), &romHeader);
		
		rtpack_header *pRTPackHeader = (rtpack_header*)pQRInfo->data.c_str();

		LogMsg("Rom header says piece %d and hash is %d.  Reported compressed size is %d, reported decompressed size is %d.  Compressed recevied size is %d", romHeader.piece, romHeader.hash, 
			pRTPackHeader->compressedSize, pRTPackHeader->decompressedSize,  pQRInfo->data.length());
		if (romHeader.totalPieces  > 1)
		{
			//requires special handling
			if (romHeader.piece == 0)
			{
				m_roms.clear();
				m_roms.push_back(pQRInfo->data);
				m_multiPartHash = romHeader.hash;
				
				SetStatus("`4Please insert side B``");
				return;
			}
			else
			{
				//add this piece?
				if (m_roms.empty())
				{
					SetStatus("`4Insert side A first!``");
					m_loadedRomHash = 0;
			
					return;
				}

				if (romHeader.hash == m_multiPartHash)
				{
					//yes, this is the next piece
					//we're assuming only 2 pieces here.  
					m_roms.push_back(pQRInfo->data);
					//ready to run

				}
				else
				{
					//second piece, but for wrong rom!
					LogMsg("Side B is for wrong game! %d is not %d", romHeader.hash, m_multiPartHash);
					SetStatus("`4Side B is for wrong game!``");
					m_loadedRomHash = 0;
					return;
				}
			}
		}
		else
		{
			//it's a single rom
			m_roms.clear();
			m_roms.push_back(pQRInfo->data);
			m_multiPartHash = 0;
		}
		
		if (!GetApp()->GetRomUtils()->ConvertEncodedTextToRom(m_roms, "decoded_rom.a26"))
		{
			LogMsg("Error converting to ROM.  Ignoring");
			return;
		}

		m_roms.clear();
		m_multiPartHash = 0;
		RunGame();
		
	}
	else
	{
		//do nothing

	}

//just write out a bmp

//GetApp()->GetOpenCV()->GetSoftSurface()->WriteBMPOut("screenshot.bmp");

}

void ConsoleManager::OnNoCardInserted()
{
	//LogMsg("No card inserted");
	m_loadedRomHash = 0; //reset this
	
	if (m_mode == APP_MODE_WAITING_FOR_EMULATOR)
	{
		LogMsg("Killing emu because no card is inserted");
		KillEmulator();

		ZoomToPositionEntity(m_pCart, m_cartNormalPos, 1000, INTERPOLATE_SMOOTHSTEP, 0);
		ScheduleCommand("stop_capture", 0);
		ScheduleCommand("start_capture", 1000);
		ScheduleCommand("start_render", 0);
		GetApp()->GetOpenCV()->SetCaptureFPS(GetApp()->m_cameraFPS);
		m_multiPartHash = 0;
		m_mode = APP_MODE_RUNNING;
		SetStatus("Insert cart");
	}
}

void ConsoleManager::Update()
{
	
	const float maxComplexityConsideredToBeBlank = 12.0f;
	const CL_Vec2i vSampleSize = CL_Vec2i(50, 50);
	bool bCardInserted = false;

	
	if (m_pauseCapture) return; //capturing disabled right now
	
	if (GetApp()->GetOpenCV()->ReadFromCamera())
	{
		GetApp()->GetOpenCV()->CopyLastFrameToSoftSurface();
		
		m_surf.InitFromSoftSurface(GetApp()->GetOpenCV()->GetSoftSurface());
		m_surf.Bind();

		SoftSurface *pSoftSurf = GetApp()->GetOpenCV()->GetSoftSurface();

		CL_Vec2i vSampleStartPos = CL_Vec2i((pSoftSurf->GetWidth() - vSampleSize.x) / 2,
			(pSoftSurf->GetHeight() - vSampleSize.y) / 2);
		float complexity = pSoftSurf->GetAverageComplexityFromRect(vSampleStartPos, vSampleSize);
		//LogMsg("Complexity is %.2f", complexity);

		if (complexity >= maxComplexityConsideredToBeBlank)
		{
			bCardInserted = true;
		}

		if (!bCardInserted)
		{
			OnNoCardInserted();
			return;
		}
		
		if (m_mode == APP_MODE_WAITING_FOR_EMULATOR)
		{
			//don't actually read the QR code, it's too slow and can slow down the emu playing in another thread
			return;
		}

		GetApp()->GetOpenCV()->DecodeQRCode();

		if (GetApp()->GetOpenCV()->GetCountRead() > 0)
		{
			OnLoadedRomFromQR(GetApp()->GetOpenCV()->GetQRReadByIndex(0));

		}
	
	}
	else
	{
		//LogMsg("Failed to read from camera");
	}
}

bool ConsoleManager::ShouldDrawToScreen()
{
	return m_bAllowRenderingToScreen;
}

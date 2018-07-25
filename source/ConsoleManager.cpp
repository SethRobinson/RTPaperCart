#include "PlatformPrecomp.h"
#include "ConsoleManager.h"
#include "App.h"

string RunLinuxShell(string command);


ConsoleManager::ConsoleManager()
{
	m_loadedRomHash = 0;
	m_mode = APP_MODE_RUNNING;
	m_multiPartHash = 0;
	m_removeMessageTimer = 0;

}

ConsoleManager::~ConsoleManager()
{
}

bool ConsoleManager::Init()
{

	return true;
}

void ConsoleManager::KillEmulator()
{
	LogMsg("Killing emu...");
	RunLinuxShell("pkill retroarch");
}

void ConsoleManager::DrawLoadingScreen()
{
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
	

	m_surf.Blit(0, 0);

	string status = "Please wait, loading cart";
	GetApp()->GetFont(FONT_SMALL)->DrawAligned(GetScreenSizeXf() / 2, GetScreenSizeYf() / 2, status, ALIGNMENT_CENTER, 4.0f);

	//the base handles actually drawing the GUI stuff over everything else, if applicable, which in this case it isn't.
	SetupOrtho();
}

void ConsoleManager::RunGame()
{

	if (m_mode == APP_MODE_WAITING_FOR_EMULATOR)
	{
		//we want to run a game but we already are?!
		LogMsg("Detected card while emu is running");
		KillEmulator();
	}


	DrawLoadingScreen();
	ForceVideoUpdate();

	//m_mode = APP_MODE_RUNNING;
	string ret = RunLinuxShell("nohup /opt/retropie/supplementary/runcommand/runcommand.sh 0 _SYS_ atari2600 /home/pi/proton/RTPaperCart/bin/decoded_rom.a26 &");
	LogMsg(ret.c_str());
	m_mode = APP_MODE_WAITING_FOR_EMULATOR;
	LogMsg("Running game");

}

void ConsoleManager::RemoveMessage(int delayMS)
{
	m_removeMessageTimer = GetTick() + delayMS;
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
				
				LogMsg("Please insert side B");
				m_status = "Please insert side B";
				return;
			}
			else
			{
				//add this piece?
				if (m_roms.empty())
				{
					m_status = "Insert side A first!";
					m_loadedRomHash = 0;
					RemoveMessage(1000);

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
					m_status = "Side B is for wrong game!";
					m_loadedRomHash = 0;
					RemoveMessage(1000);
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
			m_status.clear();

		}
		
		
		if (!GetApp()->GetRomUtils()->ConvertEncodedTextToRom(m_roms, "decoded_rom.a26"))
		{
			LogMsg("Error converting to ROM.  Ignoring");
			return;
		}

		m_roms.clear();
		m_multiPartHash = 0;
		RunGame();
		m_status.clear();
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
		m_multiPartHash = 0;
		m_mode = APP_MODE_RUNNING;
	}
}

void ConsoleManager::Draw()
{

	string status;


	switch (m_mode)
	{

	case APP_MODE_WAITING_FOR_EMULATOR:
		status = "(pretend retropie is active)";
		break;

	case APP_MODE_RUNNING:
		status = "Please insert card";
		if (!m_status.empty())
		{
			status = m_status;
		}
		break;
	default:
		status = "???";
	}
	if (GetApp()->GetOpenCV()->GetSoftSurface()->IsActive())
	{

		m_surf.InitFromSoftSurface(GetApp()->GetOpenCV()->GetSoftSurface());
		m_surf.Bind();
	}
	   
	m_surf.Blit(0, 0);
	GetApp()->GetFont(FONT_SMALL)->SetSmoothing(false);
	GetApp()->GetFont(FONT_SMALL)->DrawAligned(GetScreenSizeXf() / 2, GetScreenSizeYf() / 2, status, ALIGNMENT_CENTER, 3.0f);
	

}
void ConsoleManager::Update()
{
	//game is thinking.  
	if (m_removeMessageTimer != 0 && m_removeMessageTimer < GetTick())
	{
		m_removeMessageTimer = 0;
		m_status = "";
	}
	const float maxComplexityConsideredToBeBlank = 7.0f;
	const CL_Vec2i vSampleSize = CL_Vec2i(50, 50);

	if (GetApp()->GetOpenCV()->ReadFromCamera())
	{
		GetApp()->GetOpenCV()->CopyLastFrameToSoftSurface();
		GetApp()->GetOpenCV()->DecodeQRCode();

		if (GetApp()->GetOpenCV()->GetCountRead() > 0)
		{

			OnLoadedRomFromQR(GetApp()->GetOpenCV()->GetQRReadByIndex(0));

		}
		else
		{
			//no QR code detected.  If no card is even inserted, let's detect that and kill any running app

			SoftSurface *pSoftSurf = GetApp()->GetOpenCV()->GetSoftSurface();

			CL_Vec2i vSampleStartPos = CL_Vec2i( (pSoftSurf->GetWidth() - vSampleSize.x) / 2,
				(pSoftSurf->GetHeight() - vSampleSize.y) / 2);
			float complexity = pSoftSurf->GetAverageComplexityFromRect(vSampleStartPos, vSampleSize);
			//LogMsg("Complexity is %.2f", complexity);

			if (complexity < maxComplexityConsideredToBeBlank)
			{
				OnNoCardInserted();
			}

		}
	}
}
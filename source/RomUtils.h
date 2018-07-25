//  ***************************************************************
//  RomUtils - Creation date: 07/18/2018
//  -------------------------------------------------------------
//  Robinson Technologies Copyright (C) 2018 - All Rights Reserved
//
//  ***************************************************************
//  Programmer(s):  Seth A. Robinson (seth@rtsoft.com)
//  ***************************************************************

#ifndef RomUtils_h__
#define RomUtils_h__


//we can write up to 15 bytes.  NOT MORE!
struct RomPieceHeader
{
	byte piece;
	byte totalPieces;
	unsigned int hash;

};

class RomUtils
{
public:
	RomUtils();
	virtual ~RomUtils();

	void Init();

	void ReadRomPieceHeaderOverRTPackHeader(byte *pData, RomPieceHeader *pHeaderOut);
	bool WriteRomAsQRCode(string romFileName); //crap.bin would write to "crap_data.html".  Also returns QR code as text.
	bool ConvertEncodedTextToRom(vector<string> roms, string fileNameToWrite);

protected:
	void WriteRomPieceHeaderOverRTPackHeader(byte *pData, RomPieceHeader *pHeader);


private:
};

#endif // RomUtils_h__

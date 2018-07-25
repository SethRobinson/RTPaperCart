#include "PlatformPrecomp.h"
#include "RomUtils.h"
#include "Network/NetUtils.h"
#include "Manager/QRGenerateManager.h"


RomUtils::RomUtils()
{
}

RomUtils::~RomUtils()
{
}

void RomUtils::Init()
{
}


const int RTPACK_HEADER_OFFSET_FOR_ROM_HEADER = 17;

void RomUtils::WriteRomPieceHeaderOverRTPackHeader(byte *pData, RomPieceHeader *pHeader)
{
	memcpy(pData + RTPACK_HEADER_OFFSET_FOR_ROM_HEADER, pHeader, sizeof(RomPieceHeader));
}
void RomUtils::ReadRomPieceHeaderOverRTPackHeader(byte *pData, RomPieceHeader *pHeaderOut)
{
	memcpy(pHeaderOut, pData + RTPACK_HEADER_OFFSET_FOR_ROM_HEADER, sizeof(RomPieceHeader));
}


bool RomUtils::ConvertEncodedTextToRom(vector<string> roms, string fileNameToWrite)
{
	int totalSize = 0;

	for (int i = 0; i < roms.size(); i++)
	{
		rtpack_header *pHeader = (rtpack_header*)roms[i].c_str();
		totalSize += pHeader->decompressedSize;
	}

	byte *pOutput = new byte[totalSize];

	int curPos = 0;

	for (int i = 0; i < roms.size(); i++)
	{
		unsigned int decompressedSize;
		byte *pDecompressedPiece = DecompressRTPackToMemory((byte*)roms[i].c_str(), &decompressedSize);

		/*
		//write out data test
		FILE *fp = fopen(("crap_" + toString(i)).c_str(), "wb");
		fwrite(pDecompressedPiece, decompressedSize, 1, fp);
		fclose(fp);
		*/


		//copy over to our master
		memcpy(pOutput + curPos, pDecompressedPiece, decompressedSize);
		curPos += decompressedSize;
		SAFE_DELETE_ARRAY(pDecompressedPiece);
	}
	
	
	//write final file
	FILE *fp = fopen(fileNameToWrite.c_str(), "wb");
	fwrite(pOutput, 1, totalSize, fp);
	fclose(fp);
	SAFE_DELETE_ARRAY(pOutput);
	return true;
}

bool RomUtils::WriteRomAsQRCode(string romFileName)
{
	const int maxCompressedBytesPerCart = 2800; //sadly, just under pitfall's size
	const int uncompressedBytesPerCartWhenDoingMulti = 2048;
	unsigned int uncompressedTotalBytes = GetFileSize(romFileName);

	
	if (!FileExists(romFileName))
	{
		LogMsg("Skipping RomUtil thing, file doesn't exist");
		return false;
	}

	
	CompressFile(romFileName);
	string romFileNamertpack = ModifyFileExtension(romFileName, "rtpak");
	int compressedTotalBytes = GetFileSize(romFileNamertpack);

	LogMsg("Original is %d bytes, compressed size is %d bytes.  Decided to split into multiparts or not.", uncompressedTotalBytes, compressedTotalBytes);
	int pieces = 1;
	int uncompressedBytesPerPiece = uncompressedTotalBytes; //Assuming 1 piece for now

	if (compressedTotalBytes > maxCompressedBytesPerCart)
	{
		pieces = (compressedTotalBytes / uncompressedBytesPerCartWhenDoingMulti) + 1;
		uncompressedBytesPerPiece = uncompressedBytesPerCartWhenDoingMulti;
		assert(pieces > 1);
		LogMsg("Too big for a QR code, breaking it up into %d pieces.", pieces);
	}

	//load the raw bytes and compressed into RTPAKs as needed
	byte *pSource = LoadFileIntoMemoryBasic(romFileName, &uncompressedTotalBytes);

	int hash = HashString((char*)pSource, uncompressedTotalBytes);

	unsigned int compressedPieceSize;
	int bytesLeft = uncompressedTotalBytes;

	RomPieceHeader romPieceHeader;

	for (int piece = 0; piece < pieces; piece++)
	{
	
		int uncompressedPieceSizeTemp = uncompressedBytesPerPiece;
		uncompressedPieceSizeTemp = rt_min(uncompressedPieceSizeTemp, bytesLeft);
		bytesLeft -= uncompressedPieceSizeTemp;
		byte *pPiece = CompressMemoryToRTPack(pSource + (piece*uncompressedBytesPerPiece), uncompressedPieceSizeTemp, &compressedPieceSize);

		//for testing, write each piece out
		
		/*
		   
			FILE *fp = fopen((GetFileNameWithoutExtension(romFileName) + "_" + toString(piece + 1) + " of " + toString(pieces) + ".dat").c_str(), "wb");
			fwrite(pSource + (piece*uncompressedBytesPerPiece), uncompressedPieceSizeTemp, 1, fp);
			fclose(fp);
		*/

		//decompress and write each one out as a test

	/*
		unsigned int bytesWritten;
		byte *pCrap = DecompressRTPackToMemory(pPiece, &bytesWritten);
		FILE *fp = fopen((GetFileNameWithoutExtension(romFileName) + "_" + toString(piece + 1) + " of " + toString(pieces) + ".dat").c_str(), "wb");
		fwrite(pCrap, bytesWritten, 1, fp);
		fclose(fp);
		SAFE_DELETE_ARRAY(pCrap);
		*/
		
		romPieceHeader.piece = piece;
		romPieceHeader.totalPieces = pieces;
		romPieceHeader.hash = hash;
		WriteRomPieceHeaderOverRTPackHeader(pPiece, &romPieceHeader);



		//build a string with binary data.
		string compressedRomStringBinaryData;
		compressedRomStringBinaryData.resize(compressedPieceSize);
		memcpy((void*)compressedRomStringBinaryData.c_str(), pPiece, compressedPieceSize);

		//ok, we have this piece sitting here.  Let's make the QR code with it
		QRGenerateManager qr;
		vector<string> qrCode = qr.MakeQRWithData(compressedRomStringBinaryData, GetFileNameWithoutExtension(romFileName) + "_"+toString(piece+1)+" of "+toString(pieces)+".html");

		
		

		SAFE_DELETE_ARRAY(pPiece);
	}
		LogMsg("Compressed rom to %d pieces.", pieces);


		return true;
}
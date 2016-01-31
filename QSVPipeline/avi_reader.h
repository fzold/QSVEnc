﻿//  -----------------------------------------------------------------------------------------
//    QSVEnc by rigaya
//  -----------------------------------------------------------------------------------------
//   ソースコードについて
//   ・無保証です。
//   ・本ソースコードを使用したことによるいかなる損害・トラブルについてrigayaは責任を負いません。
//   以上に了解して頂ける場合、本ソースコードの使用、複製、改変、再頒布を行って頂いて構いません。
//  ---------------------------------------------------------------------------------------
#ifndef _AVI_READER_H_
#define _AVI_READER_H_

#include "qsv_version.h"
#if ENABLE_AVI_READER
#include <Windows.h>
#include <vfw.h>
#pragma comment(lib, "vfw32.lib")
#include "qsv_input.h"

class CAVIReader : public CQSVInput
{
public:
    CAVIReader();
    virtual ~CAVIReader();

    virtual mfxStatus Init(const TCHAR *strFileName, mfxU32 ColorFormat, const void *option, CEncodingThread *pEncThread, shared_ptr<CEncodeStatusInfo> pEncSatusInfo, sInputCrop *pInputCrop) override;

    virtual void Close();
    virtual mfxStatus LoadNextFrame(mfxFrameSurface1* pSurface) override;

    PAVIFILE m_pAviFile;
    PAVISTREAM m_pAviStream;
    PGETFRAME m_pGetFrame;
    LPBITMAPINFOHEADER m_pBitmapInfoHeader;
    int m_nYPitchMultiplizer;
};

#endif //ENABLE_AVI_READER

#endif //_AVI_READER_H_
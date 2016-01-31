﻿//  -----------------------------------------------------------------------------------------
//    QSVEnc by rigaya
//  -----------------------------------------------------------------------------------------
//   ソースコードについて
//   ・無保証です。
//   ・本ソースコードを使用したことによるいかなる損害・トラブルについてrigayaは責任を負いません。
//   以上に了解して頂ける場合、本ソースコードの使用、複製、改変、再頒布を行って頂いて構いません。
//  ---------------------------------------------------------------------------------------
#ifndef _AVCODEC_QSV_H_
#define _AVCODEC_QSV_H_

#include "qsv_version.h"

#if ENABLE_AVCODEC_QSV_READER
#include <algorithm>

#pragma warning (push)
#pragma warning (disable: 4244)
extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/error.h>
#include <libavutil/opt.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}
#pragma comment (lib, "avcodec.lib")
#pragma comment (lib, "avformat.lib")
#pragma comment (lib, "avutil.lib")
#pragma comment (lib, "swresample.lib")
#pragma warning (pop)

#include "qsv_util.h"
#include "qsv_prm.h"

#if _DEBUG
#define QSV_AV_LOG_LEVEL AV_LOG_WARNING
#else
#define QSV_AV_LOG_LEVEL AV_LOG_ERROR
#endif

typedef struct QSVCodec {
    uint32_t codec_id;   //avcodecのコーデックID
    uint32_t qsv_fourcc; //QSVのfourcc
} QSVCodec;

//QSVでデコード可能なコーデックのリスト
static const QSVCodec QSV_DECODE_LIST[] = { 
    { AV_CODEC_ID_H264,       MFX_CODEC_AVC   },
    { AV_CODEC_ID_HEVC,       MFX_CODEC_HEVC  },
    { AV_CODEC_ID_MPEG2VIDEO, MFX_CODEC_MPEG2 },
    { AV_CODEC_ID_VC1,        MFX_CODEC_VC1   },
    { AV_CODEC_ID_WMV3,       MFX_CODEC_VC1   },
};

static const TCHAR *AVQSV_CODEC_AUTO = _T("auto");
static const TCHAR *AVQSV_CODEC_COPY = _T("copy");

static const int AVQSV_DEFAULT_AUDIO_BITRATE = 192;

static inline bool avcodecIsCopy(const TCHAR *codec) {
    return codec == nullptr || 0 == _tcsicmp(codec, AVQSV_CODEC_COPY);
}
static inline bool avcodecIsAuto(const TCHAR *codec) {
    return codec != nullptr && 0 == _tcsicmp(codec, AVQSV_CODEC_AUTO);
}

//AV_LOG_TRACE    56 - QSV_LOG_TRACE -3
//AV_LOG_DEBUG    48 - QSV_LOG_DEBUG -2
//AV_LOG_VERBOSE  40 - QSV_LOG_MORE  -1
//AV_LOG_INFO     32 - QSV_LOG_INFO   0
//AV_LOG_WARNING  24 - QSV_LOG_WARN   1
//AV_LOG_ERROR    16 - QSV_LOG_ERROR  2
static inline int log_level_av2qsv(int level) {
    return clamp((AV_LOG_INFO / 8) - (level / 8), QSV_LOG_TRACE, QSV_LOG_ERROR);
}

static inline int log_level_qsv2av(int level) {
    return clamp(AV_LOG_INFO - level * 8, AV_LOG_QUIET, AV_LOG_TRACE);
}

static const AVRational QSV_NATIVE_TIMEBASE = { 1, QSV_TIMEBASE };
static const TCHAR *AVCODEC_DLL_NAME[] = {
    _T("avcodec-57.dll"), _T("avformat-57.dll"), _T("avutil-55.dll"), _T("swresample-2.dll")
};

enum AVQSVCodecType : uint32_t {
    AVQSV_CODEC_DEC = 0x01,
    AVQSV_CODEC_ENC = 0x02,
};

enum AVQSVFormatType : uint32_t {
    AVQSV_FORMAT_DEMUX = 0x01,
    AVQSV_FORMAT_MUX   = 0x02,
};

//avcodecのエラーを表示
tstring qsv_av_err2str(int ret);

//コーデックの種類を表示
tstring get_media_type_string(AVCodecID codecId);

//必要なavcodecのdllがそろっているかを確認
bool check_avcodec_dll();

//avcodecのdllが存在しない場合のエラーメッセージ
tstring error_mes_avcodec_dll_not_found();

//avcodecのライセンスがLGPLであるかどうかを確認
bool checkAvcodecLicense();

//avqsvでサポートされている動画コーデックを表示
tstring getAVQSVSupportedCodecList();

//利用可能な音声エンコーダ/デコーダを表示
tstring getAVCodecs(AVQSVCodecType flag);

//利用可能なフォーマットを表示
tstring getAVFormats(AVQSVFormatType flag);

//チャンネルレイアウトを表示
tstring getChannelLayoutString(int channels, uint64_t channel_layout);

#endif //ENABLE_AVCODEC_QSV_READER

#endif //_AVCODEC_QSV_H_
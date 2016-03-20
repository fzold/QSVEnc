﻿//  -----------------------------------------------------------------------------------------
//    QSVEnc by rigaya
//  -----------------------------------------------------------------------------------------
//   ソースコードについて
//   ・無保証です。
//   ・本ソースコードを使用したことによるいかなる損害・トラブルについてrigayaは責任を負いません。
//   以上に了解して頂ける場合、本ソースコードの使用、複製、改変、再頒布を行って頂いて構いません。
//  ---------------------------------------------------------------------------------------

#include <numeric>
#include "qsv_version.h"

#if ENABLE_AVCODEC_QSV_READER

#include "avcodec_qsv.h"

//必要なavcodecのdllがそろっているかを確認
bool check_avcodec_dll() {
#if defined(_WIN32) || defined(_WIN64)
    std::vector<HMODULE> hDllList;
    bool check = true;
    for (int i = 0; i < _countof(AVCODEC_DLL_NAME); i++) {
        HMODULE hDll = NULL;
        if (NULL == (hDll = LoadLibrary(AVCODEC_DLL_NAME[i]))) {
            check = false;
            break;
        }
        hDllList.push_back(hDll);
    }
    for (auto hDll : hDllList) {
        FreeLibrary(hDll);
    }
    return check;
#else
    return true;
#endif //#if defined(_WIN32) || defined(_WIN64)
}

//avcodecのdllが存在しない場合のエラーメッセージ
tstring error_mes_avcodec_dll_not_found() {
    tstring mes;
    mes += _T("avcodec: failed to load dlls.\n");
    mes += _T("please make sure ");
    for (int i = 0; i < _countof(AVCODEC_DLL_NAME); i++) {
        if (i) mes += _T(", ");
        if (i % 3 == 2) {
            mes += _T("\n");
        }
        mes += _T("\"") + tstring(AVCODEC_DLL_NAME[i]) + _T("\"");
    }
    mes += _T("\nis installed in your system.\n");
    return mes;
}

//avcodecのライセンスがLGPLであるかどうかを確認
bool checkAvcodecLicense() {
    auto check = [](const char *license) {
        std::string str(license);
        transform(str.begin(), str.end(), str.begin(), [](char in) -> char {return (char)tolower(in); });
        return std::string::npos != str.find("lgpl");
    };
    return (check(avutil_license()) && check(avcodec_license()) && check(avformat_license()));
}

//avcodecのエラーを表示
tstring qsv_av_err2str(int ret) {
    char mes[256];
    av_make_error_string(mes, sizeof(mes), ret);
    return char_to_tstring(mes);
}

//コーデックの種類を表示
tstring get_media_type_string(AVCodecID codecId) {
    return char_to_tstring(av_get_media_type_string(avcodec_get_type(codecId))).c_str();
}

//avqsvでサポートされている動画コーデックを表示
tstring getAVQSVSupportedCodecList() {
    tstring codecs;
    for (int i = 0; i < _countof(QSV_DECODE_LIST); i++) {
        if (i == 0 || QSV_DECODE_LIST[i-1].qsv_fourcc != QSV_DECODE_LIST[i].qsv_fourcc) {
            if (i) codecs += _T(", ");
            codecs += CodecIdToStr(QSV_DECODE_LIST[i].qsv_fourcc);
        }
    }
    return codecs;
}

//利用可能な音声エンコーダ/デコーダを表示
tstring getAVCodecs(AVQSVCodecType flag) {
    if (!check_avcodec_dll()) {
        return error_mes_avcodec_dll_not_found();
    }
    av_register_all();
    avcodec_register_all();
    
    struct avcodecName {
        uint32_t type;
        const char *name;
        const char *long_name;
    };

    vector<avcodecName> list;

    AVCodec *codec = nullptr;
    while (nullptr != (codec = av_codec_next(codec))) {
        if (codec->type == AVMEDIA_TYPE_AUDIO || codec->type == AVMEDIA_TYPE_SUBTITLE) {
            bool alreadyExists = false;
            for (uint32_t i = 0; i < list.size(); i++) {
                if (0 == strcmp(list[i].name, codec->name)) {
                    list[i].type |= codec->decode ? AVQSV_CODEC_DEC : 0x00;
                    list[i].type |= codec->encode2 ? AVQSV_CODEC_ENC : 0x00;
                    alreadyExists = true;
                    break;
                }
            }
            if (!alreadyExists) {
                uint32_t type = 0x00;
                type |= codec->decode ? AVQSV_CODEC_DEC : 0x00;
                type |= codec->encode2 ? AVQSV_CODEC_ENC : 0x00;
                list.push_back({ type, codec->name, codec->long_name });
            }
        }
    }

    std::sort(list.begin(), list.end(), [](const avcodecName& x, const avcodecName& y) {
        int i = 0;
        for (; x.name[i] && y.name[i]; i++) {
            if (x.name[i] != y.name[i]) {
                return x.name[i] < y.name[i];
            }
        }
        return x.name[i] < y.name[i];
    });
    uint32_t maxNameLength = 0;
    std::for_each(list.begin(), list.end(), [&maxNameLength](const avcodecName& format) { maxNameLength = (std::max)(maxNameLength, (uint32_t)strlen(format.name)); });
    maxNameLength = (std::min)(maxNameLength, 15u);

    uint32_t flag_dec = flag & AVQSV_CODEC_DEC;
    uint32_t flag_enc = flag & AVQSV_CODEC_ENC;
    int flagCount = popcnt32(flag);

    std::string codecstr = (flagCount > 1) ? "D-: Decode\n-E: Encode\n---------------------\n" : "";
    std::for_each(list.begin(), list.end(), [&codecstr, maxNameLength, flagCount, flag_dec, flag_enc](const avcodecName& format) {
        if (format.type & (flag_dec | flag_enc)) {
            if (flagCount > 1) {
                codecstr += (format.type & flag_dec) ? "D" : "-";
                codecstr += (format.type & flag_enc) ? "E" : "-";
                codecstr += " ";
            }
            codecstr += format.name;
            if (format.long_name) {
                for (uint32_t i = (uint32_t)strlen(format.name); i <= maxNameLength; i++)
                    codecstr += " ";
                codecstr += ": " + std::string(format.long_name);
            }
            codecstr += "\n";
        }
    });

    return char_to_tstring(codecstr);
}

//利用可能なフォーマットを表示
tstring getAVFormats(AVQSVFormatType flag) {
    if (!check_avcodec_dll()) {
        return error_mes_avcodec_dll_not_found();
    }
    av_register_all();
    avcodec_register_all();

    struct avformatName {
        uint32_t type;
        const char *name;
        const char *long_name;
    };

    vector<avformatName> list;

    std::string codecstr;
    AVInputFormat *iformat = nullptr;
    while (nullptr != (iformat = av_iformat_next(iformat))) {
        bool alreadyExists = false;
        for (uint32_t i = 0; i < list.size(); i++) {
            if (0 == strcmp(list[i].name, iformat->name)) {
                list[i].type |= AVQSV_FORMAT_DEMUX;
                alreadyExists = true;
                break;
            }
        }
        if (!alreadyExists) {
            list.push_back({ AVQSV_FORMAT_DEMUX, iformat->name, iformat->long_name });
        }
    }
    
    AVOutputFormat *oformat = nullptr;
    while (nullptr != (oformat = av_oformat_next(oformat))) {
        bool alreadyExists = false;
        for (uint32_t i = 0; i < list.size(); i++) {
            if (0 == strcmp(list[i].name, oformat->name)) {
                list[i].type |= AVQSV_FORMAT_MUX;
                alreadyExists = true;
                break;
            }
        }
        if (!alreadyExists) {
            list.push_back({ AVQSV_FORMAT_MUX, oformat->name, oformat->long_name });
        }
    }

    std::sort(list.begin(), list.end(), [](const avformatName& x, const avformatName& y) {
        int i = 0;
        for (; x.name[i] && y.name[i]; i++) {
            if (x.name[i] != y.name[i]) {
                return x.name[i] < y.name[i];
            }
        }
        return x.name[i] < y.name[i];
    });

    uint32_t maxNameLength = 0;
    std::for_each(list.begin(), list.end(), [&maxNameLength](const avformatName& format) { maxNameLength = (std::max)(maxNameLength, (uint32_t)strlen(format.name)); });
    maxNameLength = (std::min)(maxNameLength, 15u);

    uint32_t flag_demux = flag & AVQSV_FORMAT_DEMUX;
    uint32_t flag_mux = flag & AVQSV_FORMAT_MUX;
    int flagCount = popcnt32(flag);

    std::string formatstr = (flagCount > 1) ? "D-: Demux\n-M: Mux\n---------------------\n" : "";
    std::for_each(list.begin(), list.end(), [&formatstr, maxNameLength, flagCount, flag_demux, flag_mux](const avformatName& format) {
        if (format.type & (flag_demux | flag_mux)) {
            if (flagCount > 1) {
                formatstr += (format.type & flag_demux) ? "D" : "-";
                formatstr += (format.type & flag_mux) ? "M" : "-";
                formatstr += " ";
            }
            formatstr += format.name;
            if (format.long_name) {
                for (uint32_t i = (uint32_t)strlen(format.name); i <= maxNameLength; i++)
                    formatstr += " ";
                formatstr += ": " + std::string(format.long_name);
            }
            formatstr += "\n";
        }
    });

    return char_to_tstring(formatstr);
}

std::string getChannelLayoutChar(int channels, uint64_t channel_layout) {
    char string[1024] = { 0 };
    av_get_channel_layout_string(string, _countof(string), channels, channel_layout);
    if (auto ptr = strstr(string, " channel")) {
        strcpy(ptr, "ch");
    }
    if (auto ptr = strstr(string, "channel")) {
        strcpy(ptr, "ch");
    }
    if (0 == _strnicmp(string, "stereo", strlen("stereo"))) {
        return "2ch";
    }
    return string;
}

tstring getChannelLayoutString(int channels, uint64_t channel_layout) {
    return char_to_tstring(getChannelLayoutChar(channels, channel_layout));
}

vector<std::string> getAVProtocolList(int bOutput) {
    vector<std::string> protocols;

    void *opaque = nullptr;
    const char *name = nullptr;
    while (nullptr != (name = avio_enum_protocols(&opaque, bOutput))) {
        std::string data = name;
        std::transform(data.begin(), data.end(), data.begin(), ::tolower);
        protocols.push_back(data);
    }
    return protocols;
}

tstring getAVProtocols() {
    if (!check_avcodec_dll()) {
        return error_mes_avcodec_dll_not_found();
    }
    av_register_all();
    avcodec_register_all();

    const auto inputProtocols  = getAVProtocolList(0);
    const auto outputProtocols = getAVProtocolList(1);

    auto max_len = std::accumulate(inputProtocols.begin(),  inputProtocols.end(), (size_t)0, [](const size_t max_len, const std::string& str) { return (std::max)(max_len, str.length()); });
    max_len      = std::accumulate(outputProtocols.begin(), outputProtocols.end(), max_len,  [](const size_t max_len, const std::string& str) { return (std::max)(max_len, str.length()); });
    max_len += 1;

    std::string mes = "input protocols:\n";
    size_t len = 0;
    for (const auto& protocols : inputProtocols) {
        mes += protocols;
        for (auto i = protocols.length(); i < max_len; i++) {
            mes += " ";
        }
        len += max_len;
        if (len >= 79 - max_len) {
            mes += "\n";
            len = 0;
        }
    }
    mes += "\n\noutput protocols:\n";
    len = 0;
    for (const auto& protocols : outputProtocols) {
        mes += protocols;
        for (auto i = protocols.length(); i < max_len; i++) {
            mes += " ";
        }
        len += max_len;
        if (len >= 79 - max_len) {
            mes += "\n";
            len = 0;
        }
    }
    return char_to_tstring(mes);
}

bool usingAVProtocols(std::string filename, int bOutput) {
    if (!check_avcodec_dll()) {
        return false;
    }
    const auto protocolList = getAVProtocolList(bOutput);
    const auto pos = filename.find_first_of(':');
    if (pos != std::string::npos) {
        std::string check = filename.substr(0, pos);
        std::transform(check.begin(), check.end(), check.begin(), tolower);
        if (std::find(protocolList.begin(), protocolList.end(), check) != protocolList.end()) {
            return true;
        }
    }
    return false;
}

tstring getAVVersions() {
    if (!check_avcodec_dll()) {
        return error_mes_avcodec_dll_not_found();
    }
    const uint32_t ver = avutil_version();
    auto ver2str = [](uint32_t ver) {
        return strsprintf("%3d.%3d.%4d", (ver >> 16) & 0xff, (ver >> 8) & 0xff, ver & 0xff);
    };
    std::string mes;
    mes  = std::string("ffmpeg     version: ") + std::string(av_version_info()) + "\n";
    mes += std::string("avutil     version: ") + ver2str(avutil_version()) + "\n";
    mes += std::string("avcodec    version: ") + ver2str(avcodec_version()) + "\n";
    mes += std::string("avformat   version: ") + ver2str(avformat_version()) + "\n";
    mes += std::string("swresample version: ") + ver2str(swresample_version()) + "\n";
    return char_to_tstring(mes);
}

static bool avformat_network_initialized = false;

bool avformatNetworkInit() {
    if (!check_avcodec_dll()) {
        return false;
    }
    if (avformat_network_initialized) {
        return false;
    }
    avformat_network_initialized = true;
    avformat_network_init();
    return true;
}

void avformatNetworkDeinit() {
    if (avformat_network_initialized) {
        avformat_network_initialized = false;
        avformat_network_deinit();
    }
}

#endif //ENABLE_AVCODEC_QSV_READER

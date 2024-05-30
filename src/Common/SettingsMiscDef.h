#pragma once

#include "CoreModel/Locale/CLocaleTextManager.h"

#define VOLUME_METER_DECAY_FAST                  23.53
#define VOLUME_METER_DECAY_MEDIUM                11.76
#define VOLUME_METER_DECAY_SLOW                  8.57


#define DESKTOP_AUDIO_1 AFLocaleTextManager::GetSingletonInstance().Str("DesktopAudioDevice1")
#define DESKTOP_AUDIO_2 AFLocaleTextManager::GetSingletonInstance().Str("DesktopAudioDevice2")
#define AUX_AUDIO_1 AFLocaleTextManager::GetSingletonInstance().Str("AuxAudioDevice1")
#define AUX_AUDIO_2 AFLocaleTextManager::GetSingletonInstance().Str("AuxAudioDevice2")
#define AUX_AUDIO_3 AFLocaleTextManager::GetSingletonInstance().Str("AuxAudioDevice3")
#define AUX_AUDIO_4 AFLocaleTextManager::GetSingletonInstance().Str("AuxAudioDevice4")

#define SIMPLE_ENCODER_X264 "x264"
#define SIMPLE_ENCODER_X264_LOWCPU "x264_lowcpu"
#define SIMPLE_ENCODER_QSV "qsv"
#define SIMPLE_ENCODER_QSV_AV1 "qsv_av1"
#define SIMPLE_ENCODER_NVENC "nvenc"
#define SIMPLE_ENCODER_NVENC_AV1 "nvenc_av1"
#define SIMPLE_ENCODER_NVENC_HEVC "nvenc_hevc"
#define SIMPLE_ENCODER_AMD "amd"
#define SIMPLE_ENCODER_AMD_HEVC "amd_hevc"
#define SIMPLE_ENCODER_AMD_AV1 "amd_av1"
#define SIMPLE_ENCODER_APPLE_H264 "apple_h264"
#define SIMPLE_ENCODER_APPLE_HEVC "apple_hevc"

#define SERVICE_PATH "service.json"

#define FTL_PROTOCOL "ftl"
#define RTMP_PROTOCOL "rtmp"
#define SRT_PROTOCOL "srt"
#define RIST_PROTOCOL "rist"

#define MBYTE (1024ULL * 1024ULL)
#define MBYTES_LEFT_STOP_REC 50ULL
#define MAX_BYTES_LEFT (MBYTES_LEFT_STOP_REC * MBYTE)

// for log
#define RECORDING_START     "==== Recording Start ==============================================="
#define RECORDING_STOP      "==== Recording Stop ================================================"
#define REPLAY_BUFFER_START "==== Replay Buffer Start ==========================================="
#define REPLAY_BUFFER_STOP  "==== Replay Buffer Stop ============================================"
#define STREAMING_START     "==== Streaming Start ==============================================="
#define STREAMING_STOP      "==== Streaming Stop ================================================"

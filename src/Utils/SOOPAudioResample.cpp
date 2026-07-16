
#include "SOOPAudioResample.h"

#include "obs-module.h"

namespace SOOPAUDIORESAMPLE {
    namespace THREAD {
        const unsigned int S_CYCLE = 32; // Suceeded (ms)
        const unsigned int F_CYCLE = 1000; // Failed (ms)
    }
}

bool SOOPAudioResample::Initialize() {
    m_bWork = true;
    m_cThread = std::thread(&SOOPAudioResample::_Thread, this);
    if (m_cThread.joinable() == false) {
        blog(LOG_ERROR, "%s (%d) : ERROR", __FILE__, __LINE__);
        return false;
    }

    //

	return true;
}

void SOOPAudioResample::Finalize() {
    do {
        if (m_cThread.joinable() == false) {
            blog(LOG_ERROR, "%s (%d) : ERROR", __FILE__, __LINE__);
            break;
        }

        m_bWork = false;
        m_cThread.join();
    } while (false);
}

bool SOOPAudioResample::SetInputFormat(int iSPS, enum audio_format eAF, enum speaker_layout eSL) {
    std::lock_guard<std::mutex> tcLock(m_cIM);

    //
    
    if ((m_iISPS != iSPS) || (m_eIAF != eAF) || (m_eISL != eSL)) {
        if (m_tcSP != nullptr)
            m_tcSP.reset();

        m_tcIVV.clear();

        m_iISPS = iSPS;
        m_eIAF = eAF;
        m_eISL = eSL;
        m_iMinIF = iSPS / 100; // 10 ms
        m_iMinIS = get_audio_size(eAF, eSL, m_iMinIF); // 10 ms
        m_iMaxIS = m_iMinIS * 100; // 1000 ms
        m_tcIVV.resize(get_audio_channels(eSL));
    }

    //

    return true;
}

bool SOOPAudioResample::SetInputBuffer(const struct audio_data* audio_data) {
    std::lock_guard<std::mutex> tcLock(m_cIM);

    //

    if (m_tcIVV.empty() == true) {
        //blog(LOG_ERROR, "%s (%d) : ERROR", __FILE__, __LINE__);
        return false;
    }

    if (m_tcIVV.size() > MAX_AV_PLANES) {
        //blog(LOG_ERROR, "%s (%d) : ERROR", __FILE__, __LINE__);
        return false;
    }

    if (m_tcIVV[0].size() >= m_iMaxIS) {
        //blog(LOG_ERROR, "%s (%d) : FULL", __FILE__, __LINE__);
        for (auto& tcIV : m_tcIVV) tcIV.clear();
    }

    //

    if (audio_data == nullptr) {
        //blog(LOG_ERROR, "%s (%d) : audio_data : %d", __FILE__, __LINE__, audio_data);
        return false;
    }

    if (audio_data->frames < m_iMinIF) {
        //blog(LOG_ERROR, "%s (%d) : audio_data->frames : %d", __FILE__, __LINE__, audio_data->frames);
        return false;
    }

    //

    for (int i = 0, j = m_tcIVV.size(), k = get_audio_size(m_eIAF, m_eISL, audio_data->frames); i < j; ++i) {
        m_tcIVV[i].reserve(m_tcIVV[i].size() + k);

        if (audio_data->data[i] == nullptr) {
            //blog(LOG_ERROR, "%s (%d) : audio_data->data[%d] : %d", __FILE__, __LINE__, i, audio_data->data[i]);
            return false;
        }

        m_tcIVV[i].insert(m_tcIVV[i].end(), audio_data->data[i], audio_data->data[i] + k);
    }

    //

    return true;
}

bool SOOPAudioResample::SetOutputFormat(int iSPS, enum audio_format eAF, enum speaker_layout eSL) {
    std::lock_guard<std::mutex> tcLock(m_cOM);

    //

    if ((m_iOSPS != iSPS) || (m_eOAF != eAF) || (m_eOSL != eSL)) {
        if (m_tcSP != nullptr)
            m_tcSP.reset();

        m_tcOVV.clear();

        m_iOSPS = iSPS;
        m_eOAF = eAF;
        m_eOSL = eSL;
        m_iOBPC = get_audio_size(eAF, eSL, (iSPS / 1000) * SOOPAUDIORESAMPLE::THREAD::S_CYCLE);
        m_iLate = m_iOBPC;
        m_tcOVV.resize(get_audio_channels(eSL));
    }

    //

    return true;
}

bool SOOPAudioResample::GetOutputBuffer(std::vector<std::vector<uint8_t>>& tcVV) {
    std::lock_guard<std::mutex> tcLock(m_cOM);
    
    //

    if (tcVV.data() == nullptr) {
        blog(LOG_ERROR, "%s (%d) : tcVV.data() : %d", __FILE__, __LINE__, tcVV.data());
        return false;
    }

    if (m_tcOVV[0].size() >= m_iOBPC) {
        if (tcVV.size() != m_tcOVV.size()) {
            blog(LOG_ERROR, "%s (%d) : ERROR", __FILE__, __LINE__);
            return false;
        }

        for (size_t i = 0, j = tcVV.size(); i < j; ++i) {
            tcVV[i].assign(std::make_move_iterator(m_tcOVV[i].begin()), std::make_move_iterator(m_tcOVV[i].begin() + m_iOBPC));
            m_tcOVV[i].erase(m_tcOVV[i].begin(), m_tcOVV[i].begin() + m_iOBPC);
        }
    }
#if 0
    else
        blog(LOG_INFO, "%s (%d) : LESS : %d", __FILE__, __LINE__, m_tcOVV[0].size());
#endif // 0


    //

    return true;
}

void SOOPAudioResample::_Thread() {
    while (m_bWork == true) {
        if (_Work() == true) {
            auto cCurTime = std::chrono::steady_clock::now();
            if (cCurTime <= m_cRefTime) {
                std::this_thread::sleep_until(m_cRefTime);
            }
            else {
                auto cDifTime = cCurTime - m_cRefTime;
                if (cDifTime >= std::chrono::milliseconds(SOOPAUDIORESAMPLE::THREAD::S_CYCLE)) 
                    m_cRefTime = cCurTime;
            }

            m_cRefTime += std::chrono::milliseconds(SOOPAUDIORESAMPLE::THREAD::S_CYCLE);
        }
        else {
            _Clear();

            std::this_thread::sleep_for(std::chrono::milliseconds(SOOPAUDIORESAMPLE::THREAD::F_CYCLE));
            m_cRefTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(SOOPAUDIORESAMPLE::THREAD::S_CYCLE);
        }
    }


}

bool SOOPAudioResample::_Work() {
    std::scoped_lock tcLock(m_cIM, m_cOM);

    if ((m_iISPS != m_iOSPS) || (m_eIAF != m_eOAF) || (m_eISL != m_eOSL)) {
        if (m_tcSP == nullptr) {
            resample_info dst = { m_iOSPS, m_eOAF, m_eOSL };
            resample_info src = { m_iISPS, m_eIAF, m_eISL };
            m_tcSP = std::shared_ptr<audio_resampler_t>(audio_resampler_create(&dst, &src), audio_resampler_destroy);
            if (m_tcSP == nullptr) {
#if 0
                blog(LOG_ERROR, "%s (%d) : m_tcSP : %d", __FILE__, __LINE__, m_tcSP);
#endif // 0

                return false;
            }   
        }
    }

    // m_tcIVV -> input

    if (m_tcIVV.size() == 0) {
#if 0
        blog(LOG_ERROR, "%s (%d) : ERROR", __FILE__, __LINE__);
#endif // 0

        return false;
    }

    if ((m_tcIVV[0].size() % m_iMinIS) == 0) {
        uint8_t* output[MAX_AV_PLANES] = {};
        uint32_t out_frames;
        uint64_t ts_offset;

        if (m_tcIVV.size() > MAX_AV_PLANES) {
            blog(LOG_ERROR, "%s (%d) : m_tcIVV.size() : %d", __FILE__, __LINE__, m_tcIVV.size());
            return false;
        }

        uint8_t* input[MAX_AV_PLANES] = {};
        for (int i = 0; i < m_tcIVV.size(); ++i) {
            input[i] = m_tcIVV[i].data();
            if (input[i] == nullptr) {
                blog(LOG_ERROR, "%s (%d) : input[%d] : %d", __FILE__, __LINE__, i, input[i]);
                return false;
            }
        }

        uint32_t in_frames = m_tcIVV[0].size() / get_audio_size(m_eIAF, m_eISL, 1);
#if 0
        FILE* pFile = _fsopen("D:\\temp\\test.pcm", "ab", _SH_DENYNO);
        if (pFile == nullptr) {
            blog(LOG_ERROR, "%s (%d) : pFile : %d", __FILE__, __LINE__, pFile);
            return false;
        }

        if (m_tcIVV.empty() == true) {
            blog(LOG_ERROR, "%s (%d) : ERROR", __FILE__, __LINE__);
            return false;
        }

        if (m_tcIVV[0].data() == nullptr) {
            blog(LOG_ERROR, "%s (%d) : tcVV[0].data() : %d", __FILE__, __LINE__, tcVV[0].data());
            return false;
        }

        if (fwrite(m_tcIVV[0].data(), 1, m_tcIVV[0].size(), pFile) != m_tcIVV[0].size())
            blog(LOG_ERROR, "%s (%d) : ERROR", __FILE__, __LINE__);

        if (fclose(pFile))
            blog(LOG_ERROR, "%s (%d) : ERROR", __FILE__, __LINE__);
#endif
        if (audio_resampler_resample(m_tcSP.get(), output, &out_frames, &ts_offset, input, in_frames) == false) {
            blog(LOG_ERROR, "%s (%d) : ERROR", __FILE__, __LINE__);
            return false;
        }
#if 0
        if (output[0] == nullptr) {
            blog(LOG_ERROR, "%s (%d) : output[0] : %d", __FILE__, __LINE__, output[0]);
            return false;
        }

        FILE* pFile = _fsopen("D:\\temp\\test.pcm", "ab", _SH_DENYNO);
        if (pFile == nullptr) {
            blog(LOG_ERROR, "%s (%d) : pFile : %d", __FILE__, __LINE__, pFile);
            return false;
        }
    
        if (fwrite(output[0], get_audio_size(m_eOAF, m_eOSL, 1), out_frames, pFile) != out_frames)
            blog(LOG_ERROR, "%s (%d) : ERROR", __FILE__, __LINE__);

        if (fclose(pFile))
            blog(LOG_ERROR, "%s (%d) : ERROR", __FILE__, __LINE__);
#endif
        for (auto& tcIV : m_tcIVV) tcIV.clear();

        //blog(LOG_INFO, "%d -> %d, %I64d, %d, %d", in_frames, out_frames, ts_offset, m_tcIVV[0].size(), m_tcOVV[0].size());

        // output -> m_tcOVV

        if (m_tcOVV.size() > MAX_AV_PLANES) {
            blog(LOG_ERROR, "%s (%d) : m_tcOVV.size() : %d", __FILE__, __LINE__, m_tcOVV.size());
            return false;
        }

        if (m_tcOVV[0].size() > m_iLate) {
#if 0
            blog(LOG_ERROR, "%s (%d) : LATE : %d > %d", __FILE__, __LINE__, m_tcOVV[0].size(), m_iLate);
#endif // 0

            for (auto& tcOV : m_tcOVV) tcOV.clear();
        }
        
        for (int i = 0, j = get_audio_size(m_eOAF, m_eOSL, out_frames); i < m_tcOVV.size(); ++i) {
            m_tcOVV[i].reserve(m_tcOVV[i].size() + j);

            if (output[i] == nullptr) {
                blog(LOG_ERROR, "%s (%d) : output[%d] : %d", __FILE__, __LINE__, i, output[i]);
                return false;
            }

            m_tcOVV[i].insert(m_tcOVV[i].end(), output[i], output[i] + j);
        }
    }

    //

    return true;
}

void SOOPAudioResample::_Clear() {
    std::scoped_lock tcLock(m_cIM, m_cOM);
    for (auto& tcOV : m_tcOVV) tcOV.clear();
    for (auto& tcIV : m_tcIVV) tcIV.clear();
}

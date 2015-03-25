/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2012-2013 Intel Corporation. All Rights Reserved.

*******************************************************************************/
#include "util_cmdline.h"
#include "pxccapture.h"

UtilCmdLine::UtilCmdLine(PXCSession *session, pxcUID iuid) {
    m_session=session;
    m_iuid=iuid;
    m_nframes=50000;
    m_sdname=0;
    m_nchannels=0;
    m_sampleRate=0;
    m_volume=0;
    m_ttstext = 0;
    m_language = 0;
    m_recordedFile = 0;
	m_meshFormat = 0;
    m_realtime = -1; // not set
    m_bRecord = false;
    m_traceFile = 0;
    m_eos = 0;
	m_bFace = false;
	m_bGesture = false;
	m_bVoice = false;
	m_bNoRender = false;
	m_bMirror = false;
	m_bObject = false;
	m_bSolid = false;
    m_outFile = 0;
    m_bFace = false;
    m_bHead = false;
    m_bBody = false;
}

bool UtilCmdLine::Parse(const pxcCHAR *options, int argc, pxcCHAR *argv[]) {
    for (int i=1;i<argc;i++) {
        if (!argv[i]) continue; // skip options already processed
        if (i+1<argc) {
            if (!wcscmp(argv[i],L"-csize") && wcsstr(options,L"-csize")) {
				PXCSizeI32 size={0,0}; pxcI32 fps=0;
                if (swscanf_s(argv[++i],L"%dx%dx%d",&size.width,&size.height,&fps)>=2) 
                    m_csize.push_back(std::pair<PXCSizeI32,pxcI32>(size,fps));
                continue;
            }
            if (!wcscmp(argv[i],L"-dsize") && wcsstr(options,L"-dsize")) {
				PXCSizeI32 size={0,0}; pxcI32 fps=0;
                if (swscanf_s(argv[++i],L"%dx%dx%d",&size.width,&size.height,&fps)>=2)
                    m_dsize.push_back(std::pair<PXCSizeI32,pxcI32>(size,fps));
				
				if( m_isize.size() > 0 && (m_dsize.front().first.width != m_isize.front().first.width 
											||  m_dsize.front().first.width != m_isize.front().first.width 
											|| (pxcF32)m_dsize.front().second != (pxcF32)m_isize.front().second))
				{  
					wprintf_s(L"Error: dsize and isize must be identical\n");
					return false; 
				}
                continue;
            }
			if (!wcscmp(argv[i],L"-isize") && wcsstr(options,L"-isize")) {
				PXCSizeI32 size={0,0}; pxcI32 fps=0;
                if (swscanf_s(argv[++i],L"%dx%dx%d",&size.width,&size.height,&fps)>=2)
                    m_isize.push_back(std::pair<PXCSizeI32,pxcI32>(size,fps));

				if( m_dsize.size() > 0 && (m_dsize.front().first.width != m_isize.front().first.width 
											||  m_dsize.front().first.width != m_isize.front().first.width 
											|| (pxcF32)m_dsize.front().second != (pxcF32)m_isize.front().second))
				{  
					wprintf_s(L"Error: dsize and isize must be identical\n");
					return false; 
				}

                continue;
            }
            if (!wcscmp(argv[i],L"-load") && m_session) {
                m_session->LoadImplFromFile(argv[++i]);
                continue;
            }
            if ((!wcscmp(argv[i],L"-file") || !wcscmp(argv[i],L"-wavfile")) && wcsstr(options,L"-file")) {
                m_recordedFile=argv[++i];
                continue;
            }
            if (!wcscmp(argv[i],L"-format")) {
                m_meshFormat=argv[++i];
                continue;
            }
            if (!wcscmp(argv[i], L"-out")) {
                m_outFile = argv[++i];
                continue;
            }            
            if (!wcscmp(argv[i], L"-realtime") && wcsstr(options, L"-realtime")) {
                i++;
                if (!wcscmp(argv[i],L"on") || !wcscmp(argv[i],L"1")) m_realtime = 1;
                if (!wcscmp(argv[i],L"off") || !wcscmp(argv[i],L"0")) m_realtime = 0;
                continue;
            }
            if ((!wcscmp(argv[i],L"-sdname") || !wcscmp(argv[i],L"-mic")) && wcsstr(options,L"-sdname")) {
                m_sdname=argv[++i];
                continue;
            }
            if (!wcscmp(argv[i],L"-nframes") && wcsstr(options,L"-nframes")) {
                swscanf_s(argv[++i],L"%d",&m_nframes);
                continue;
            }
            if (!wcscmp(argv[i],L"-nchannels") && wcsstr(options,L"-nchannels")) {
                swscanf_s(argv[++i],L"%d",&m_nchannels);
                continue;
            }
            if (!wcscmp(argv[i],L"-smprate") && wcsstr(options,L"-smprate")) {
                swscanf_s(argv[++i],L"%d",&m_sampleRate);
                continue;
            }
            if (!wcscmp(argv[i],L"-grammar") && wcsstr(options,L"-grammar")) {
                pxcCHAR delims[] = L",";
                m_grammar.clear();
                for (pxcCHAR *tc, *tk=wcstok_s(argv[++i], delims, &tc);tk;tk=wcstok_s(NULL,delims,&tc))
                    m_grammar.push_back(tk);
                continue;
            }
            if (!wcscmp(argv[i],L"-text") && wcsstr(options,L"-text")) {
               m_ttstext =argv[++i];
               continue;
            }
            if (!wcscmp(argv[i],L"-language") && wcsstr(options,L"-language")) {
                i++;
                for (int j=3;j>=0;j--) 
                    m_language=(m_language<<8)+(pxcI32)argv[i][j];
                continue;
            }
            if (!wcscmp(argv[i],L"-volume") && wcsstr(options,L"-volume")) {
                swscanf_s(argv[++i],L"%g",&m_volume);
                continue;
            }
            if (!wcscmp(argv[i],L"-iuid")) {
                i++;
                for (int j=3;j>=0;j--) 
                    m_iuid=(m_iuid<<8)+(pxcI32)argv[i][j];
                continue;
            }
            if (!wcscmp(argv[i],L"-eos") && wcsstr(options,L"-eos")) {
                swscanf_s(argv[++i],L"%d",&m_eos);
                continue;
            }
            if (!wcscmp(argv[i],L"-trace") && wcsstr(options,L"-trace")) {
                m_traceFile=argv[++i];
                continue;
            }
        }
        if (!wcscmp(argv[i],L"-record") && wcsstr(options,L"-record")) {
            m_bRecord=true;
            continue;
        }
        if (!wcscmp(argv[i],L"-face") && wcsstr(options,L"-face")) {
            m_bFace=true;
            continue;
        }
        if (!wcscmp(argv[i], L"-head") && wcsstr(options, L"-head")) {
            m_bHead = true;
            continue;
        }
        if (!wcscmp(argv[i], L"-body") && wcsstr(options, L"-body")) {
            m_bBody = true;
            continue;
        }
        if (!wcscmp(argv[i], L"-gesture") && wcsstr(options, L"-gesture")) {
            m_bGesture=true;
            continue;
        }
        if (!wcscmp(argv[i],L"-voice") && wcsstr(options,L"-voice")) {
            m_bVoice=true;
            continue;
        }
		if (!wcscmp(argv[i],L"-noRender") && wcsstr(options,L"-noRender")) {
            m_bNoRender=true;
            continue;
        }
		if (!wcscmp(argv[i],L"-mirror") && wcsstr(options,L"-mirror")) {
            m_bMirror=true;
            continue;
        }
		if (!wcscmp(argv[i], L"-object") && wcsstr(options, L"-object")) {
			m_bObject = true;
			continue;
		}
		if (!wcscmp(argv[i], L"-solid") && wcsstr(options, L"-solid")) {
			m_bSolid = true;
			continue;
		}
        if (!wcscmp(argv[i],L"-listio") && m_session) {
            PXCSession::ImplDesc desc, desc2;
            memset(&desc,0,sizeof(desc));
            desc.group=PXCSession::IMPL_GROUP_SENSOR;
            desc.subgroup=PXCSession::IMPL_SUBGROUP_VIDEO_CAPTURE|PXCSession::IMPL_SUBGROUP_AUDIO_CAPTURE;
            for (int i=0;;i++) {
                if (m_session->QueryImpl(&desc,i,&desc2)<PXC_STATUS_NO_ERROR) break;
                PXCCapture *capture = 0;
                if (m_session->CreateImpl<PXCCapture>(&desc2,&capture)<PXC_STATUS_NO_ERROR) continue;
                wprintf_s(L"Module: %s\n", desc2.friendlyName);
                for (int d=0;;d++) {
                    PXCCapture::DeviceInfo dinfo;
                    if (capture->QueryDeviceInfo(d,&dinfo)<PXC_STATUS_NO_ERROR) break;
                    wprintf_s(L"   device %d: %s\n", d, dinfo.name);
                }
                capture->Release();
            }
            return false;
        }
        wprintf_s(L"Usage: [options]\n");
        wprintf_s(L"-load module_dll      Load the specified module dll into the session\n");
        if (wcsstr(options,L"-iuid"))       wprintf_s(L"-iuid XYZT            Use the specified module for this sample\n");
        if (wcsstr(options,L"-csize"))      wprintf_s(L"-csize 640x480x30     Set the source device color resolution and frame rate\n");
        if (wcsstr(options,L"-dsize"))      wprintf_s(L"-dsize 320x240x60     Set the source device depth resolution and frame rate\n");
		if (wcsstr(options,L"-isize"))      wprintf_s(L"-isize 640x480x30     Set the source device IR resolution and frame rate\n");
        if (wcsstr(options,L"-file"))       wprintf_s(L"-file FILENAME        Specify a file name for playback from file or recording to file\n");
        if (wcsstr(options,L"-record"))     wprintf_s(L"-record               Use together with -file option to enable recording\n");
        if (wcsstr(options,L"-realtime"))   wprintf_s(L"-realtime [on|off]    Use together with -file option to enable/disable realtime mode of playback\n");
        if (wcsstr(options,L"-nframes"))    wprintf_s(L"-nframes 100          Record specific number of frames then exit\n");
        if (wcsstr(options,L"-sdname"))     wprintf_s(L"-sdname Integrated    Specify the source device by its partial name\n");
        if (wcsstr(options,L"-nchannels"))  wprintf_s(L"-nchannels 2          Specify the source device audio channel number\n");
        if (wcsstr(options,L"-smprate"))    wprintf_s(L"-smprate 44100        Specify the source device audio sample rate\n");
        if (wcsstr(options,L"-grammar"))    wprintf_s(L"-grammar \"left,right\" Specify the grammar words, use comma as delimitier\n");
        if (wcsstr(options,L"-eos"))        wprintf_s(L"-eos 200              Silence interval between phrases for voice recognition, milliseconds\n");
        if (wcsstr(options,L"-volume"))     wprintf_s(L"-volume 0.2           Specify audio volume (0-1) \n");
        if (wcsstr(options,L"-text"))       wprintf_s(L"-text \"Text\"          Specify text to render using TTS engine\n");
        if (wcsstr(options,L"-trace"))      wprintf_s(L"-trace FILENAME       Specify the file name for trace data\n");
        if (wcsstr(options,L"-listio"))     wprintf_s(L"-listio               List all I/O devices\n");
		if (wcsstr(options,L"-face"))		wprintf_s(L"-face                 Enable face analysis stream\n");
		if (wcsstr(options,L"-gesture"))	wprintf_s(L"-gesture              Enable gesture detection stream\n");
		if (wcsstr(options,L"-voice"))		wprintf_s(L"-voice                Enable voice recognition stream\n");
		if (wcsstr(options,L"-language"))	wprintf_s(L"-language enUS        Set the language ID\n");
		if (wcsstr(options,L"-noRender"))	wprintf_s(L"-noRender             Disable rendering of all streams\n");
		if (wcsstr(options,L"-mirror"))		wprintf_s(L"-mirror               Enable MIRROR_MODE_HORIZONTAL\n");
		if (wcsstr(options,L"-object"))		wprintf_s(L"-object               Enable object on planar surface detection\n");
		if (wcsstr(options,L"-solid"))		wprintf_s(L"-solid                Enable solidification\n");
		if (wcsstr(options,L"-format"))		wprintf_s(L"-format [OBJ|PLY|STL]   Choose output format. Default OBJ\n");
		wprintf_s(L"-help                 This help\n");
        return false;
    }
    if (m_bRecord && !m_recordedFile)
    {
        wprintf_s(L"-record option could be used only together with filename specified by -file option\n");
        return false;
    }
    return true;
}

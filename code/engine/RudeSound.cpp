/*
 *  RudeSound.cpp
 *
 *  Bork3D Game Engine
 *  Copyright (c) 2009 Bork 3D LLC. All rights reserved.
 *
 */

#include "RudeSound.h"

#include "RudeFile.h"
#include "RudeRegistry.h"

#define kListenerDistance			1.0

const char * kSoundFilenames[kNumSounds] = {
	"swing_wood.wav",
	"swing_wedge.wav",
	"swing_wedge_insand.wav",
	"swing_driver.wav",
	"swing_iron_soft.wav",
	"swing_putter.wav",
	"ball_in_hole.wav",
	"crowd_missedputt.wav",
	"sfx_cheer.wav",
	"sfx_claps.wav",
	"sfx_start.wav",
	"sfx_back.wav",
	"sfx_exit.wav",
	"sfx_select.wav",
	"sfx_clicklow.wav",
	"sfx_clickhi.wav",
};

const char * kSoundBGMs[kNumBGMs] = {
	"bgm_silence.m4a",
	"bgm_title.m4a",
	"bgm_putting.m4a"
};

#if defined(RUDE_AMIGAOS4)
#include <SDL2/SDL.h>
#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

#define MAX_ACTIVE_VOICES 8

struct ActiveVoice {
	bool active;
	const unsigned char *data;
	unsigned int length;
	unsigned int position;
};

static ActiveVoice g_voices[MAX_ACTIVE_VOICES];

static void SDLCALL RudeAudioCallback(void *userdata, Uint8 *stream, int len)
{
	RudeSound *sound = static_cast<RudeSound *>(userdata);
	memset(stream, 0, len);

	if(!sound)
		return;

	// 1. Stream background music if playing and music is enabled
	if(sound->GetMusicOn() && sound->m_bgmPlaying && sound->m_bgmDecoder)
	{
		drmp3 *mp3 = static_cast<drmp3 *>(sound->m_bgmDecoder);
		// Output stream format is 16-bit stereo (4 bytes per frame)
		int bytesPerFrame = 4;
		int framesNeeded = len / bytesPerFrame;
		int framesReadTotal = 0;

		drmp3_int16 pcmBuffer[2048]; // up to 1024 stereo frames
		while(framesNeeded > 0)
		{
			int chunk = framesNeeded;
			if(chunk > 1024) chunk = 1024;

			drmp3_uint64 read = drmp3_read_pcm_frames_s16(mp3, chunk, pcmBuffer);
			if(read == 0)
			{
				// Loop music back to beginning
				drmp3_seek_to_pcm_frame(mp3, 0);
				read = drmp3_read_pcm_frames_s16(mp3, chunk, pcmBuffer);
				if(read == 0)
					break;
			}

			int bytesRead = static_cast<int>(read) * bytesPerFrame;
			int destOffset = framesReadTotal * bytesPerFrame;
			int volume = static_cast<int>(sound->m_bgmVol * SDL_MIX_MAXVOLUME);
			if(volume > SDL_MIX_MAXVOLUME) volume = SDL_MIX_MAXVOLUME;
			if(volume < 0) volume = 0;

			SDL_MixAudioFormat(stream + destOffset, reinterpret_cast<const Uint8 *>(pcmBuffer),
			                   sound->m_obtainedFormat, bytesRead, volume);

			framesReadTotal += static_cast<int>(read);
			framesNeeded -= static_cast<int>(read);
		}
	}

	// 2. Mix active sound effects if sound is enabled
	if(sound->SoundOn())
	{
		for(int i = 0; i < MAX_ACTIVE_VOICES; ++i)
		{
			if(!g_voices[i].active)
				continue;

			unsigned int remaining = g_voices[i].length - g_voices[i].position;
			unsigned int toMix = static_cast<unsigned int>(len);
			if(toMix > remaining)
				toMix = remaining;

			SDL_MixAudioFormat(stream, g_voices[i].data + g_voices[i].position,
			                   sound->m_obtainedFormat, toMix, SDL_MIX_MAXVOLUME);

			g_voices[i].position += toMix;
			if(g_voices[i].position >= g_voices[i].length)
			{
				g_voices[i].active = false;
			}
		}
	}
}
#endif

RudeSound::RudeSound()
{
	m_soundon = true;
	m_musicOn = true;
	
	m_curBGM = kBGMNone;
	
	m_bgmVolFadeEnabled = false;
	m_bgmVol = 1.0;
	m_bgmVolFade = 0.0;

#if defined(RUDE_IPHONE) || defined(RUDE_MACOS)
	SoundEngine_Initialize(44100);
	SoundEngine_SetListenerPosition(0.0, 0.0, kListenerDistance);
#endif

#if defined(RUDE_AMIGAOS4)
	m_audioDevice = 0;
	m_bgmDecoder = NULL;
	m_bgmPlaying = false;
	m_obtainedFreq = 44100;
	m_obtainedFormat = AUDIO_S16SYS;
	m_obtainedChannels = 2;

	for(int i = 0; i < kNumSounds; ++i)
	{
		m_soundids[i] = -1;
		m_sfxBuffers[i] = NULL;
		m_sfxLengths[i] = 0;
	}
	for(int i = 0; i < MAX_ACTIVE_VOICES; ++i)
	{
		g_voices[i].active = false;
		g_voices[i].data = NULL;
		g_voices[i].length = 0;
		g_voices[i].position = 0;
	}

	SDL_AudioSpec desired, obtained;
	memset(&desired, 0, sizeof(desired));
	desired.freq = 44100;
	desired.format = AUDIO_S16SYS;
	desired.channels = 2;
	desired.samples = 2048;
	desired.callback = RudeAudioCallback;
	desired.userdata = this;

	m_audioDevice = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, 0);
	if(m_audioDevice > 0)
	{
		m_obtainedFreq = obtained.freq;
		m_obtainedFormat = obtained.format;
		m_obtainedChannels = obtained.channels;
		SDL_PauseAudioDevice(m_audioDevice, 0);
		RUDE_REPORT("RudeSound: SDL audio initialized: freq=%d, format=0x%x, channels=%d\n",
		            m_obtainedFreq, m_obtainedFormat, m_obtainedChannels);
	}
	else
	{
		RUDE_REPORT("RudeSound: SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
	}
#endif

	for(int i = 0; i < kNumSounds; i++)
	{
		m_soundids[i] = -1;

		LoadWave(kSoundFilenames[i], (eSoundEffect) i);
	}
	
	RudeRegistry *reg = RudeRegistry::GetSingleton();
	
	int loadsize = sizeof(m_musicOn);
	if(reg->QueryByte("GOLF", "RS_MUSIC", &m_musicOn, &loadsize) == 0)
	{
		
	}
	else
	{
		m_musicOn = true;
	}
	
}

RudeSound::~RudeSound()
{
#if defined(RUDE_AMIGAOS4)
	StopSong();
	if(m_audioDevice > 0)
	{
		SDL_CloseAudioDevice(m_audioDevice);
		m_audioDevice = 0;
	}
	for(int i = 0; i < kNumSounds; ++i)
	{
		if(m_sfxBuffers[i])
		{
			free(m_sfxBuffers[i]);
			m_sfxBuffers[i] = NULL;
		}
	}
#endif
}

void RudeSound::Shutdown()
{

}

RudeSound *soundinstance = 0;

RudeSound * RudeSound::GetInstance()
{
	if(soundinstance == 0)
		soundinstance = new RudeSound();
	return soundinstance;
}

void RudeSound::BgmVolFade(float amount)
{
	m_bgmVolFade = amount;
	m_bgmVolFadeEnabled = true;
}

void RudeSound::BgmVol(float vol)
{
	m_bgmVol = vol;
	m_bgmVolFade = 0.0f;
	
#if defined(RUDE_IPHONE) || defined(RUDE_MACOS)
	if(m_curBGM != kBGMNone)
		SoundEngine_SetBackgroundMusicVolume(m_bgmVol);
#endif
}

void RudeSound::PlaySong(eSoundBGM num)
{
	RUDE_REPORT("RudeSound::PlaySong %d\n", num);
	
	if(num == m_curBGM)
	{
		RUDE_REPORT("BGM already playing\n");
		return;
	}
	
	if(m_curBGM != kBGMNone)
	{
		RUDE_REPORT("Unloading previous BGM\n");

#if defined(RUDE_IPHONE) || defined(RUDE_MACOS)
		SoundEngine_UnloadBackgroundMusicTrack();
#endif
#if defined(RUDE_AMIGAOS4)
		StopSong();
#endif
	}
	
	if(m_musicOn && num != kBGMNone && num != kBGMSilence)
	{
		m_curBGM = num;
		
		char buffer[512];
		RudeFileGetFile(kSoundBGMs[m_curBGM], buffer, 512);
				
#if defined(RUDE_IPHONE) || defined(RUDE_MACOS)
		SoundEngine_LoadBackgroundMusicTrack(buffer, false, false);
		SoundEngine_StartBackgroundMusic();
#endif
#if defined(RUDE_AMIGAOS4)
		if(m_audioDevice > 0)
		{
			char mp3filename[64];
			snprintf(mp3filename, sizeof(mp3filename), "%s", kSoundBGMs[m_curBGM]);
			char *ext = strrchr(mp3filename, '.');
			if(ext)
				strcpy(ext, ".mp3");

			char mp3path[512];
			if(RudeFileGetFile(mp3filename, mp3path, sizeof(mp3path), true))
			{
				SDL_LockAudioDevice(m_audioDevice);
				if(m_bgmDecoder)
				{
					drmp3 *oldmp3 = static_cast<drmp3 *>(m_bgmDecoder);
					drmp3_uninit(oldmp3);
					delete oldmp3;
					m_bgmDecoder = NULL;
				}
				drmp3 *mp3 = new drmp3();
				if(drmp3_init_file(mp3, mp3path, NULL))
				{
					m_bgmDecoder = mp3;
					m_bgmPlaying = true;
					RUDE_REPORT("RudeSound: playing MP3 %s (channels=%d, freq=%d)\n",
					            mp3path, mp3->channels, mp3->sampleRate);
				}
				else
				{
					delete mp3;
					m_bgmDecoder = NULL;
					m_bgmPlaying = false;
					RUDE_REPORT("RudeSound: failed to load MP3 %s\n", mp3path);
				}
				SDL_UnlockAudioDevice(m_audioDevice);
			}
		}
#endif
	}
	else
	{
		m_curBGM = num;
	}
}

void RudeSound::StopSong()
{
	if(m_curBGM == kBGMNone)
		return;
	
	RUDE_REPORT("RudeSound::StopSong %d\n", m_curBGM);
	
	m_curBGM = kBGMNone;

#if defined(RUDE_IPHONE) || defined(RUDE_MACOS)
	SoundEngine_UnloadBackgroundMusicTrack();
#endif

#if defined(RUDE_AMIGAOS4)
	if(m_audioDevice > 0)
	{
		SDL_LockAudioDevice(m_audioDevice);
		m_bgmPlaying = false;
		if(m_bgmDecoder)
		{
			drmp3 *mp3 = static_cast<drmp3 *>(m_bgmDecoder);
			drmp3_uninit(mp3);
			delete mp3;
			m_bgmDecoder = NULL;
		}
		SDL_UnlockAudioDevice(m_audioDevice);
	}
#endif
}

bool RudeSound::ToggleMusic()
{
	m_musicOn = !m_musicOn;
	
	if(!m_musicOn)
	{
		StopSong();
	}
	
	RudeRegistry *reg = RudeRegistry::GetSingleton();
	reg->SetByte("GOLF", "RS_MUSIC", &m_musicOn, sizeof(m_musicOn));
	
	return m_musicOn;
}

void RudeSound::PlayWave(eSoundEffect num)
{
	if(num == kSoundNone)
		return;
	
#if 0
	int result = SoundEngine_StartEffect(m_soundids[num]);
	//RUDE_ASSERT(result == noErr, "Could not play effect (result = %d)\n", result);
#endif
	
#if defined(RUDE_IPHONE) || defined(RUDE_MACOS)
	AudioServicesPlaySystemSound(m_soundids[num]);
#endif

#if defined(RUDE_AMIGAOS4)
	if(m_soundon && m_audioDevice > 0 && num < kNumSounds && m_sfxBuffers[num] && m_sfxLengths[num] > 0)
	{
		SDL_LockAudioDevice(m_audioDevice);
		// Find inactive voice, or oldest active voice
		int voiceSlot = -1;
		for(int i = 0; i < MAX_ACTIVE_VOICES; ++i)
		{
			if(!g_voices[i].active)
			{
				voiceSlot = i;
				break;
			}
		}
		if(voiceSlot < 0) voiceSlot = 0; // fallback steal slot 0

		g_voices[voiceSlot].data = m_sfxBuffers[num];
		g_voices[voiceSlot].length = m_sfxLengths[num];
		g_voices[voiceSlot].position = 0;
		g_voices[voiceSlot].active = true;
		SDL_UnlockAudioDevice(m_audioDevice);
	}
#endif
}


void RudeSound::LoadWave(const char *sound, eSoundEffect num)
{
	RUDE_ASSERT(num < kNumSounds, "Invalid sound num");
	RUDE_ASSERT(sound, "No sound name");
	RUDE_REPORT("RudeSound::LoadWave %s\n", sound);
	
#if 0
	char buffer[512];
	RudeFileGetFile(sound, buffer, 512);

	int result = SoundEngine_LoadEffect(buffer, &m_soundids[num]);
	//RUDE_ASSERT(result == noErr, "Could not load effect (result = %d)\n", result);
#endif
	
#if defined(RUDE_IPHONE) || defined(RUDE_MACOS)
	CFBundleRef bundle = CFBundleGetMainBundle();
	CFStringRef file = CFStringCreateWithCString(0, sound, kCFStringEncodingASCII);
	CFURLRef myURLRef = CFBundleCopyResourceURL(bundle, file, 0, 0);
	
	OSStatus error = AudioServicesCreateSystemSoundID(myURLRef, &m_soundids[num]);
	RUDE_ASSERT(error == kAudioServicesNoError, "Could not load sound %s", sound);
#endif

#if defined(RUDE_AMIGAOS4)
	if(m_audioDevice > 0)
	{
		char filepath[512];
		if(RudeFileGetFile(sound, filepath, sizeof(filepath), true))
		{
			SDL_AudioSpec wavSpec;
			Uint8 *wavBuf = NULL;
			Uint32 wavLen = 0;
			if(SDL_LoadWAV(filepath, &wavSpec, &wavBuf, &wavLen) != NULL)
			{
				SDL_AudioCVT cvt;
				if(SDL_BuildAudioCVT(&cvt, wavSpec.format, wavSpec.channels, wavSpec.freq,
				                     m_obtainedFormat, m_obtainedChannels, m_obtainedFreq) >= 0)
				{
					cvt.buf = static_cast<Uint8 *>(malloc(wavLen * (cvt.len_mult > 0 ? cvt.len_mult : 1)));
					if(cvt.buf)
					{
						memcpy(cvt.buf, wavBuf, wavLen);
						cvt.len = wavLen;
						SDL_ConvertAudio(&cvt);
						m_sfxBuffers[num] = cvt.buf;
						m_sfxLengths[num] = cvt.len_cvt;
						m_soundids[num] = num;
					}
				}
				SDL_FreeWAV(wavBuf);
			}
			else
			{
				RUDE_REPORT("RudeSound: failed to load WAV %s: %s\n", filepath, SDL_GetError());
			}
		}
	}
#endif
}

void RudeSound::Tick(float delta)
{
	if(m_bgmVolFadeEnabled && m_curBGM != kBGMNone)
	{
		m_bgmVol += m_bgmVolFade * delta;
		
		if(m_bgmVol > 1.0f)
		{
			m_bgmVol = 1.0f;
			m_bgmVolFade = 0.0f;
			m_bgmVolFadeEnabled = false;
		}
		else if(m_bgmVol < 0.0f)
		{
			m_bgmVol = 0.0f;
			m_bgmVolFade = 0.0f;
			m_bgmVolFadeEnabled = false;
			
			StopSong();
			return;
		}
		
#if defined(RUDE_IPHONE) || defined(RUDE_MACOS)
		SoundEngine_SetBackgroundMusicVolume(m_bgmVol);
#endif
		
	}
}

void RudeSound::ToggleSound()
{
	if(m_soundon)
	{
		m_soundon = false;
	}
	else
	{
		m_soundon = true;
	}

}

void RudeSound::Pause()
{

}

void RudeSound::Unpause()
{

}
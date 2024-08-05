// !{! DirectSoundHelper 선언 코드. 원본 코드의 위치는 DirectSoundPlay/DirectSoundHelper.h.
#pragma once

#include <dsound.h>  //library "dsound.lib"
#pragma comment (lib, "dsound.lib")
#include <mmsystem.h>
#pragma comment (lib, "winmm.lib") //for many mmio* functions
#pragma comment (lib, "dxguid.lib") //for several IID_IDirectSound* symbols
#include <vector>


//--------------------------------------------------------------------------------------
// 도움 매크로
//--------------------------------------------------------------------------------------
#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#endif
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#endif
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#endif

//--------------------------------------------------------------------------------------
// 디버깅 편의용 윈도우 함수
//--------------------------------------------------------------------------------------
#if defined(_DEBUG) && defined(_WINDOWS) && !defined(_AFX) && !defined(_AFXDLL) && !defined(TRACE)
#define TRACE	TRACE_WIN
HRESULT TRACE_WIN(LPCTSTR lpszFormat, ...);
HRESULT TRACE_WIN(HRESULT hr, LPCTSTR lpszFormat, ...);
#define TRACE_WIN_REQUIRE_IMPL
#endif



//-----------------------------------------------------------------------------
// Classes used by this header
//-----------------------------------------------------------------------------
class CSoundManager;
class CSound;
class CStreamingSound;
class CWaveFile;


//-----------------------------------------------------------------------------
// Typing macros 
//-----------------------------------------------------------------------------
#define DXUT_StopSound(s)         { if(s) s->Stop(); }
#define DXUT_PlaySound(s)         { if(s) s->Play( 0, 0 ); }
#define DXUT_PlaySoundLooping(s)  { if(s) s->Play( 0, DSBPLAY_LOOPING ); }


//-----------------------------------------------------------------------------
// Name: class CSoundManager
// Desc: 
//-----------------------------------------------------------------------------
class CSoundManager
{
protected:
	IDirectSound8* m_pDS;
private:
	std::vector <CSound*> m_soundVector;

public:
	CSoundManager();
	~CSoundManager();

	HRESULT init(HWND hWnd);
	void release();
	void stop(int id);
	HRESULT play(int id, bool bLooped);
	HRESULT add(LPTSTR filename,int *id);

protected:
	HRESULT Initialize( HWND hWnd, DWORD dwCoopLevel );
	inline IDirectSound8* GetDirectSound() { return m_pDS; }
	HRESULT SetPrimaryBufferFormat( DWORD dwPrimaryChannels, DWORD dwPrimaryFreq, DWORD dwPrimaryBitRate );
	HRESULT Get3DListenerInterface( IDirectSound3DListener** ppDSListener );

	HRESULT Create( CSound** ppSound, LPTSTR strWaveFileName, DWORD dwCreationFlags = 0, GUID guid3DAlgorithm = GUID_NULL, DWORD dwNumBuffers = 1 );
	HRESULT CreateFromMemory( CSound** ppSound, BYTE* pbData, ULONG ulDataSize, LPWAVEFORMATEX pwfx, DWORD dwCreationFlags = 0, GUID guid3DAlgorithm = GUID_NULL, DWORD dwNumBuffers = 1 );
	HRESULT CreateStreaming( CStreamingSound** ppStreamingSound, LPTSTR strWaveFileName, DWORD dwCreationFlags, GUID guid3DAlgorithm, DWORD dwNotifyCount, DWORD dwNotifySize, HANDLE hNotifyEvent );
};


//-----------------------------------------------------------------------------
// Name: class CSound
// Desc: Encapsulates functionality of a DirectSound buffer.
//-----------------------------------------------------------------------------
class CSound
{
protected:
	IDirectSoundBuffer** m_apDSBuffer;
	DWORD                m_dwDSBufferSize;
	CWaveFile*           m_pWaveFile;
	DWORD                m_dwNumBuffers;
	DWORD                m_dwCreationFlags;

	HRESULT RestoreBuffer( IDirectSoundBuffer* pDSB, BOOL* pbWasRestored );

public:
	CSound( IDirectSoundBuffer** apDSBuffer, DWORD dwDSBufferSize, DWORD dwNumBuffers, CWaveFile* pWaveFile, DWORD dwCreationFlags );
	virtual ~CSound();

	HRESULT Get3DBufferInterface( DWORD dwIndex, IDirectSound3DBuffer** ppDS3DBuffer );
	HRESULT FillBufferWithSound( IDirectSoundBuffer* pDSB, BOOL bRepeatWavIfBufferLarger );
	IDirectSoundBuffer* GetFreeBuffer();
	IDirectSoundBuffer* GetBuffer( DWORD dwIndex );

	HRESULT Play( DWORD dwPriority = 0, DWORD dwFlags = 0, LONG lVolume = 0, LONG lFrequency = -1, LONG lPan = 0 );
	HRESULT Play3D( LPDS3DBUFFER p3DBuffer, DWORD dwPriority = 0, DWORD dwFlags = 0, LONG lFrequency = 0 );
	HRESULT Stop();
	HRESULT Reset();
	BOOL    IsSoundPlaying();
};


//-----------------------------------------------------------------------------
// Name: class CStreamingSound
// Desc: Encapsulates functionality to play a wave file with DirectSound.  
//       The Create() method loads a chunk of wave file into the buffer, 
//       and as sound plays more is written to the buffer by calling 
//       HandleWaveStreamNotification() whenever hNotifyEvent is signaled.
//-----------------------------------------------------------------------------
class CStreamingSound : public CSound
{
protected:
	DWORD m_dwLastPlayPos;
	DWORD m_dwPlayProgress;
	DWORD m_dwNotifySize;
	DWORD m_dwNextWriteOffset;
	BOOL  m_bFillNextNotificationWithSilence;

public:
	CStreamingSound( IDirectSoundBuffer* pDSBuffer, DWORD dwDSBufferSize, CWaveFile* pWaveFile, DWORD dwNotifySize );
	~CStreamingSound();

	HRESULT HandleWaveStreamNotification( BOOL bLoopedPlay );
	HRESULT Reset();
};



//-----------------------------------------------------------------------------
// class CWaveFile -- from here to the end of the file
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Typing macros 
//-----------------------------------------------------------------------------
#define WAVEFILE_READ   1
#define WAVEFILE_WRITE  2

//-----------------------------------------------------------------------------
// Name: class CWaveFile
// Desc: Encapsulates reading or writing sound data to or from a wave file
//-----------------------------------------------------------------------------
class CWaveFile
{
public:
	WAVEFORMATEX* m_pwfx;        // Pointer to WAVEFORMATEX structure
	HMMIO         m_hmmio;       // MM I/O handle for the WAVE
	MMCKINFO      m_ck;          // Multimedia RIFF chunk
	MMCKINFO      m_ckRiff;      // Use in opening a WAVE file
	DWORD         m_dwSize;      // The size of the wave file
	MMIOINFO      m_mmioinfoOut;
	DWORD         m_dwFlags;
	BOOL          m_bIsReadingFromMemory;
	BYTE*         m_pbData;
	BYTE*         m_pbDataCur;
	ULONG         m_ulDataSize;
	CHAR*         m_pResourceBuffer;

protected:
	HRESULT ReadMMIO();
	HRESULT WriteMMIO( WAVEFORMATEX *pwfxDest );

public:
	CWaveFile();
	~CWaveFile();

	HRESULT Open(LPTSTR strFileName, WAVEFORMATEX* pwfx, DWORD dwFlags );
	HRESULT OpenFromMemory( BYTE* pbData, ULONG ulDataSize, WAVEFORMATEX* pwfx, DWORD dwFlags );
	HRESULT Close();

	HRESULT Read( BYTE* pBuffer, DWORD dwSizeToRead, DWORD* pdwSizeRead );
	HRESULT Write( UINT nSizeToWrite, BYTE* pbData, UINT* pnSizeWrote );

	DWORD   GetSize();
	HRESULT ResetFile();
	WAVEFORMATEX* GetFormat() { return m_pwfx; };
};

// !}!
#ifdef _WIN32
#include <xt/Win32.hpp>
#include <vector>
#include <cstring>
#include <algorithm>

// ---- local ----

static XtBlockingStreamState ReadWin32BlockingStreamState(
  XtwWin32BlockingStream* stream) {

  XtBlockingStreamState result;
  EnterCriticalSection(&stream->lock.cs);
  result = stream->state;
  LeaveCriticalSection(&stream->lock.cs);
  return result;
}

static void ReceiveWin32BlockingStreamControl(
  XtwWin32BlockingStream* stream, XtBlockingStreamState state) {

  EnterCriticalSection(&stream->lock.cs);
  stream->state = state;
  XT_ASSERT(SetEvent(stream->respondEvent.event));
  LeaveCriticalSection(&stream->lock.cs);
}

static void SendWin32BlockingStreamControl(
  XtwWin32BlockingStream* stream, XtBlockingStreamState from, XtBlockingStreamState to) {

  EnterCriticalSection(&stream->lock.cs);
  if(stream->state == to) {
    LeaveCriticalSection(&stream->lock.cs);
    return;
  }
  stream->state = from;
  XT_ASSERT(SetEvent(stream->controlEvent.event));
  LeaveCriticalSection(&stream->lock.cs);
  while(ReadWin32BlockingStreamState(stream) != to)
    XT_ASSERT(WaitForSingleObject(stream->respondEvent.event, XT_WAIT_TIMEOUT_MS) == WAIT_OBJECT_0);
}

static DWORD WINAPI OnWin32BlockingBuffer(void* user) {
  XtBlockingStreamState state;
  auto stream = static_cast<XtwWin32BlockingStream*>(user);

  XT_ASSERT(SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)));
  while((state = ReadWin32BlockingStreamState(stream)) != XtBlockingStreamState::Closed) {
    switch(state) {
    case XtBlockingStreamState::Started:
      stream->ProcessBuffer(false);
      break;
    case XtBlockingStreamState::Closing:
      ReceiveWin32BlockingStreamControl(stream, XtBlockingStreamState::Closed);
      CoUninitialize();
      return S_OK;
    case XtBlockingStreamState::Stopping:
      stream->StopStream();
      ReceiveWin32BlockingStreamControl(stream, XtBlockingStreamState::Stopped);
      break;
    case XtBlockingStreamState::Starting:
      stream->ProcessBuffer(true);
      stream->StartStream();
      ReceiveWin32BlockingStreamControl(stream, XtBlockingStreamState::Started);
      break;
    case XtBlockingStreamState::Stopped:
      XT_ASSERT(WaitForSingleObject(stream->controlEvent.event, INFINITE) == WAIT_OBJECT_0);
      break;
    default:
      XT_FAIL("Unexpected stream state.");
      break;
    }
  }
  XT_FAIL("End of stream callback reached.");
  return S_OK;
}

// ---- win32 ----

const char* XtwWfxChannelNames[18] = {
  "Front Left", "Front Right", "Front Center",
  "Low Frequency", "Back Left", "Back Right",
  "Front Left Of Center", "Front Right Of Center", "Back Center", 
  "Side Left", "Side Right", "Top Center",
  "Top Front Left", "Top Front Center", "Top Front Right",
  "Top Back Left", "Top Back Center", "Top Back Right"
};

// ---- win32 ----

std::string XtwWideStringToUtf8(const wchar_t* wide) {
  int count;
  XT_ASSERT((count = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, 0)) > 0);
  std::string result(count - 1, '\0');
  XT_ASSERT(WideCharToMultiByte(CP_UTF8, 0, wide, -1, &result[0], count, nullptr, 0) > 0);
  return result;
}

bool XtwWfxToFormat(const WAVEFORMATEX& wfx, XtBool output, XtFormat& format) {  

  memset(&format, 0, sizeof(XtFormat));
  const WAVEFORMATEXTENSIBLE* wfxe = nullptr;
  if(wfx.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
    wfxe = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(&wfx);
  
  format.mix.rate = wfx.nSamplesPerSec;
  format.channels.inputs = output? 0: wfx.nChannels;
  format.channels.outputs = output? wfx.nChannels: 0;
  format.channels.inMask = output || wfxe == nullptr? 0: wfxe->dwChannelMask;
  format.channels.outMask = !output || wfxe == nullptr? 0: wfxe->dwChannelMask;

  if(wfxe != nullptr && wfx.wBitsPerSample != wfxe->Samples.wValidBitsPerSample)
    return false;
  if(wfxe != nullptr && wfxe->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
    format.mix.sample = XtSampleFloat32;
  else
    switch(wfx.wBitsPerSample) {
    case 8: format.mix.sample = XtSampleUInt8; break;
    case 16: format.mix.sample = XtSampleInt16; break;
    case 24: format.mix.sample = XtSampleInt24; break;
    case 32: format.mix.sample = XtSampleInt32; break;
    default: return false;
    }
  return true;
}

bool XtwFormatToWfx(const XtFormat& format, WAVEFORMATEXTENSIBLE& wfx) {  

  memset(&wfx, 0, sizeof(WAVEFORMATEXTENSIBLE));
  if(format.channels.inputs > 0 && format.channels.outputs > 0)
    return false;

  auto attributes = XtAudioGetSampleAttributes(format.mix.sample);
  wfx.Format.cbSize = 22;
  wfx.Format.nSamplesPerSec = format.mix.rate;
  wfx.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
  wfx.Format.nChannels = format.channels.inputs + format.channels.outputs;
  wfx.Format.wBitsPerSample = XtiGetSampleSize(format.mix.sample) * 8;
  wfx.Format.nBlockAlign = wfx.Format.wBitsPerSample / 8 * wfx.Format.nChannels;
  wfx.Format.nAvgBytesPerSec = wfx.Format.nBlockAlign * format.mix.rate;
  wfx.SubFormat = attributes.isFloat? KSDATAFORMAT_SUBTYPE_IEEE_FLOAT: KSDATAFORMAT_SUBTYPE_PCM;
  wfx.Samples.wValidBitsPerSample = wfx.Format.wBitsPerSample;
  wfx.dwChannelMask = static_cast<DWORD>(format.channels.inputs? format.channels.inMask: format.channels.outMask);
  if(wfx.dwChannelMask == 0)
    wfx.dwChannelMask = (1U << wfx.Format.nChannels) - 1;
  return true;
}

#endif // _WIN32
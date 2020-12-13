#if XT_ENABLE_DSOUND
#include <xt/api/private/Platform.hpp>
#include <xt/api/private/Service.hpp>
#include <xt/api/private/Device.hpp>
#include <xt/api/private/Stream.hpp>
#include <xt/private/BlockingStream.hpp>
#include <xt/private/Shared.hpp>
#include <xt/private/Win32.hpp>
#include <xt/private/Services.hpp>
#define INITGUID 1
#include <dsound.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <vector>
#include <memory>

// ---- local ----

static const int XtDsWakeUpsPerBuffer = 8;
static const double XtDsMinBufferMs = 100.0;
static const double XtDsMaxBufferMs = 5000.0;
static const double XtDsDefaultBufferMs = 500.0;

struct XtWaitableTimer
{
  HANDLE timer;
  XtWaitableTimer(XtWaitableTimer const&) = delete;
  XtWaitableTimer& operator=(XtWaitableTimer const&) = delete;
  ~XtWaitableTimer() { XT_ASSERT(CloseHandle(timer)); }
  XtWaitableTimer() { XT_ASSERT((timer = CreateWaitableTimer(nullptr, FALSE, nullptr)) != nullptr) ; }
};

// ---- forward ----

struct DSoundStream: public XtBlockingStream {
  const int32_t frameSize;
  std::vector<uint8_t> buffer;
  uint64_t xtBytesProcessed;
  uint64_t dsBytesProcessed;
  int32_t previousDsPosition;
  const int32_t bufferFrames;
  const XtWaitableTimer timer;
  const CComPtr<IDirectSound> output;
  const CComPtr<IDirectSoundCapture> input;
  const CComPtr<IDirectSoundBuffer> render;
  const CComPtr<IDirectSoundCaptureBuffer> capture;
  XT_IMPLEMENT_BLOCKING_STREAM(DSound);

  ~DSoundStream() {  }
  DSoundStream(bool secondary,
    CComPtr<IDirectSoundCapture> input, CComPtr<IDirectSound> output,
    CComPtr<IDirectSoundCaptureBuffer> capture, CComPtr<IDirectSoundBuffer> render, 
    int32_t bufferFrames, int32_t frameSize):
  XtBlockingStream(secondary), frameSize(frameSize),
  buffer(static_cast<size_t>(bufferFrames * frameSize), 0),
  xtBytesProcessed(0), dsBytesProcessed(0),
  previousDsPosition(0), bufferFrames(bufferFrames), timer(),
  output(output), input(input), render(render), capture(capture) {}
};

// ---- local ----

static DWORD WrapAround(int32_t bytes, int32_t bufferSize) {
  return bytes >= 0? bytes: bytes + bufferSize;
}

static bool InsideSafetyGap(DWORD read, DWORD write, DWORD lockPosition) {
  return read < write && read <= lockPosition && lockPosition < write ||
         read > write && (read <= lockPosition || lockPosition < write);
}

static UINT GetTimerPeriod(int32_t bufferFrames, int32_t rate) {
  return static_cast<UINT>(bufferFrames * 1000.0 / rate / XtDsWakeUpsPerBuffer);
}

static void SplitBufferParts(
  std::vector<uint8_t>& buffer, void* part1, DWORD size1, void* part2, DWORD size2) {

  memcpy(part1, &buffer[0], size1);
  if(size2 != 0)
    memcpy(part2, &buffer[size1], size2);
}

static void CombineBufferParts(
  std::vector<uint8_t>& buffer, void* part1, DWORD size1, void* part2, DWORD size2) {

  memcpy(&buffer[0], part1, size1);
  if(size2 != 0)
    memcpy(&buffer[size1], part2, size2);
}

// ---- service ----

// ---- device ----

XtFault DSoundDevice::ShowControlPanel() {
  return S_OK;
}

XtFault DSoundDevice::GetName(char* buffer, int32_t* size) const {
  XtiCopyString(this->name.c_str(), buffer, size);
  return S_OK;
}

XtFault DSoundDevice::SupportsAccess(XtBool interleaved, XtBool* supports) const {
  *supports = interleaved;
  return S_OK;
}

XtFault DSoundDevice::GetBufferSize(const XtFormat* format, XtBufferSize* size) const {
  size->min = XtDsMinBufferMs;
  size->max = XtDsMaxBufferMs;
  size->current = XtDsDefaultBufferMs;
  return S_OK;
}

XtFault DSoundDevice::SupportsFormat(const XtFormat* format, XtBool* supports) const {
  *supports = format->channels.inputs > 0 != !input 
           && format->channels.outputs > 0 != !output 
           && format->mix.rate >= 8000 
           && format->mix.rate <= 192000;
  return S_OK;
}

XtFault DSoundDevice::GetChannelName(XtBool output, int32_t index, char* buffer, int32_t* size) const {
  XtiCopyString(XtiWfxChannelNames[index], buffer, size);
  return S_OK;
}

XtFault DSoundDevice::GetChannelCount(XtBool output, int32_t* count) const {
  *count = (output != XtFalse) == !this->output? 0:  sizeof(XtiWfxChannelNames) / sizeof(const char*);
  return S_OK;
}

XtFault DSoundDevice::GetMix(XtBool* valid, XtMix* mix) const {

  UINT count;
  HRESULT hr;
  GUID deviceId;
  XtFormat format;
  CComPtr<IMMDeviceCollection> devices;
  CComPtr<IMMDeviceEnumerator> enumerator;

  GUID thisId = guid;
  if(thisId == GUID_NULL)
    if(!output)
      XT_VERIFY_COM(GetDeviceID(&DSDEVID_DefaultCapture, &thisId));
    else
      XT_VERIFY_COM(GetDeviceID(&DSDEVID_DefaultPlayback, &thisId));
  XT_VERIFY_COM(enumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator)));
  XT_VERIFY_COM(enumerator->EnumAudioEndpoints(eAll, DEVICE_STATE_ACTIVE, &devices));
  XT_VERIFY_COM(devices->GetCount(&count));  

  for(UINT i = 0; i < count; i++) {    
    CComPtr<IMMDevice> device;
    CComPtr<IAudioClient> client;
    CComPtr<IPropertyStore> store;
    CComHeapPtr<WAVEFORMATEX> wfx;
    XtPropVariant currentIdString;
    XT_VERIFY_COM(devices->Item(i, &device));
    XT_VERIFY_COM(device->OpenPropertyStore(STGM_READ, &store));
    XT_VERIFY_COM(store->GetValue(PKEY_AudioEndpoint_GUID, &currentIdString.pv));
    XT_VERIFY_COM(CLSIDFromString(currentIdString.pv.pwszVal, &deviceId));
    if(deviceId == thisId) {
      XT_VERIFY_COM(device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&client)));
      XT_VERIFY_COM(client->GetMixFormat(&wfx));
      if(!XtiWfxToFormat(*wfx, !input, format))
        return DSERR_BADFORMAT;
      *valid = XtTrue;
      mix->rate = format.mix.rate;
      mix->sample = format.mix.sample;
      return S_OK;
    }
  }
  return S_OK;
}

XtFault DSoundDevice::OpenStreamCore(const XtDeviceStreamParams* params, bool secondary, void* user, XtStream** stream) {

  HRESULT hr;
  int32_t frameSize;
  int32_t bufferFrames;
  WAVEFORMATEXTENSIBLE wfx;
  DSBUFFERDESC renderDesc = { 0 };
  DSCBUFFERDESC captureDesc = { 0 };
  CComPtr<IDirectSound> newOutput;
  CComPtr<IDirectSoundBuffer> render;
  CComPtr<IDirectSoundCapture> newInput;
  CComPtr<IDirectSoundCaptureBuffer> capture;

  double bufferSize = params->bufferSize;
  XT_ASSERT(XtiFormatToWfx(params->format, wfx));
  if(bufferSize < XtDsMinBufferMs)
    bufferSize = XtDsMinBufferMs;
  if(bufferSize > XtDsMaxBufferMs)
    bufferSize = XtDsMaxBufferMs;
  bufferFrames = static_cast<int32_t>(bufferSize / 1000.0 * params->format.mix.rate);
  frameSize = (params->format.channels.inputs + params->format.channels.outputs) * XtiGetSampleSize(params->format.mix.sample);

  if(input) {
    captureDesc.dwSize = sizeof(DSCBUFFERDESC);
    captureDesc.dwBufferBytes = bufferFrames * frameSize;
    captureDesc.lpwfxFormat = reinterpret_cast<WAVEFORMATEX*>(&wfx);
    XT_VERIFY_COM(DirectSoundCaptureCreate8(&guid, &newInput, nullptr));
    XT_VERIFY_COM(newInput->CreateCaptureBuffer(&captureDesc, &capture, nullptr));
  } else {
    renderDesc.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_TRUEPLAYPOSITION;
    renderDesc.dwSize = sizeof(DSBUFFERDESC);
    renderDesc.dwBufferBytes = bufferFrames * frameSize;
    renderDesc.lpwfxFormat = reinterpret_cast<WAVEFORMATEX*>(&wfx);
    XT_VERIFY_COM(DirectSoundCreate(&guid, &newOutput, nullptr));
    XT_VERIFY_COM(newOutput->SetCooperativeLevel(static_cast<HWND>(XtPlatform::instance->_window), DSSCL_PRIORITY));
    XT_VERIFY_COM(newOutput->CreateSoundBuffer(&renderDesc, &render, nullptr));
  }

  *stream = new DSoundStream(secondary, newInput, newOutput, capture, render, bufferFrames, frameSize);
  return S_OK;
}

// ---- stream ----

XtFault DSoundStream::GetFrames(int32_t* frames) const {
  *frames = bufferFrames;
  return S_OK;
}

XtFault DSoundStream::GetLatency(XtLatency* latency) const {
  return S_OK;
}

void DSoundStream::StopStream() {
  if(capture)
    XT_ASSERT(SUCCEEDED(capture->Stop()));
  else
    XT_ASSERT(SUCCEEDED(render->Stop()));
  if(!_secondary) {
    XT_ASSERT(CancelWaitableTimer(timer.timer));
    XT_ASSERT(timeEndPeriod(GetTimerPeriod(bufferFrames, _params.format.mix.rate) / 2) == TIMERR_NOERROR);
    XT_ASSERT(SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL));
  }
  xtBytesProcessed = 0;
  dsBytesProcessed = 0;
  previousDsPosition = 0;
}

void DSoundStream::StartStream() {
  LARGE_INTEGER due;
  due.QuadPart = -1;
  UINT timerPeriod = GetTimerPeriod(bufferFrames, _params.format.mix.rate);
  long lTimerPeriod = timerPeriod;
  if(!_secondary) {
    XT_ASSERT(SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL));
    XT_ASSERT(timeBeginPeriod(timerPeriod / 2) == TIMERR_NOERROR);
    XT_ASSERT(SetWaitableTimer(timer.timer, &due, lTimerPeriod, nullptr, nullptr, TRUE));
  }
  if(capture)
    XT_ASSERT(SUCCEEDED(capture->Start(DSCBSTART_LOOPING)));
  else
    XT_ASSERT(SUCCEEDED(render->Play(0, 0, DSBPLAY_LOOPING)));
}

void DSoundStream::ProcessBuffer(bool prefill) {
  
  void* audio1;
  void* audio2;
  int32_t gap, available, dsProcessed;
  DWORD bufferBytes = static_cast<DWORD>(buffer.size());
  DWORD size1, size2, read, write, lockPosition, waitResult;
  DWORD bufferMillis = static_cast<DWORD>(bufferFrames * 1000.0 / _params.format.mix.rate);

  XtBuffer xtBuffer = { 0 };

  if(!prefill && !_secondary) {
    waitResult = WaitForSingleObject(timer.timer, bufferMillis);
    if(waitResult == WAIT_TIMEOUT)
      return;
    XT_ASSERT(waitResult == WAIT_OBJECT_0);
  }
  
  if(prefill && render) {
    if(!XT_VERIFY_ON_BUFFER(render->Lock(0, bufferBytes, &audio1, &size1, &audio2, &size2, 0)))
      return;
    if(size2 == 0) {
      xtBuffer.input = nullptr;
      xtBuffer.output = audio1;
      xtBuffer.frames = bufferFrames;
      OnBuffer(&xtBuffer);
    } else {
      xtBuffer.input = nullptr;
      xtBuffer.output = &buffer[0];
      xtBuffer.frames = bufferFrames;
      OnBuffer(&xtBuffer);
      SplitBufferParts(buffer, audio1, size1, audio2, size2);
    }
    if(!XT_VERIFY_ON_BUFFER(render->Unlock(audio1, size1, audio2, size2)))
      return;
    xtBytesProcessed += bufferBytes;
    return;
  }

  if(capture && !prefill) {
    if(!XT_VERIFY_ON_BUFFER(capture->GetCurrentPosition(&write, &read)))
      return;
    gap = WrapAround(write - read, bufferBytes);
    dsProcessed = WrapAround(write - previousDsPosition, bufferBytes);
    dsBytesProcessed += dsProcessed;
    lockPosition = xtBytesProcessed % bufferBytes;
    available = static_cast<int32_t>(dsBytesProcessed - xtBytesProcessed - gap);
    previousDsPosition = write;
    if(available <= 0)
      return;
    if(InsideSafetyGap(read, write, lockPosition)) {
      XT_VERIFY_ON_BUFFER(DSERR_BUFFERLOST);
      return;
    }
    if(!XT_VERIFY_ON_BUFFER(capture->Lock(lockPosition, available, &audio1, &size1, &audio2, &size2, 0)))
      return;
    if(size2 == 0) {
      xtBuffer.input = audio1;
      xtBuffer.output = nullptr;
      xtBuffer.frames = available / frameSize;
      OnBuffer(&xtBuffer);
      if(!XT_VERIFY_ON_BUFFER(capture->Unlock(audio1, size1, audio2, size2)))
        return;
    } else {
      CombineBufferParts(buffer, audio1, size1, audio2, size2);
      if(!XT_VERIFY_ON_BUFFER(capture->Unlock(audio1, size1, audio2, size2)))
        return;
      xtBuffer.input = &buffer[0];
      xtBuffer.output = nullptr;
      xtBuffer.frames = available / frameSize;
      OnBuffer(&xtBuffer);
    }
    xtBytesProcessed += available;
  }

  if(render && !prefill) {
    if(!XT_VERIFY_ON_BUFFER(render->GetCurrentPosition(&read, &write)))
      return;
    gap = WrapAround(write - read, bufferBytes);
    dsProcessed = WrapAround(read - previousDsPosition, bufferBytes);
    dsBytesProcessed += dsProcessed;
    lockPosition = xtBytesProcessed % bufferBytes;
    available = static_cast<int32_t>(bufferBytes - gap - (xtBytesProcessed - dsBytesProcessed));
    previousDsPosition = read;
    if(available <= 0)
      return;
    if(InsideSafetyGap(read, write, lockPosition)) {
      XT_VERIFY_ON_BUFFER(DSERR_BUFFERLOST);
      return;
    }
    if(!XT_VERIFY_ON_BUFFER(render->Lock(lockPosition, available, &audio1, &size1, &audio2, &size2, 0)))
      return;
    if(size2 == 0) {
      xtBuffer.input = nullptr;
      xtBuffer.output = audio1;
      xtBuffer.frames = available / frameSize;
      OnBuffer(&xtBuffer);
    } else {
      xtBuffer.input = nullptr;
      xtBuffer.output = &buffer[0];
      xtBuffer.frames = available / frameSize;
      OnBuffer(&xtBuffer);
      SplitBufferParts(buffer, audio1, size1, audio2, size2);
    }
    if(!XT_VERIFY_ON_BUFFER(render->Unlock(audio1, size1, audio2, size2)))
      return;
    xtBytesProcessed += available;
  }
}

#endif // XT_ENABLE_DSOUND
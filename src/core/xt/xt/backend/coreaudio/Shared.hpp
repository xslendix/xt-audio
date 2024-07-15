#ifndef XT_CORE_AUDIO_SHARED_HPP
#define XT_CORE_AUDIO_SHARED_HPP
#if XT_ENABLE_CORE_AUDIO
#include <xt/private/Device.hpp>
#include <xt/private/Stream.hpp>
#include <xt/private/Service.hpp>
#include <xt/private/DeviceList.hpp>
#include <xt/backend/coreaudio/Private.hpp>

// TODO REMOVE THIS COMMENT
// these are the "internal" counterparts to the public xt-audio API

struct CoreAudioService final:
public XtService 
{
  CoreAudioService();
  ~CoreAudioService();
  XT_IMPLEMENT_SERVICE(CoreAudio);
};

struct CoreAudioDevice final:
public XtDevice
{
  XT_IMPLEMENT_DEVICE();
  XT_IMPLEMENT_DEVICE_STREAM();
  XT_IMPLEMENT_DEVICE_BASE(CoreAudio);
  CoreAudioDevice();
};

struct CoreAudioDeviceList final:
public XtDeviceList
{
  XT_IMPLEMENT_DEVICE_LIST(CoreAudio);
};

struct CoreAudioStream final:
public XtStream
{
  XT_IMPLEMENT_STREAM();
  XT_IMPLEMENT_STREAM_BASE();
  XT_IMPLEMENT_STREAM_BASE_SYSTEM(CoreAudio);
};

#endif // XT_ENABLE_CORE_AUDIO
#endif // XT_CORE_AUDIO_SHARED_HPP
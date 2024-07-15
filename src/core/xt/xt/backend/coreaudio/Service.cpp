#if XT_ENABLE_CORE_AUDIO
#include <xt/private/Platform.hpp>
#include <xt/backend/coreaudio/Shared.hpp>
#include <xt/backend/coreaudio/Private.hpp>

#include <memory>

XtFault
CoreAudioService::GetFormatFault() const
{ 
  // TODO return a core-audio specific error code that means "audio format is not supported"
  return 0; 
}

CoreAudioService::
CoreAudioService() {}
CoreAudioService::
~CoreAudioService() {}

XtFault
CoreAudioService::OpenDeviceList(XtEnumFlags flags, XtDeviceList** list) const
{ 
  *list = new CoreAudioDeviceList; 
  return 0; 
}

XtServiceCaps
CoreAudioService::GetCapabilities() const
{
  auto result = XtServiceCapsNone;
  return static_cast<XtServiceCaps>(result);
}

XtFault
CoreAudioService::OpenDevice(char const* id, XtDevice** device) const
{  
  *device = nullptr;
  return 0;
}

XtFault
CoreAudioService::GetDefaultDeviceId(XtBool output, XtBool* valid, char* buffer, int32_t* size) const
{
  *valid = XtFalse;
  return 0; 
}

#endif // XT_ENABLE_CORE_AUDIO
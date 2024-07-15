#if XT_ENABLE_CORE_AUDIO
#include <xt/backend/coreaudio/Shared.hpp>
#include <xt/backend/coreaudio/Private.hpp>

XtFault
CoreAudioDeviceList::GetCount(int32_t* count) const
{
  // TODO scan devices
  *count = 0;  
  return 0; 
}

XtFault 
CoreAudioDeviceList::GetId(int32_t index, char* buffer, int32_t* size) const
{ 
  return 0;
}

XtFault
CoreAudioDeviceList::GetName(char const* id, char* buffer, int32_t* size) const
{ 
  return 0;
}

XtFault
CoreAudioDeviceList::GetCapabilities(char const* id, XtDeviceCaps* capabilities) const
{ 
  *capabilities = XtDeviceCapsNone;
  return 0; 
}

#endif // XT_ENABLE_CORE_AUDIO
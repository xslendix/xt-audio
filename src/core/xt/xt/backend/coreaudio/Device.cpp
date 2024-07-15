#if XT_ENABLE_CORE_AUDIO
#include <xt/private/Platform.hpp>
#include <xt/backend/coreaudio/Shared.hpp>
#include <xt/backend/coreaudio/Private.hpp>

// TODO figure out if there's an interesting object to return to the client
void*
CoreAudioDevice::GetHandle() const { return nullptr; }

// TODO i assume theres no control panel
XtFault
CoreAudioDevice::ShowControlPanel() { return 0; }

// --- here it gets interesting

XtFault 
CoreAudioDevice::GetChannelCount(XtBool output, int32_t* count) const
{ 
  *count = 0;
  return 0; // 0 = all-is-well
}

XtFault
CoreAudioDevice::SupportsAccess(XtBool interleaved, XtBool* supports) const
{ 
  *supports = XtFalse;
  return 0;
}

XtFault
CoreAudioDevice::GetMix(XtBool* valid, XtMix* mix) const
{
  *valid = XtFalse;
  return 0;
}

XtFault 
CoreAudioDevice::SupportsFormat(const XtFormat* format, XtBool* supports) const
{
  *supports = XtFalse;
  return 0;
}

XtFault
CoreAudioDevice::GetBufferSize(XtFormat const* format, XtBufferSize* size) const
{
  size->min = 0;
  size->max = 0;
  size->current = 0;
  return 0;
}

XtFault
CoreAudioDevice::GetChannelName(XtBool output, int32_t index, char* buffer, int32_t* size) const
{
  XtiCopyString("FIGURE_OUT_THE_CHANNEL_NAME_FOR_INDEX", buffer, size);
  return 0;
}

XtFault 
CoreAudioDevice::OpenStreamCore(XtDeviceStreamParams const* params, XtStream** stream)
{  
  // This is where usually the most stuff needs to happen.
  *stream = nullptr;
  return 0;
}

#endif // XT_ENABLE_CORE_AUDIO
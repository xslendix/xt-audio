#if XT_ENABLE_CORE_AUDIO
#include <xt/backend/coreaudio/Shared.hpp>
#include <xt/backend/coreaudio/Private.hpp>

#include <utility>

// TODO figure out if there's an interesting object to return to the client
void*
CoreAudioStream::GetHandle() const { return nullptr; }

XtBool
CoreAudioStream::IsRunning() const
{
  return false;
}

XtFault
CoreAudioStream::GetLatency(XtLatency* latency) const
{ 
  return 0; 
}

XtFault
CoreAudioStream::GetFrames(int32_t* frames) const
{
  *frames = 0;
  return 0;
}

void
CoreAudioStream::Stop()
{ 
  // See ASIO and JACK
  OnRunning(XtFalse, 0);
}

XtFault
CoreAudioStream::Start()
{  
  // See ASIO and JACK
  OnRunning(XtTrue, 0);
  return 0;
}

#endif // XT_ENABLE_CORE_AUDIO
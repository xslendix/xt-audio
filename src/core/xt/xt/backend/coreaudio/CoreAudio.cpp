#if XT_ENABLE_CORE_AUDIO
#include <xt/shared/MacOS.hpp>
#include <xt/backend/coreaudio/Shared.hpp>
#include <xt/backend/coreaudio/Private.hpp>

#include <memory>
#include <cassert>

#include <iostream> // TODO remove me

std::unique_ptr<XtService>
XtiCreateCoreAudioService()
{ 
  // TODO remove this print
  // It's just here to make sure we can link/load/run the library (dylib)
  std::cout << "Hello from MacOS CoreAudio!\n";
  return std::make_unique<CoreAudioService>(); 
}

XtServiceError
XtiGetCoreAudioError(XtFault fault)
{
  XtServiceError result;
  result.text = "TODO";
  result.cause = XtCauseUnknown;
  return result;
}

#endif // XT_ENABLE_CORE_AUDIO
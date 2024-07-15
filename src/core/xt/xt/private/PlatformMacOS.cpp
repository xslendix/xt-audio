#ifdef __APPLE__
#include <xt/private/Platform.hpp>

// TODO -- any or all of these might be interesting! Or not, I don't know.
XtPlatform::~XtPlatform() { }
void XtPlatform::EndThread() { }
void XtPlatform::BeginThread() { }
bool XtPlatform::Init(void* window) { return true; }
void XtPlatform::RevertThreadPriority(int32_t policy, int32_t previous) { }
void XtPlatform::RaiseThreadPriority(int32_t* policy, int32_t* previous) { }

XtSystem
XtPlatform::SetupToSystem(XtSetup setup) const
{
  // CoreAudio is the alpha and omega on MacOS.
  switch (setup)
  {
  case XtSetupProAudio:
  case XtSetupSystemAudio:
  case XtSetupConsumerAudio: return XtSystemCoreAudio;
  default: XT_ASSERT(false); return static_cast<XtSystem>(0);
  }
}

#endif // __APPLE__
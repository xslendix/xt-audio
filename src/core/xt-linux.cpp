/* Copyright (C) 2015-2020 Sjoerd van Kreel.
 *
 * This file is part of XT-Audio.
 *
 * XT-Audio is free software: you can redistribute it and/or modify it under the 
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * XT-Audio is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with XT-Audio. If not, see<http://www.gnu.org/licenses/>.
 */
#ifdef __linux__
#include "xt-linux.hpp"
#include <pthread.h>

// ---- local ----

static pthread_t XtlMainThread;
static bool XtlInitialized = false;

extern const XtService* XtiServiceAlsa;
extern const XtService* XtiServiceJack;
extern const XtService* XtiServicePulse;

const XtService* const XtiServices[] =
{
#ifndef XT_DISABLE_PULSE
  XtiServicePulse,
#endif // XT_DISABLE_PULSE
#ifndef XT_DISABLE_ALSA
  XtiServiceAlsa,
#endif // XT_DISABLE_ALSA
#ifndef XT_DISABLE_JACK
  XtiServiceJack,
#endif // XT_DISABLE_JACK
  nullptr,
};

static XtStreamState ReadLinuxBlockingStreamState(
  XtlLinuxBlockingStream* stream) {

  XtStreamState result;
  XT_ASSERT(pthread_mutex_lock(&stream->lock.m) == 0);
  result = stream->state;
  XT_ASSERT(pthread_mutex_unlock(&stream->lock.m) == 0);
  return result;
}

static void ReceiveLinuxBlockingStreamControl(
  XtlLinuxBlockingStream* stream, XtStreamState state) {

  XT_ASSERT(pthread_mutex_lock(&stream->lock.m) == 0);
  stream->state = state;
  XT_ASSERT(pthread_cond_signal(&stream->respondCv.cv) == 0);
  XT_ASSERT(pthread_mutex_unlock(&stream->lock.m) == 0);
}

static void SendLinuxBlockingStreamControl(
  XtlLinuxBlockingStream* stream, XtStreamState from, XtStreamState to) {

  XT_ASSERT(pthread_mutex_lock(&stream->lock.m) == 0);
  if(stream->state == to) {
    XT_ASSERT(pthread_mutex_unlock(&stream->lock.m) == 0);
    return;
  }
  stream->state = from;
  XT_ASSERT(pthread_cond_signal(&stream->controlCv.cv) == 0);
  while(stream->state != to)
    XT_ASSERT(pthread_cond_wait(&stream->respondCv.cv, &stream->lock.m) == 0);
  XT_ASSERT(pthread_mutex_unlock(&stream->lock.m) == 0);
}

static void* LinuxBlockingStreamCallback(void* user) {

  int policy;
  int maxPriority;
  int startPriority;
  XtStreamState state;
  struct sched_param param;
  auto stream = static_cast<XtlLinuxBlockingStream*>(user);

  XT_ASSERT(pthread_getschedparam(pthread_self(), &policy, &param) == 0);
  startPriority = param.sched_priority;
  maxPriority = sched_get_priority_max(policy);
  while((state = ReadLinuxBlockingStreamState(stream)) != XtStreamStateClosed) {
    switch(state) {
    case XtStreamStateStarted:
      stream->ProcessBuffer(false);
      break;
    case XtStreamStateClosing:
      ReceiveLinuxBlockingStreamControl(stream, XtStreamStateClosed);
      return nullptr;
    case XtStreamStateStopping:
      stream->StopStream();
      param.sched_priority = startPriority;
      XT_ASSERT(pthread_setschedparam(pthread_self(), policy, &param) == 0);
      ReceiveLinuxBlockingStreamControl(stream, XtStreamStateStopped);
      break;
    case XtStreamStateStarting:
      stream->ProcessBuffer(true);
      param.sched_priority = maxPriority;
      XT_ASSERT(pthread_setschedparam(pthread_self(), policy, &param) == 0);
      stream->StartStream();
      ReceiveLinuxBlockingStreamControl(stream, XtStreamStateStarted);
      break;
    case XtStreamStateStopped:
      XT_ASSERT(pthread_mutex_lock(&stream->lock.m) == 0);
      XT_ASSERT(pthread_cond_wait(&stream->controlCv.cv, &stream->lock.m) == 0);
      XT_ASSERT(pthread_mutex_unlock(&stream->lock.m) == 0);
      break;
    default:
      XT_FAIL("Unexpected stream state.");
      break;
    }
  }
  XT_FAIL("End of stream callback reached.");
  return nullptr;
}

// ---- api ----

XtBool XT_CALL XtAudioIsWin32(void) {
  return XtFalse;
}

int32_t XT_CALL XtAudioGetServiceCount(void) { 
  return sizeof(XtiServices) / sizeof(XtiServices[0]) - 1;
}

const XtService* XT_CALL XtAudioGetServiceByIndex(int32_t index) {
  XT_ASSERT(0 <= index && index < XtAudioGetServiceCount());
  return XtiServices[index];
}

const XtService* XT_CALL XtAudioGetServiceBySystem(XtSystem system) {
  XT_ASSERT(XtSystemAlsa <= system && system <= XtSystemWasapi);
  switch(system) {
  case XtSystemAlsa: return XtiServiceAlsa;
  case XtSystemJack: return XtiServiceJack;
  case XtSystemPulse: return XtiServicePulse;
  case XtSystemAsio:
  case XtSystemDSound:
  case XtSystemWasapi: return nullptr;
  default: return XT_FAIL("Unknown system."), nullptr;
  }
}

// ---- internal ----

void XtiTerminatePlatform() { 
  XtlTerminateJack();
  XtlTerminateAlsa();
  XtlInitialized = false;
}

void XtiInitPlatform(void* wnd) {
  XT_ASSERT(!XtlInitialized);
  XtlMainThread = pthread_self();
  XtlInitAlsa();
  XtlInitJack();
  XtlInitialized = true;
  XT_TRACE(XtLevelInfo, "Built with ALSA: %d.", XtAudioGetServiceBySystem(XtSystemAlsa) != nullptr);
  XT_TRACE(XtLevelInfo, "Built with JACK: %d.", XtAudioGetServiceBySystem(XtSystemJack) != nullptr);
  XT_TRACE(XtLevelInfo, "Built with PulseAudio: %d.", XtAudioGetServiceBySystem(XtSystemPulse) != nullptr);
}

int32_t XtiLockIncr(volatile int32_t* dest) {
  return __sync_add_and_fetch(dest, 1);
}

int32_t XtiLockDecr(volatile int32_t* dest) {
  return __sync_sub_and_fetch(dest, 1);
}

int32_t XtiCas(volatile int32_t* dest, int32_t exch, int32_t comp) {
  return __sync_val_compare_and_swap(dest, comp, exch);
}

bool XtiCalledOnMainThread() {
  return XtlInitialized && pthread_equal(pthread_self(), XtlMainThread);
}

XtSystem XtiSetupToSystem(XtSetup setup) {
  switch(setup) {
  case XtSetupProAudio: return XtSystemJack;
  case XtSetupSystemAudio: return XtSystemAlsa;
  case XtSetupConsumerAudio: return XtSystemPulse;
  default: return XT_FAIL("Unknown setup."), XtSystemPulse;
  }
}

// ---- linux ----

XtCause XtlPosixErrorToCause(XtFault fault) {
  switch(fault) {
  case ESRCH: return XtCauseService;
  case EINVAL: return XtCauseFormat;
  case EBUSY:
  case ENXIO:
  case EPIPE:
  case ENODEV:
  case ENOENT:
  case ESTRPIPE: return XtCauseEndpoint;
  default: return XtCauseUnknown;
  }
}

XtlLinuxBlockingStream::XtlLinuxBlockingStream(bool secondary):
XtBlockingStream(secondary),
lock(),
state(XtStreamStateStopped),
respondCv(), 
controlCv() {
  if(!secondary) {
    pthread_t thread;
    XT_ASSERT(pthread_create(&thread, nullptr, &LinuxBlockingStreamCallback, this) == 0);
  }
}

XtlLinuxBlockingStream::~XtlLinuxBlockingStream() {
  if(!secondary) 
    SendLinuxBlockingStreamControl(this, XtStreamStateClosing, XtStreamStateClosed);
}

XtFault XtlLinuxBlockingStream::Start() {
  if(!secondary)
    SendLinuxBlockingStreamControl(this, XtStreamStateStarting, XtStreamStateStarted);
  else {
    ProcessBuffer(true);
    StartStream();
  }
  return 0;
}

XtFault XtlLinuxBlockingStream::Stop() {
  if(secondary)
    StopStream();
  else
    SendLinuxBlockingStreamControl(this, XtStreamStateStopping, XtStreamStateStopped);
  return 0;
}

void XtlLinuxBlockingStream::RequestStop() {
  StopStream();
  if(!secondary) {
    XT_ASSERT(pthread_mutex_lock(&lock.m) == 0);
    state = XtStreamStateStopped;
    XT_ASSERT(pthread_cond_signal(&respondCv.cv) == 0);
    XT_ASSERT(pthread_mutex_unlock(&lock.m) == 0);
  }
}

bool XtlLinuxBlockingStream::VerifyStreamCallback(int error, const char* file, int line, const char* func, const char* expr) {
  if(error == 0)
    return true;
  RequestStop();
  XtiTrace(XtLevelError, file, line, func, expr);
  ProcessCallback(nullptr, nullptr, 0, 0.0, 0, XtFalse, XtiCreateError(XtStreamGetSystem(this), error));
  return false;
}

#endif // __linux__
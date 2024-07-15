#ifndef XT_API_SHARED_H
#define XT_API_SHARED_H

/** @cond */
#include <stdint.h>
/** @endcond */

#ifdef _WIN32
#define XT_CALL __stdcall
#elif __linux__
#define XT_CALL
#elif __APPLE__
#define XT_CALL
// TODO I think we can leave this empty like linux, but keep the TODO here for now in case weird stuff starts happening
#else
#error "Unsupported platform."
#endif // _WIN32
#define XT_CALLBACK XT_CALL

/** @cond */
#ifndef XT_EXPORT
#define XT_API
#elif (defined _WIN32)
#define XT_API __declspec(dllexport)
#elif __linux__
#define XT_API __attribute__((visibility("default")))
#elif __APPLE__
// TODO consolidate __apple__/__linux__ and also go with -fvisibility=hidden.
#define XT_API __attribute__((visibility("default")))
#else
#error "Unsupported platform."
#endif // XT_EXPORT
/** @endcond */

typedef int32_t XtBool;
typedef uint64_t XtError;
static XtBool const XtTrue = 1;
static XtBool const XtFalse = 0;

typedef struct XtDevice XtDevice;
typedef struct XtStream XtStream;
typedef struct XtService XtService;
typedef struct XtPlatform XtPlatform;
typedef struct XtDeviceList XtDeviceList;

#endif // XT_API_SHARED_H
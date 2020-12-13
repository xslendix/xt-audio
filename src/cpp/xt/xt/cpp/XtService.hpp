#ifndef XT_CPP_SERVICE_HPP
#define XT_CPP_SERVICE_HPP

/** @file */

#include <xt/cpp/Core.hpp>
#include <xt/cpp/Enums.hpp>
#include <xt/cpp/Structs.hpp>
#include <xt/cpp/Forward.hpp>
#include <xt/cpp/XtDevice.hpp>
#include <xt/cpp/XtDeviceList.hpp>

#include <memory>
#include <optional>

namespace Xt {

using namespace Detail;

class Service final 
{
  friend class Platform;
  XtService const* const _s;
  Service(XtService const* s): _s(s) { }
public:
  Capabilities GetCapabilities() const;  
  std::unique_ptr<Device> OpenDevice(std::string const& id) const;
  std::optional<std::string> GetDefaultDeviceId(bool output) const;
  std::unique_ptr<DeviceList> OpenDeviceList(EnumFlags flags) const;
  std::unique_ptr<Stream> AggregateStream(AggregateStreamParams const& params, void* user);
};

inline Capabilities
Service::GetCapabilities() const
{
  auto coreCapabilities = XtServiceGetCapabilities(_s);
  return static_cast<Capabilities>(coreCapabilities); 
}

inline std::unique_ptr<DeviceList> 
Service::OpenDeviceList(EnumFlags flags) const 
{ 
  XtDeviceList* list; 
  auto coreFlags = static_cast<XtEnumFlags>(flags);
  Detail::HandleError(XtServiceOpenDeviceList(_s, coreFlags, &list));
  return std::unique_ptr<DeviceList>(new DeviceList(list));
}

inline std::unique_ptr<Device> 
Service::OpenDevice(std::string const& id) const 
{ 
  XtDevice* device; 
  Detail::HandleError(XtServiceOpenDevice(_s, id.c_str(), &device));
  return std::unique_ptr<Device>(new Device(device));
}

inline std::optional<std::string>
Service::GetDefaultDeviceId(bool output) const
{
  XtBool valid;
  int32_t size = 0;
  Detail::HandleError(XtServiceGetDefaultDeviceId(_s, output, &valid, nullptr, &size));
  if(!valid) return std::optional<std::string>(std::nullopt);
  std::vector<char> buffer(static_cast<size_t>(size));
  Detail::HandleError(XtServiceGetDefaultDeviceId(_s, output, &valid, buffer.data(), &size));
  if(!valid) return std::optional<std::string>(std::nullopt);
  return std::optional<std::string>(std::string(buffer.data()));
}

inline std::unique_ptr<Stream> 
Service::AggregateStream(AggregateStreamParams const& params, void* user)
{
  XtStream* stream = nullptr;
  std::vector<XtAggregateDeviceParams> ds(params.count);
  for(int32_t i = 0; i < params.count; i++)
  {
    ds[i].device = params.devices[i].device->_d;
    ds[i].bufferSize = params.devices[i].bufferSize;
    ds[i].channels = *reinterpret_cast<XtChannels const*>(&params.devices[i].channels);
  }
  XtAggregateStreamParams coreParams = { 0 };
  coreParams.devices = ds.data();
  coreParams.count = params.count;
  coreParams.master = params.master->_d;
  coreParams.stream.onBuffer = Detail::ForwardOnBuffer;
  coreParams.stream.interleaved = params.stream.interleaved;
  coreParams.mix = *reinterpret_cast<XtMix const*>(&params.mix);
  coreParams.stream.onXRun = params.stream.onXRun == nullptr? nullptr: Detail::ForwardOnXRun;
  std::unique_ptr<Stream> result(new Stream(params.stream.onBuffer, params.stream.onXRun, user));
  Detail::HandleError(XtServiceAggregateStream(_s, &coreParams, result.get(), &stream));
  result->_s = stream;
  return result;
}

} // namespace Xt
#endif // XT_CPP_SERVICE_HPP
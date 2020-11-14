using System;
using System.Linq;
using System.Runtime.InteropServices;
using static Xt.XtNative;

namespace Xt
{
    public sealed class XtService
    {
        [DllImport("xt-core")] static extern XtCapabilities XtServiceGetCapabilities(IntPtr s);
        [DllImport("xt-core")] static extern ulong XtServiceGetDeviceCount(IntPtr s, out int count);
        [DllImport("xt-core")] static extern ulong XtServiceOpenDevice(IntPtr s, int index, out IntPtr device);
        [DllImport("xt-core")] static extern ulong XtServiceOpenDefaultDevice(IntPtr s, bool output, out IntPtr device);
        [DllImport("xt-core")] static extern ulong XtServiceAggregateStream(IntPtr s, in AggregateStreamParams @params, IntPtr user, out IntPtr stream);

        readonly IntPtr _s;
        internal XtService(IntPtr s) => _s = s;

        public XtCapabilities GetCapabilities() => XtServiceGetCapabilities(_s);
        public int GetDeviceCount() => HandleError(XtServiceGetDeviceCount(_s, out var r), r);
        public XtDevice OpenDevice(int index) => HandleError(XtServiceOpenDevice(_s, index, out var r), new XtDevice(r));
        public XtDevice OpenDefaultDevice(bool output) => HandleError(XtServiceOpenDefaultDevice(_s, output, out var r), new XtDevice(r));

        public XtStream AggregateStream(in XtAggregateStreamParams @params, object user)
        {
            var result = new XtStream(@params.stream.streamCallback, @params.stream.xRunCallback, user);
            var native = new AggregateStreamParams();
            native.mix = @params.mix;
            native.count = @params.count;
            native.master = @params.master.Handle();
            native.devices = @params.devices.Select(ToNative).ToArray();
            native.stream.streamCallback = result.NativeStreamCallback();
            native.stream.interleaved = @params.stream.interleaved ? 1 : 0;
            native.stream.xRunCallback = @params.stream.xRunCallback == null ? null : result.NativeXRunCallback();
            result.Init(HandleError(XtServiceAggregateStream(_s, in native, IntPtr.Zero, out var r), r));
            return result;
        }

        static AggregateDeviceParams ToNative(XtAggregateDeviceParams managed)
        {
            var result = new AggregateDeviceParams();
            result.channels = managed.channels;
            result.bufferSize = managed.bufferSize;
            result.device = managed.device.Handle();
            return result;
        }
    }
}
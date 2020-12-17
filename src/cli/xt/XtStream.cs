using System;
using System.Runtime.InteropServices;
using System.Security;
using static Xt.Utility;

namespace Xt
{
    [SuppressUnmanagedCodeSecurity]
    public sealed class XtStream : IDisposable
    {
        [DllImport("xt-core")] static extern void XtStreamStop(IntPtr s);
        [DllImport("xt-core")] static extern ulong XtStreamStart(IntPtr s);
        [DllImport("xt-core")] static extern void XtStreamDestroy(IntPtr s);
        [DllImport("xt-core")] static extern int XtStreamIsRunning(IntPtr s);
        [DllImport("xt-core")] static extern unsafe XtFormat* XtStreamGetFormat(IntPtr s);
        [DllImport("xt-core")] static extern ulong XtStreamGetFrames(IntPtr s, out int frames);
        [DllImport("xt-core")] static extern ulong XtStreamGetLatency(IntPtr s, out XtLatency latency);

        IntPtr _s;
        readonly object _user;
        readonly XtOnXRun _onXRun;
        readonly XtOnBuffer _onBuffer;
        readonly XtOnRunning _onRunning;
        readonly OnXRun _onNativeXRun;
        readonly OnBuffer _onNativeBuffer;
        readonly OnRunning _onNativeRunning;

        internal XtStream(XtOnBuffer onBuffer, XtOnXRun onXRun, XtOnRunning onRunning, object user)
        {
            _user = user;
            _onXRun = onXRun;
            _onBuffer = onBuffer;
            _onRunning = onRunning;
            _onNativeXRun = OnXRun;
            _onNativeBuffer = OnBuffer;
            _onNativeRunning = OnRunning;
        }

        public void Stop() => XtStreamStop(_s);
        public void Dispose() => XtStreamDestroy(_s);
        public void Start() => HandleError(XtStreamStart(_s));
        public bool IsRunning() => XtStreamIsRunning(_s) != 0;
        public unsafe XtFormat GetFormat() => *XtStreamGetFormat(_s);
        public int GetFrames() => HandleError(XtStreamGetFrames(_s, out var r), r);
        public XtLatency GetLatency() => HandleError(XtStreamGetLatency(_s, out var r), r);

        internal void Init(IntPtr s) => _s = s;
        internal OnXRun OnNativeXRun() => _onNativeXRun;
        internal OnBuffer OnNativeBuffer() => _onNativeBuffer;
        internal OnRunning OnNativeRunning() => _onNativeRunning;

        void OnXRun(IntPtr stream, int index, IntPtr user) => _onXRun(this, index, _user);
        void OnRunning(IntPtr stream, int running, IntPtr user) => _onRunning(this, running != 0, _user);
        int OnBuffer(IntPtr stream, in XtBuffer buffer, IntPtr user) => _onBuffer(this, in buffer, _user);
    }
}
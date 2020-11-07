using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace Xt
{
	static class NativeUtility
	{
		internal static unsafe string PtrToStringUTF8(IntPtr ptr)
		{
			var range = Enumerable.Range(0, int.MaxValue);
			var length = range.SkipWhile(i => ((byte*)ptr)[i] != 0).First();
			return Encoding.UTF8.GetString((byte*)ptr, length);
		}
	}

	class StreamCallbackWrapper
	{
		Array _input;
		Array _output;
		byte[] _scratch;
		XtStream _stream;
		XtManagedBuffer _buffer;
		readonly bool _interleaved;
		readonly XtManagedStreamCallback _callback;

		internal StreamCallbackWrapper(bool interleaved, XtManagedStreamCallback callback)
			=> (_interleaved, _callback) = (interleaved, callback);

		internal void Init(XtStream stream)
		{
			var types = new Dictionary<XtSample, Type>();
			types.Add(XtSample.UInt8, typeof(byte));
			types.Add(XtSample.Int16, typeof(short));
			types.Add(XtSample.Int24, typeof(byte));
			types.Add(XtSample.Int32, typeof(int));
			types.Add(XtSample.Float32, typeof(float));
			var format = stream.GetFormat();
			var frames = stream.GetFrames();
			var type = types[format.mix.sample];
			var attrs = XtAudio.GetSampleAttributes(format.mix.sample);
			_stream = stream;
			_scratch = new byte[Math.Max(format.channels.inputs, format.channels.outputs) * frames * attrs.size];
			_buffer.input = _interleaved 
				? Array.CreateInstance(type, format.channels.inputs * frames * attrs.count) 
				: Enumerable.Range(0, format.channels.inputs).Select(_ => Array.CreateInstance(type, frames * attrs.count)).ToArray();
			_buffer.output = _interleaved 
				? Array.CreateInstance(type, format.channels.outputs * frames * attrs.count) 
				: Enumerable.Range(0, format.channels.outputs).Select(_ => Array.CreateInstance(type, frames * attrs.count)).ToArray();
		}

		internal unsafe void Callback(IntPtr stream, in XtBuffer buffer, in XtTime time, ulong error, IntPtr user)
		{
			var format = _stream.GetFormat();
			var frames = _stream.GetFrames();
			var attrs = XtAudio.GetSampleAttributes(format.mix.sample);
			if (_interleaved && buffer.input != IntPtr.Zero)
			{
				Marshal.Copy(buffer.input, _scratch, 0, frames * format.channels.inputs * attrs.size);
				Buffer.BlockCopy(_scratch, 0, _buffer.input, 0, frames * format.channels.inputs * attrs.size);
			}
			for (int i = 0; !_interleaved && buffer.input != IntPtr.Zero && i < format.channels.inputs; i++)
			{
				IntPtr* data = (IntPtr*)buffer.input.ToPointer();
				Marshal.Copy(data[i], _scratch, 0, frames * attrs.size);
				Buffer.BlockCopy(_scratch, 0, (Array)_buffer.input.GetValue(i), 0, frames * attrs.size);
			}
			_callback(_stream, in _buffer, in time, error);
			if (_interleaved && buffer.output != IntPtr.Zero)
			{
				Buffer.BlockCopy(_buffer.output, 0, _scratch, 0, frames * format.channels.outputs * attrs.size);
				Marshal.Copy(_scratch, 0, buffer.output, frames * format.channels.outputs * attrs.size);
			}
			for (int i = 0; !_interleaved && buffer.output != IntPtr.Zero && i < format.channels.outputs; i++)
			{
				IntPtr* data = (IntPtr*)buffer.output.ToPointer();
				Buffer.BlockCopy((Array)_buffer.output.GetValue(i), 0, _scratch, 0, frames * attrs.size);
				Marshal.Copy(_scratch, 0, data[i], frames * attrs.size);
			}
		}
	}
}
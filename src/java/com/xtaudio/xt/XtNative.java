package com.xtaudio.xt;

import com.sun.jna.DefaultTypeMapper;
import com.sun.jna.FromNativeContext;
import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.NativeLibrary;
import com.sun.jna.Pointer;
import com.sun.jna.ToNativeContext;
import com.sun.jna.TypeConverter;
import com.sun.jna.ptr.IntByReference;
import com.sun.jna.win32.StdCallFunctionMapper;
import com.sun.jna.win32.StdCallLibrary;
import com.xtaudio.xt.NativeTypes.*;
import com.xtaudio.xt.NativeTypes.XtSetup;

import java.util.HashMap;
import java.util.Map;

final class XtNative {

    private static boolean initialized = false;

    static class EnumConverter<E extends Enum<E>> implements TypeConverter {

        final int _base;
        final Class<E> _type;
        EnumConverter(Class<E> type, int base) { _base = base; _type = type; }
        @Override public Class<Integer> nativeType() { return Integer.class; }
        @Override public Object toNative(Object o, ToNativeContext tnc) { return o == null? 0: ((Enum<E>)o).ordinal() + _base; }
        @Override public Object fromNative(Object o, FromNativeContext fnc) { return _type.getEnumConstants()[((int)o) - _base]; }
    }

    static class XtTypeMapper extends DefaultTypeMapper {
        XtTypeMapper() {
            addTypeConverter(XtSetup.class, new EnumConverter<>(XtSetup.class, 0));
            addTypeConverter(XtCause.class, new EnumConverter<>(XtCause.class, 0));
            addTypeConverter(XtSample.class, new EnumConverter<>(XtSample.class, 0));
            addTypeConverter(XtSystem.class, new EnumConverter<>(XtSystem.class, 1));
        }
    }

    static void init() {
        if(initialized) return;
        boolean isX64 = Native.POINTER_SIZE == 8;
        System.setProperty("jna.encoding", "UTF-8");
        boolean isWin32 = System.getProperty("os.name").contains("Windows");
        Map<String, Object> options = new HashMap<>();
        options.put(Library.OPTION_TYPE_MAPPER, new XtTypeMapper());
        if(isWin32 && !isX64) {
            options.put(Library.OPTION_FUNCTION_MAPPER, new StdCallFunctionMapper());
            options.put(Library.OPTION_CALLING_CONVENTION, StdCallLibrary.STDCALL_CONVENTION);
        }
        if(isWin32 && !isX64)
            Native.register(NativeLibrary.getInstance("win32-x86/xt-core.dll", options));
        else if(isWin32 && isX64)
            Native.register(NativeLibrary.getInstance("win32-x64/xt-core.dll", options));
        else if(!isWin32 && !isX64)
            Native.register(NativeLibrary.getInstance("linux-x86/libxt-core.so", options));
        else
            Native.register(NativeLibrary.getInstance("linux-x64/libxt-core.so", options));
        initialized = true;
    }

    static void handleError(long error) {
        if(error != 0)
            throw new XtException(error);
    }

    static <T> T handleError(long error, T result) {
        if(error != 0)
            throw new XtException(error);
        return result;
    }

    static native String XtPrintErrorInfoToString(XtErrorInfo info);
    static native XtErrorInfo.ByValue XtAudioGetErrorInfo(long error);

    static native void XtAudioTerminate();
    static native XtVersion.ByValue XtAudioGetVersion();
    static native Pointer XtAudioGetService(XtSystem system);
    static native XtSystem XtAudioSetupToSystem(XtSetup setup);
    static native void XtAudioGetSystems(int[] buffer, IntByReference size);
    static native XtAttributes XtAudioGetSampleAttributes(XtSample sample);
    static native void XtAudioInit(String id, Pointer window, XtErrorCallback error);
}

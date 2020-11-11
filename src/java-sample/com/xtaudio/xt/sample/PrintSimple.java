package com.xtaudio.xt.sample;

import com.xtaudio.xt.*;
import com.xtaudio.xt.NativeTypes.*;

public class PrintSimple {

    public static void main(String[] args) {
        try(var audio = new XtAudio(null, null, null)) {
            for(XtSystem system: XtAudio.getSystems()) {
                XtService service = XtAudio.getService(system);
                for(int d = 0; d < service.getDeviceCount(); d++)
                    try(var device = service.openDevice(d)) {
                        System.out.println(system + ": " + device);
                    }
            }
        }
    }
}
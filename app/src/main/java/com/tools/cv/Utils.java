package com.tools.cv;

import android.content.Context;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

public class Utils {

    public static String copyAsset2Sdcard(Context context, String name) {
        File cascadeDir = context.getDir("cascade", Context.MODE_PRIVATE);
        File cascadeFile = new File(cascadeDir, "lbpcascade_frontalface.xml");
        if (!cascadeFile.exists()) {
            InputStream is = null;
            FileOutputStream fos = null;
            try {
                is = context.getAssets().open(name);
                fos = new FileOutputStream(cascadeFile);
                int len;
                byte[] buffer = new byte[2048];

                while ((len = is.read(buffer)) != -1) {
                    fos.write(buffer, 0, len);
                }
            } catch (IOException e) {
                e.printStackTrace();
            } finally {
                if (is == null) {
                    try {
                        is.close();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }
                if (fos == null) {
                    try {
                        fos.close();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }
            }
        }
        return cascadeFile.getAbsolutePath();
    }
}

package igagis.github.io.audouttests;

import android.app.Activity;

/**
 * Created by ivan.gagis on 18.5.2017.
 */

public class MainActivity extends Activity{
    {
        System.loadLibrary("c++_shared");
        System.loadLibrary("audouttests");
        test();
    }

    public static native void test();
}

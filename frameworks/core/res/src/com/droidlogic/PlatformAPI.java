package com.droidlogic;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.UserHandle;
import android.os.Handler;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

/** Hidden APIs that are not listed as system api for 28 */
final class PlatformAPI {

    private static final Method sMethod_startServiceAsUser;
    private static final Method sMethod_registerReceiverAsUser;

    static {
        try {
        sMethod_startServiceAsUser = Context.class.getMethod(
                "startServiceAsUser", Intent.class, UserHandle.class);
        sMethod_registerReceiverAsUser = Context.class.getMethod(
                "registerReceiverAsUser", BroadcastReceiver.class,
                UserHandle.class, IntentFilter.class, String.class,
                Handler.class);
        } catch (NoSuchMethodException e) {
            throw new RuntimeException(e);
        }
    }

    public static void startServiceAsUser(Context context, Intent intent,
            UserHandle user) {
        try {
            sMethod_startServiceAsUser.invoke(context, intent, user);
        } catch (IllegalAccessException e) {
            throw new RuntimeException(e);
        } catch (InvocationTargetException e) {
            throw new RuntimeException(e);
        }
    }

    public static void registerReceiverAsUser(Context context,
            BroadcastReceiver receiver, UserHandle user,
            IntentFilter filter, String broadcastPermission,
            Handler receiverHandler) {
        try {
            sMethod_registerReceiverAsUser.invoke(context, receiver, user,
                    filter, broadcastPermission, receiverHandler);
         } catch (IllegalAccessException e) {
            throw new RuntimeException(e);
       } catch (InvocationTargetException e) {
            throw new RuntimeException(e);
        }
    }

    public static boolean getBooleanProperty(String property, boolean defVal) {
        try {
            return (boolean)Class.forName("android.os.SystemProperties")
                .getMethod("getBoolean", new Class[] { String.class, Boolean.TYPE })
                .invoke(null, new Object[] { property, defVal });
        } catch(Exception e) {
            e.printStackTrace();
        }
        return false;
    }

    public static String getStringProperty(String property, String defVal) {
        try {
            return (String)Class.forName("android.os.SystemProperties")
                .getMethod("get", new Class[] { String.class, Boolean.TYPE })
                .invoke(null, new Object[] { property, defVal });
        } catch(Exception e) {
            e.printStackTrace();
        }
        return "";
    }

}

package droidlogic_jni

import (
    // "fmt"
    _"reflect"
    "android/soong/android"
    "android/soong/cc"
    "github.com/google/blueprint/proptools"
    _ "runtime/debug"
)

func init() {
    android.RegisterModuleType("hdmiin_go_defaults", hdmiin_go_DefaultsFactory)
}

func hdmiin_Defaults(ctx android.LoadHookContext) {
    type propsE struct {
        Enabled *bool
    }

    p := &propsE{}

    type props struct {
        Shared_libs  []string
    }
    p2 := &props{}

    // support hdmi or not, not support, do not need
    namespace := "amlogic_vendorconfig"
    vconfig := ctx.Config().VendorConfig(namespace)
    if vconfig.Bool("support_hdmiin") == true {
        // fmt.Println("droidlogic_jni: Supprt hdmi in")
        p.Enabled = proptools.BoolPtr(true)
    } else {
        // fmt.Println("droidlogic_jni: NOT Supprt hdmi in!")
        p.Enabled = proptools.BoolPtr(false)
    }
    ctx.AppendProperties(p)

    if vconfig.String("build_alsa_audio") == "tiny" {
        // fmt.Println("droidlogic_jni: alsa:", vconfig.String("build_alsa_audio"))
        p2.Shared_libs =  append(p2.Shared_libs, "libtinyalsa")
    } else {
        // fmt.Println("droidlogic_jni: alsa:", vconfig.String("build_alsa_audio"))
    }
    ctx.AppendProperties(p2)
}



func hdmiin_go_DefaultsFactory() (android.Module) {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, hdmiin_Defaults)
    return module
}


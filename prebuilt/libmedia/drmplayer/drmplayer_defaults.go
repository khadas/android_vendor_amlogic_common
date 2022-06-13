package amlogic_drmplayer

import (
    // "fmt"
    "android/soong/android"
    "android/soong/cc"
    "github.com/google/blueprint/proptools"
)

func init() {
    android.RegisterModuleType("amlogic_drmplayer_go_defaults",drmplayer_DefaultsFactory)
}

func drmplayer_DefaultsFactory() (android.Module) {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, drmplayer_Defaults)
    return module
}

func drmplayer_Defaults(ctx android.LoadHookContext) {
    type props struct {
        Enabled *bool
    }
    p := &props{}
    p.Enabled = drmplayer_globalDefaults(ctx)
    ctx.AppendProperties(p)
}


func drmplayer_globalDefaults(ctx android.BaseContext) (*bool) {
    var module_enabled *bool
    if android.ExistentPathForSource(ctx, "vendor/amlogic/common/frameworks/av/drmplayer").Valid() == true {
        module_enabled = proptools.BoolPtr(false)
        // fmt.Println("drmplayer:vendor/amlogic/common/frameworks/av/drmplayer exist, use source to build")
    } else {
        // fmt.Println("drmplayer:vendor/amlogic/common/frameworks/av/drmplayer not exist, use prebuilt codec lib")
    }
    return module_enabled
}

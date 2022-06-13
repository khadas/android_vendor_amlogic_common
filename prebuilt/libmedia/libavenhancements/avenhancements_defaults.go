package droidlogic_avenhancements

import (
    // "fmt"
    "android/soong/android"
    "android/soong/cc"
    "github.com/google/blueprint/proptools"
)

func init() {
    android.RegisterModuleType("avenhancements_adapter_go_defaults",avenhancements_adapter_DefaultsFactory)
    android.RegisterModuleType("avenhancements_amnuplayer_go_defaults",avenhancements_amnuplayer_DefaultsFactory)
}

func avenhancements_adapter_DefaultsFactory() (android.Module) {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, avenhancements_adapter_Defaults)
    return module
}

func avenhancements_adapter_Defaults(ctx android.LoadHookContext) {
    type props struct {
        Enabled *bool
    }
    p := &props{}
    p.Enabled = avenhancements_adapter_globalDefaults(ctx)
    ctx.AppendProperties(p)
}

func avenhancements_adapter_globalDefaults(ctx android.BaseContext) (*bool) {
    var module_enabled *bool
    if android.ExistentPathForSource(ctx, "vendor/amlogic/common/frameworks/av/AmFFmpegAdapter").Valid() ||
       android.ExistentPathForSource(ctx, "vendor/amlogic/common/AmFFmpegAdapter").Valid() {
        module_enabled = proptools.BoolPtr(false)
        // fmt.Println("avenhancements:AmFFmpegAdapter exist, use source to build")
    } else {
        // fmt.Println("avenhancements:vendor/amlogic/common/AmFFmpegAdapter not exist, use prebuilt enhance lib")
    }
    return module_enabled
}

func avenhancements_amnuplayer_DefaultsFactory() (android.Module) {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module,  func(ctx android.LoadHookContext) {
        type props struct {
            Enabled *bool
        }
        p := &props{}

        if android.ExistentPathForSource(ctx, "vendor/amlogic/common/frameworks/av/amnuplayer").Valid() == true ||
            android.ExistentPathForSource(ctx, "vendor/amlogic/common/amnuplayer").Valid() == true {
            p.Enabled = proptools.BoolPtr(false)
            // fmt.Println("avenhancements:amnuplayer exist, use source to build")
        }
        ctx.AppendProperties(p)
    })
    return module
}


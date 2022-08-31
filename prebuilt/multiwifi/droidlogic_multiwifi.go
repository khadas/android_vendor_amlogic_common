package droidlogic_multiwifi

import (
    // "fmt"
    "android/soong/android"
    "android/soong/cc"
    "github.com/google/blueprint/proptools"
)

func init() {
    android.RegisterModuleType("multiwifi_go_defaults", multiwifi_go_DefaultsFactory)
}

func multiwifi_go_DefaultsFactory() (android.Module) {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, func(ctx android.LoadHookContext) {
        type props struct {
            Enabled *bool
        }
        p := &props{}

       checkPath := "vendor/amlogic/common/wifi_bt/wifi/wifi-ext"
        if android.ExistentPathForSource(ctx, checkPath).Valid() == true {
            p.Enabled = proptools.BoolPtr(false)
            // fmt.Printf("multiwifi:%s exist, use source to build\n", checkPath)
        } else {
            // fmt.Printf("multiwifi:%s not exist, use prebuilt to build\n", checkPath)
        }
        ctx.AppendProperties(p)
    })
    return module
}

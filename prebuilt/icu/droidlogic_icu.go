package droidlogic_icu

import (
    // "fmt"
    "android/soong/android"
    "android/soong/cc"
    "github.com/google/blueprint/proptools"
)

func init() {
    android.RegisterModuleType("droidlogic_icu_go_defaults", icu_go_DefaultsFactory)
}

func icu_go_DefaultsFactory() (android.Module) {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, func(ctx android.LoadHookContext) {
        type props struct {
            Enabled *bool
        }
        p := &props{}

       icuSrcPath := "vendor/amlogic/common/external/icu"
        if android.ExistentPathForSource(ctx, icuSrcPath).Valid() == true {
            p.Enabled = proptools.BoolPtr(false)
            // fmt.Printf("icu:%s exist, use source to build\n", icuSrcPath)
        } else {
            // fmt.Printf("icu:%s not exist, use prebuilt to build\n", icuSrcPath)
        }
        ctx.AppendProperties(p)
    })
    return module
}

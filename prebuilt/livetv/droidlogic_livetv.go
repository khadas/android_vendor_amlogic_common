package droidlogic_livetv

import (
    // "fmt"
    "android/soong/android"
    "android/soong/cc"
    "github.com/google/blueprint/proptools"
)

func init() {
    android.RegisterModuleType("prebuilt_livetv_go_defaults",prebuilt_livetv_DefaultsFactory)
}

func prebuilt_livetv_DefaultsFactory() (android.Module) {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, func(ctx android.LoadHookContext) {
        type props struct {
            Enabled *bool
        }
        p := &props{}

        vconfig := ctx.Config().VendorConfig("amlogic_vendorconfig")
        if vconfig.Bool("build_livetv_from_source") == true {
            // fmt.Println("livetv: build_livetv_from_source is true, not build prebuilt")
            p.Enabled = proptools.BoolPtr(false)
        } else {
            // fmt.Println("livetv: build_livetv_from_source is false, built prebuilt")
            p.Enabled = proptools.BoolPtr(true)
        }
        ctx.AppendProperties(p)
    })

    return module
}


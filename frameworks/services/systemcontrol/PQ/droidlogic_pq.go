package droidlogic_pqcontrol

import (
    // "fmt"
    "android/soong/android"
    "android/soong/cc"
)

func init() {
    android.RegisterModuleType("pqcontrol_go_defaults", pqcontrol_go_DefaultsFactory)
}

func pqcontrol_go_DefaultsFactory() (android.Module) {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, func(ctx android.LoadHookContext) {
        type props struct {
            Shared_libs  []string
        }
        p := &props{}

        namespace := "amlogic_vendorconfig"
        vconfig := ctx.Config().VendorConfig(namespace)

        if vconfig.Bool("build_livetv") == true {
            // fmt.Println("systemcontrol:pq:build_livetv set!")
            p.Shared_libs =  append(p.Shared_libs, "libtvbinder")
        }
        ctx.AppendProperties(p)
    })
    return module
}


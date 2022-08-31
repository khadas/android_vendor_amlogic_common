package droidlogic_stagefrighthw

import (
    // "fmt"
    "android/soong/android"
    "android/soong/cc"
    "github.com/google/blueprint/proptools"
)

func init() {
    // fmt.Println("init: droidlogic_stagefrighthw")
    android.RegisterModuleType("stagefrighthw_go_defaults", stagefrighthw_go_DefaultsFactory)
    android.RegisterModuleType("stagefrighthw_ddlib_go_defaults", stagefrighthw_ddlib_go_DefaultsFactory)
    android.RegisterModuleType("stagefrighthw_videodec_go_defaults", stagefrighthw_videodec_go_DefaultsFactory)
}

func stagefrighthw_go_DefaultsFactory() (android.Module) {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, func(ctx android.LoadHookContext) {
        type props struct {
            Enabled *bool
        }
        p := &props{}

        if android.ExistentPathForSource(ctx, "vendor/omx").Valid() == true {
            p.Enabled = proptools.BoolPtr(false)
            // fmt.Println("stagefrighthw: vendor/omx exist, use source to build")
        } else {
            // fmt.Println("stagefrighthw: vendor/omx not exist, use prebuilt to build")
        }
        ctx.AppendProperties(p)
    })
    return module
}


func stagefrighthw_ddlib_go_DefaultsFactory() (android.Module) {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, func(ctx android.LoadHookContext) {
        type props struct {
            Enabled *bool
        }
        p := &props{}

        vconfig := ctx.Config().VendorConfig("amlogic_vendorconfig")
        if vconfig.Bool("ddlib_from_customer") == true {
            // fmt.Println("stagefrighthw: ddlib_from_customer is true, not build prebuilt")
            p.Enabled = proptools.BoolPtr(false)
        } else {
            // fmt.Println("stagefrighthw: ddlib_from_customer is false, built prebuilt")
            p.Enabled = proptools.BoolPtr(true)
        }
        ctx.AppendProperties(p)
    })
    return module
}


func stagefrighthw_videodec_go_DefaultsFactory() (android.Module) {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, func(ctx android.LoadHookContext) {
      type props struct {
          Enabled *bool
      }
      p := &props{}

      if android.ExistentPathForSource(ctx, "media_hal").Valid() == true {
          p.Enabled = proptools.BoolPtr(false)
          // fmt.Println("media_hal exist, use source to build")
      }
      ctx.AppendProperties(p)
    })
    return module
}


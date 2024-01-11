package droidlogic_dvb

import (
    //"fmt"
    "android/soong/android"
    "android/soong/cc"
    "github.com/google/blueprint/proptools"
)

func init() {
    android.RegisterModuleType("droidlogic_dvb_go_defaults", dvb_go_DefaultsFactory)
}

func dvb_go_DefaultsFactory() (android.Module) {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, func(ctx android.LoadHookContext) {
        type props struct {
            Enabled *bool
        }
        p := &props{}

       dvbSrcPath := "vendor/amlogic/reference/external/dvb"
       icuSrcPath := "vendor/amlogic/reference/external/icu"

       if android.ExistentPathForSource(ctx, dvbSrcPath).Valid() == true {
            p.Enabled = proptools.BoolPtr(false)
            //fmt.Printf("dvb:%s exist, use source to build\n", dvbSrcPath)
        } else if android.ExistentPathForSource(ctx, icuSrcPath).Valid() == true {
            p.Enabled = proptools.BoolPtr(false)
            //fmt.Printf("dvb:%s not exist, use prebuilt to build\n", icuSrcPath)
        }
        ctx.AppendProperties(p)
    })
    return module
}

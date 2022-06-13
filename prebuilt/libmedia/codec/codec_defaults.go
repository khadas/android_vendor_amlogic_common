package amlogic_codec

import (
    // "fmt"
    "android/soong/android"
    "android/soong/cc"
    "github.com/google/blueprint/proptools"
)

func init() {
    android.RegisterModuleType("amlogic_codec_go_defaults",codec_DefaultsFactory)
}

func codec_DefaultsFactory() (android.Module) {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, codec_Defaults)
    return module
}

func codec_Defaults(ctx android.LoadHookContext) {
    type props struct {
        Enabled *bool
    }
    p := &props{}
    p.Enabled = codec_globalDefaults(ctx)
    ctx.AppendProperties(p)
}

func codec_globalDefaults(ctx android.BaseContext) (*bool) {
    var module_enabled *bool
    if android.ExistentPathForSource(ctx, "vendor/amlogic/common/frameworks/av/AmFFmpegAdapter").Valid() == true ||
       android.ExistentPathForSource(ctx, "vendor/amlogic/common/AmFFmpegAdapter").Valid() == true {
        module_enabled = proptools.BoolPtr(false)
        // fmt.Println("avcodec:vendor/amlogic/common/AmFFmpegAdapter exist, use source to build")
    } else {
        // fmt.Println("avcodec:vendor/amlogic/common/AmFFmpegAdapter not exist, use prebuilt codec lib")
    }
    return module_enabled
}

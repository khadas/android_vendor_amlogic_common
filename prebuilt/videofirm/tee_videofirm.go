package tee_videofirm
import (
    "android/soong/android"
    "android/soong/cc"
    "github.com/google/blueprint/proptools"
    "android/soong/vendor/amlogic/common/prebuilt/libmediadrm/common"
)

func init() {
    android.RegisterModuleType("libtee_load_video_fw_go_defaults",
                                       libtee_load_video_fw_DefaultsFactory)
    android.RegisterModuleType("tee_preload_fw_go_defaults",
                                       tee_preload_fw_DefaultsFactory)
}

func libtee_load_video_fw_DefaultsFactory() (android.Module) {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, func(ctx android.LoadHookContext) {
        var props common.Props
        info := common.Information{
            FileString: "libtee_load_video_fw.so",
            SrcPath: "vendor/amlogic/common/video_firmware",
            System: false,
            MultiArch: true,
        }
        props.Enabled = proptools.BoolPtr(true)
        common.SetProps(ctx, &props, info)
        p := &props
        ctx.AppendProperties(p)
    })
    return module
}

func tee_preload_fw_DefaultsFactory() (android.Module) {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, func(ctx android.LoadHookContext) {
        var props common.Props
        info := common.Information{
            FileString: "tee_preload_fw",
            SrcPath: "vendor/amlogic/common/video_firmware",
            System: false,
            MultiArch: true,
        }
        props.Enabled = proptools.BoolPtr(true)
        common.SetProps(ctx, &props, info)
        p := &props
        ctx.AppendProperties(p)
    })
    return module
}

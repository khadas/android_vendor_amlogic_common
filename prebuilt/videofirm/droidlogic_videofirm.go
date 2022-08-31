package droidlogic_videofirm
import (
    // "fmt"
    "android/soong/android"
    "android/soong/cc"
)

func init() {
    android.RegisterModuleType("droidlogic_videofirm_go_defaults",      videofirm_go_DefaultsFactory)
}

func videofirm_go_DefaultsFactory() (android.Module) {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, func(ctx android.LoadHookContext) {
        type props struct {
            Srcs []string
        }
        p := &props{}

        vconfig := ctx.Config().VendorConfig("amlogic_vendorconfig")
        tdk_ver := vconfig.String("tdk_version")
        ta_sgn := vconfig.Bool("enable_ta_sign")
        ta_enc := vconfig.Bool("enable_ta_encrypt")
        // fmt.Printf("videofirm prebuilt: tdk_ver=%s ta_sgn=%t ta_enc=%t\n", tdk_ver, ta_sgn, ta_enc)

        if tdk_ver == "TDK38" {
            // fmt.Println("videofirm prebuilt, process tdk 3.8")
            if ta_sgn == true {
                if ta_enc == true {
                    p.Srcs = append(p.Srcs, ":526fc4fc_7ee6_ta_need_sign_v3_enc")
                } else {
                    p.Srcs = append(p.Srcs, ":526fc4fc_7ee6_ta_need_sign_v3")
                }
            } else {
                p.Srcs = append(p.Srcs, "ta/v3/526fc4fc-7ee6-4a12-96e3-83da9565bce8.ta")
            }
        } else {
            // fmt.Println("videofirm prebuilt, process tdk 2.4")
            if ta_sgn == true {
                if ta_enc == true {
                    p.Srcs = append(p.Srcs, ":526fc4fc_7ee6_ta_need_sign_enc")
                } else {
                    p.Srcs = append(p.Srcs, ":526fc4fc_7ee6_ta_need_sign")
                }
            } else {
                p.Srcs = append(p.Srcs, "ta/526fc4fc-7ee6-4a12-96e3-83da9565bce8.ta")
            }
        }

        ctx.AppendProperties(p)
    })
    return module
}





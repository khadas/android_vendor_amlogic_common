package droidlogic_wvcenc
import (
    // "fmt"
    "android/soong/android"
    "android/soong/cc"
    "github.com/google/blueprint/proptools"
)

func init() {
    android.RegisterModuleType("droidlogic_wvcenc_go_defaults",      wvcenc_go_DefaultsFactory)
    android.RegisterModuleType("droidlogic_wvcenc_genrule_defaults", wvcenc_enable_ta_DefaultsFactory)
    android.RegisterModuleType("droidlogic_wvcenc_ta_defaults",      wvcenc_sign_ta_DefaultsFactory)
}


func wvcenc_go_DefaultsFactory() (android.Module) {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, func(ctx android.LoadHookContext) {
        type props struct {
            Enabled *bool
        }
        p := &props{}

        vconfig := ctx.Config().VendorConfig("amlogic_vendorconfig")
        if vconfig.String("widevine_oemcrypto_level") != "1" {
            // fmt.Println("wvcenc: widevine_oemcrypto_level != 1, not include wvcenc prebuilt")
            p.Enabled = proptools.BoolPtr(false)
        } else {
            // fmt.Println("wvcenc: widevine_oemcrypto_level == 1 can built prebuilt")
        }
        ctx.AppendProperties(p)
    })
    return module
}

func wvcenc_enable_ta_DefaultsFactory() (android.Module) {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, func(ctx android.LoadHookContext) {
        type props struct {
            Enabled *bool
        }
        p := &props{}

        vconfig := ctx.Config().VendorConfig("amlogic_vendorconfig")
        if vconfig.String("widevine_oemcrypto_level") != "L1" || vconfig.Bool("enable_ta_sign") == false {
            // fmt.Println("wvcenc: not sign ta ")
            p.Enabled = proptools.BoolPtr(false)
        } else {
            // fmt.Println("wvcenc: should sign ta")
        }
        ctx.AppendProperties(p)
    })
    return module
}


func wvcenc_sign_ta_DefaultsFactory() (android.Module) {
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
        wv_level := vconfig.String("widevine_oemcrypto_level")
        // fmt.Printf("wvcenc prebuilt: tdk_ver=%s ta_sgn=%t ta_enc=%t wv_level=%s\n", tdk_ver, ta_sgn, ta_enc, wv_level)
        if wv_level != "L1" || ta_sgn == false {
            // fmt.Println("wvcenc: module no need to enable")
            return
        }

        if tdk_ver == "TDK38" {
            // fmt.Println("wvcenc prebuilt, process tdk 3.8")
            if ta_sgn == true {
                if ta_enc == true {
                    p.Srcs = append(p.Srcs, ":e043cde0_61d0_ta_need_sign_v3_enc")
                } else {
                    p.Srcs = append(p.Srcs, ":e043cde0_61d0_ta_need_sign_v3")
                }
            } else {
                p.Srcs = append(p.Srcs, "ta/v3/e043cde0-61d0-11e5-9c260002a5d5c51b.ta")
            }
        } else {
            // fmt.Println("wvcenc prebuilt, process tdk 2.4")
            if ta_sgn == true {
                if ta_enc == true {
                    p.Srcs = append(p.Srcs, ":e043cde0_61d0_ta_need_sign_enc")
                } else {
                    p.Srcs = append(p.Srcs, ":e043cde0_61d0_ta_need_sign")
                }
            } else {
                p.Srcs = append(p.Srcs, "e043cde0-61d0-11e5-9c260002a5d5c51b.ta")
            }
        }

        ctx.AppendProperties(p)
    })

    return module
}


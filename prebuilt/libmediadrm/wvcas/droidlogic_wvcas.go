package droidlogic_wvcas
import (
    "fmt"
    "android/soong/android"
    "android/soong/cc"
    "github.com/google/blueprint/proptools"
)

func init() {
    android.RegisterModuleType("droidlogic_wvcas_go_defaults",      wvcas_go_DefaultsFactory)
    android.RegisterModuleType("droidlogic_wvcas_genrule_defaults", wvcas_enable_ta_DefaultsFactory)
    android.RegisterModuleType("droidlogic_wvcas_ta_defaults",      wvcas_sign_ta_DefaultsFactory)
}

func wvcas_go_DefaultsFactory() (android.Module) {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, func(ctx android.LoadHookContext) {
        type props struct {
            Enabled *bool
        }
        p := &props{}

        wvcasSrcPath := "vendor/amlogic/common/widevinecas"
        if android.ExistentPathForSource(ctx, wvcasSrcPath).Valid() == true {
            p.Enabled = proptools.BoolPtr(false)
            fmt.Printf("wvcasSrcPath:%s exist, use wvcas source to build\n", wvcasSrcPath)
        } else {
            fmt.Printf("wvcasSrcPath:%s not exist, use wvcas prebuilt to build\n", wvcasSrcPath)
 
        vconfig := ctx.Config().VendorConfig("amlogic_vendorconfig")
        if vconfig.String("widevine_oemcrypto_level") != "1" {
            fmt.Println("wvcas: widevine_oemcrypto_level != 1, not include wvcas prebuilt")
            p.Enabled = proptools.BoolPtr(false)
        } else {
            fmt.Println("wvcas: widevine_oemcrypto_level == 1 can built prebuilt")
        }
        }

        ctx.AppendProperties(p)
    })
    return module
}

func wvcas_enable_ta_DefaultsFactory() (android.Module) {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, func(ctx android.LoadHookContext) {
        type props struct {
            Enabled *bool
        }
        p := &props{}

        vconfig := ctx.Config().VendorConfig("amlogic_vendorconfig")
        if vconfig.String("widevine_oemcrypto_level") != "L1" || vconfig.Bool("enable_ta_sign") == false {
            fmt.Println("wvcas: not sign ta ")
            p.Enabled = proptools.BoolPtr(false)
        } else {
            fmt.Println("wvcas: should sign ta")
        }
        ctx.AppendProperties(p)
    })
    return module
}

func wvcas_sign_ta_DefaultsFactory() (android.Module) {
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
        fmt.Printf("wvcas prebuilt: tdk_ver=%s ta_sgn=%t ta_enc=%t wv_level=%s\n", tdk_ver, ta_sgn, ta_enc, wv_level)
        if wv_level != "L1" || ta_sgn == false {
            fmt.Println("wvcas: module no need to enable")
            return
        }

        if tdk_ver == "TDK38" {
            fmt.Println("wvcas prebuilt, process tdk 3.8")
            if ta_sgn == true {
                if ta_enc == true {
                    p.Srcs = append(p.Srcs, ":e043cde0_61d0_ta_need_sign_v3_enc")
                } else {
                    p.Srcs = append(p.Srcs, ":e043cde0_61d0_ta_need_sign_v3")
                }
            } else {
                p.Srcs = append(p.Srcs, "ta/v3/e043cde0-61d0-11e5-9c26-0002a5d5c5ca.ta")
            }
        } else {
            fmt.Println("wvcas prebuilt, process tdk 2.4")
            if ta_sgn == true {
                if ta_enc == true {
                    p.Srcs = append(p.Srcs, ":e043cde0_61d0_ta_need_sign_enc")
                } else {
                    p.Srcs = append(p.Srcs, ":e043cde0_61d0_ta_need_sign")
                }
            } else {
                p.Srcs = append(p.Srcs, "ta/v2/e043cde0-61d0-11e5-9c26-0002a5d5c5ca.ta")
            }
        }

        ctx.AppendProperties(p)
    })

    return module
}
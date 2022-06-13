package droidlogic_secmem

import (
    "fmt"
    "android/soong/android"
    "android/soong/cc"
    "github.com/google/blueprint/proptools"
)

func init() {
    android.RegisterModuleType("droidlogic_secmem_go_defaults", secmem_go_DefaultsFactory)
    android.RegisterModuleType("droidlogic_secmem_genrule_defaults", secmem_enable_ta_DefaultsFactory)
    android.RegisterModuleType("droidlogic_secmem_ta_defaults", secmem_sign_ta_DefaultsFactory)
}

func secmem_go_DefaultsFactory() (android.Module) {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, func(ctx android.LoadHookContext) {
        type props struct {
            Enabled *bool
        }
        p := &props{}

        libsecmemSrcPath := "vendor/amlogic/common/libsecmem"
        if android.ExistentPathForSource(ctx, libsecmemSrcPath).Valid() == true {
            p.Enabled = proptools.BoolPtr(false)
            fmt.Printf("libsecmemSrcPath:%s exist, use libsecmem source to build\n", libsecmemSrcPath)
        } else {
            fmt.Printf("libsecmemSrcPath:%s not exist, use secmem prebuilt to build\n", libsecmemSrcPath)
        }
        ctx.AppendProperties(p)
    })
    return module
}

func secmem_enable_ta_DefaultsFactory() (android.Module) {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, func(ctx android.LoadHookContext) {
        type props struct {
            Enabled *bool
        }
        p := &props{}

        vconfig := ctx.Config().VendorConfig("amlogic_vendorconfig")
        if vconfig.Bool("omx_with_optee_tvp") == false || vconfig.Bool("enable_ta_sign") == false {
            // fmt.Println("secmem: not sign ta or optee tvp")
            p.Enabled = proptools.BoolPtr(false)
        } else {
            // fmt.Println("secmem: should sign ta")
        }
        ctx.AppendProperties(p)
    })
    return module
}

func secmem_sign_ta_DefaultsFactory() (android.Module) {
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
        with_tvp := vconfig.Bool("omx_with_optee_tvp")
        fmt.Printf("secmem prebuilt: tdk_ver=%s ta_sgn=%t ta_enc=%t with_tvp=%t\n", tdk_ver, ta_sgn, ta_enc, with_tvp)

        // module not enable, do nothing!
        if vconfig.Bool("omx_with_optee_tvp") == false || vconfig.Bool("enable_ta_sign") == false {
            fmt.Println("secmem: not sign ta or optee tvp")
            return
        }

        if tdk_ver == "TDK38" {
            // fmt.Println("secmem prebuilt, process tdk 3.8")
            if ta_sgn == true {
                if ta_enc == true {
                    p.Srcs = append(p.Srcs, ":2c1a33c0_4_ta_need_sign_v3_enc")
                } else {
                    p.Srcs = append(p.Srcs, ":2c1a33c0_4_ta_need_sign_v3")
                }
            } else {
                p.Srcs = append(p.Srcs, "ta/v3/2c1a33c0-44cc-11e5-bc3b0002a5d5c51b.ta")
            }
        } else {
            // fmt.Println("secmem prebuilt, process tdk 2.4")
            if ta_sgn == true {
                if ta_enc == true {
                    p.Srcs = append(p.Srcs, ":2c1a33c0_4_ta_need_sign_enc")
                } else {
                    p.Srcs = append(p.Srcs, ":2c1a33c0_4_ta_need_sign")
                }
            } else {
                p.Srcs = append(p.Srcs, "2c1a33c0-44cc-11e5-bc3b0002a5d5c51b.ta")
            }
        }

        ctx.AppendProperties(p)
    })

    return module
}


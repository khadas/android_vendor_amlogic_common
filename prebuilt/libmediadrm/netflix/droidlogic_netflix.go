package droidlogic_netflix
import (
    // "fmt"
    "android/soong/android"
    "android/soong/cc"
    "github.com/google/blueprint/proptools"
)

func init() {
    android.RegisterModuleType("droidlogic_netflix_go_defaults",      netflix_go_DefaultsFactory)
    android.RegisterModuleType("droidlogic_netflix_genrule_defaults", netflix_enable_ta_DefaultsFactory)
    android.RegisterModuleType("droidlogic_netflix_ta_defaults",      netflix_sign_ta_DefaultsFactory)
}


func netflix_go_DefaultsFactory() (android.Module) {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, func(ctx android.LoadHookContext) {
        type props struct {
            Enabled *bool
        }
        p := &props{}

        vconfig := ctx.Config().VendorConfig("amlogic_vendorconfig")
        if vconfig.Bool("netflix_mgkid") == false {
            // fmt.Println("netflix: netflix_mgkid is false, not include netflix prebuilt")
            p.Enabled = proptools.BoolPtr(false)
        } else {
            // fmt.Println("netflix: netflix_mgkid is true, can built prebuilt")
        }
        ctx.AppendProperties(p)
    })
    return module
}

func netflix_enable_ta_DefaultsFactory() (android.Module) {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, func(ctx android.LoadHookContext) {
        type props struct {
            Enabled *bool
        }
        p := &props{}

        vconfig := ctx.Config().VendorConfig("amlogic_vendorconfig")
        if vconfig.Bool("netflix_mgkid") == false || vconfig.Bool("enable_ta_sign") == false {
            // fmt.Println("netflix: not sign ta ")
            p.Enabled = proptools.BoolPtr(false)
        } else {
            // fmt.Println("netflix: should sign ta")
        }
        ctx.AppendProperties(p)
    })
    return module
}


func netflix_sign_ta_DefaultsFactory() (android.Module) {
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
        has_mgkid := vconfig.Bool("netflix_mgkid")
        // fmt.Printf("netflix prebuilt: tdk_ver=%s ta_sgn=%t ta_enc=%t has_mgkid=%t\n", tdk_ver, ta_sgn, ta_enc, has_mgkid)

        // module not enable, do nothing!
        if has_mgkid == false || ta_sgn == false {
            // fmt.Println("netflix: not sign ta or has mgkid")
            return
        }

        if tdk_ver == "TDK38" {
            // fmt.Println("netflix prebuilt, process tdk 3.8")
            if ta_sgn == true {
                if ta_enc == true {
                    p.Srcs = append(p.Srcs, ":00d1ca22_1764_4e35_ta_need_sign_v3_enc")
                } else {
                    p.Srcs = append(p.Srcs, ":00d1ca22_1764_4e35_ta_need_sign_v3")
                }
            } else {
                p.Srcs = append(p.Srcs, "ta/v3/00d1ca22-1764-4e35-90aa5b8c12630764.ta")
            }
        } else {
            // fmt.Println("netflix prebuilt, process tdk 2.4")
            if ta_sgn == true {
                if ta_enc == true {
                    p.Srcs = append(p.Srcs, ":00d1ca22_1764_4e35_ta_need_sign_enc")
                } else {
                    p.Srcs = append(p.Srcs, ":00d1ca22_1764_4e35_ta_need_sign")
                }
            } else {
                p.Srcs = append(p.Srcs, "00d1ca22-1764-4e35-90aa5b8c12630764.ta")
            }
        }

        ctx.AppendProperties(p)
    })

    return module
}


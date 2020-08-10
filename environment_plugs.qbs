import qbs
import qbs.Environment
import "BuildHelpers.js" as Funcs
Product{
    name: "Environment"
    Export {
        Depends { name: "cpp" }

        cpp.debugInformation: true
        cpp.defines: {
            var defines = []
            return base.concat(defines)
        }



        cpp.cFlags: {
            var flags = ["-MD","-zi"]
            if(qbs.buildVariant === "release")
                flags = flags.concat(["-O2"])
            else
                flags = flags.concat(["-O0"])
            return flags
        }
        cpp.cxxFlags: {
            var flags = base
            if(qbs.toolchain.contains("msvc"))
                flags = flags.concat(["/EHsc"])
            else
                flags = flags.concat([])
            return flags
        }
        Properties{
            condition: qbs.toolchain.contains("msvc")
            cpp.linkerFlags: {
                var libs = ["/DEBUG", "/INCREMENTAL:NO", "/ignore:4221", "/ignore:4099","/ignore:4075"]
                return libs;
            }
        }
        property bool usePrecompiledHeader : Funcs.getEnvOrDie("USE_PRECOMPILED_HEADER") == "1"
    }
}

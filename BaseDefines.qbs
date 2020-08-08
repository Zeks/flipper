import qbs
import qbs.Process

Product{
    Depends { name: "localvariables" }
    cpp.debugInformation: true
    cpp.cxxLanguageVersion: "c++17"
    localvariables.debugAppend : qbs.buildVariant == "debug" ? "d" : ""

    
    cpp.libraryPaths:  {
        var path = []
        if(qbs.toolchain.contains("msvc"))
        {
            path = localvariables.projectPath
            path+= qbs.buildVariant == "release" ? "/release" : "/debug"
        }
        else
        {
            var addition = qbs.buildVariant == "release" ? "/release" : "/debug";
            path= path.concat(localvariables.projectPath + addition)
            path= path.concat("/home/zekses/Downloads/grpc/libs/opt")
            //path= path.concat("/home/zekses/Downloads/flipper/Run")
            path= path.concat("/home/zekses/Downloads/grpc/third_party/protobuf/src/.libs")
        }
        return path
    }
    cpp.cxxFlags: ["-Wno-unused-function"]
    destinationDirectory: {
        var path = localvariables.projectPath
        path += qbs.buildVariant == "release" ? "/release" : "/debug"
        return path
    }
    cpp.defines: ["MAJOR_PROTOCOL_VERSION=2", "MINOR_PROTOCOL_VERSION=0","NONEXISTENT_OPUS"]
}

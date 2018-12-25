import qbs
import qbs.Process

Product{
    Depends { name: "conditionals" }
    cpp.debugInformation: true
    cpp.cxxLanguageVersion: "c++17"
    conditionals.debugAppend : qbs.buildVariant == "debug" ? "d" : ""

    
    cpp.libraryPaths:  {
        var path = []
        if(qbs.toolchain.contains("msvc"))
        {
            path = conditionals.projectPath
            path+= qbs.buildVariant == "release" ? "/release" : "/debug"
        }
        else
        {
            var addition = qbs.buildVariant == "release" ? "/release" : "/debug";
            path= path.concat(conditionals.projectPath + addition)
            path= path.concat("/home/zeks/grpc/libs/opt")
            //path= path.concat("/home/zeks/flipper/Run")
            path= path.concat("/home/zeks/grpc/third_party/protobuf/src/.libs")
        }
        return path
    }
    destinationDirectory: {
        var path = conditionals.projectPath
        path += qbs.buildVariant == "release" ? "/release" : "/debug"
        return path
    }
    cpp.defines: ["PROTOCOL_VERSION=1"]
}

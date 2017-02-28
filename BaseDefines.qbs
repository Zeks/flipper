import qbs
import qbs.Process

Product{
    Depends { name: "conditionals" }
    cpp.debugInformation: true
    cpp.cxxLanguageVersion: "c++11"
    conditionals.debugAppend : qbs.buildVariant == "debug" ? "d" : ""
    cpp.libraryPaths:  {
        var path = conditionals.projectPath
        path+= qbs.buildVariant == "release" ? "/release" : "/debug"
        print("libpath: " + path);
        return path
    }
    destinationDirectory: {
         var path = conditionals.projectPath
         path+= qbs.buildVariant == "release" ? "/Run" : "/Run"
         print("destdir1: " + path);
         return path
    }
}

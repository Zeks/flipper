import qbs 1.0
import qbs.Process
import "BaseDefines.qbs" as App


App{
    name: "discord"
    consoleApplication:false
    type:"application"
    qbsSearchPaths: [sourceDirectory + "/modules", sourceDirectory + "/repo_modules"]
    Depends { name: "Qt.core"}
    Depends { name: "Qt.sql" }
    Depends { name: "Qt.core" }

    Depends { name: "Qt.network" }

    Depends { name: "Qt.concurrent" }

    Depends { name: "cpp" }
    Depends { name: "logger" }

    Depends { name: "proto_generation" }
    Depends { name: "grpc_generation" }

    cpp.defines: base.concat(["L_LOGGER_LIBRARY"])
    cpp.includePaths: [
        sourceDirectory,
        sourceDirectory + "/../",
        sourceDirectory + "/include",
        sourceDirectory + "/libs",
        sourceDirectory + "/libs/Logger/include",
        sourceDirectory + "/third_party/sleepy-discord/include",
        sourceDirectory + "/third_party/sleepy-discord/deps",
        sourceDirectory + "/third_party/sleepy-discord/deps/asio/asio/include",
        sourceDirectory + "/third_party/sleepy-discord/deps/websocketpp",
        sourceDirectory + "/third_party/sleepy-discord/deps/cpr/include",



    ]
    cpp.minimumWindowsVersion: "6.0"
    files: [
        "src/main_discord.cpp",
    ]

    cpp.staticLibraries: {
        var libs = []
        
            libs = ["logger", "dl", "protobuf", "sleepy-discord", "cpr", "curl", "crypto", "ssl"]
        libs = libs.concat(conditionals.zlib)
        libs = libs.concat(conditionals.ssl)
        libs = libs.concat(["grpc", "grpc++", "gpr"])
        return libs
    }

}

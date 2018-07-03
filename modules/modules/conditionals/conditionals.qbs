import qbs
Module {
	Depends { name: "cpp" }
    property string debugAppend: ""
    property bool logger : true
    property bool grpc: true
    property bool usePrecompiledHeader: true

    property string projectPath: qbs.toolchain.contains("msvc") ? "E:/Programming/Flipper" : "~/flipper"
    property string protoc: qbs.toolchain.contains("msvc") ? "E:/Programming/protobuf/protoc.exe":"~/grpc/bins/opt/protobuf/protoc"
    property string grpcPlugin: qbs.toolchain.contains("msvc") ?"E:/Programming/protobuf/grpc_cpp_plugin.exe": "~/grpc/bins/opt/grpc_cpp_plugin"

    property string protobufName: "libprotobufd"
    property stringList zlib : qbs.toolchain.contains("msvc") ? ["zlibd", "cares", "crypto"] : []
    property stringList ssl : qbs.toolchain.contains("msvc") ? ["ssl"] : []

}

import qbs
Module {
	Depends { name: "cpp" }
    property string debugAppend: ""

    property string projectPath: "E:/Programming/Flipper"
    property string protoc: "E:/Programming/protobuf/protoc.exe"
    property string grpcPlugin: "E:/Programming/protobuf/grpc_cpp_plugin.exe"

    property bool logger : true
    property bool grpc: true
    property bool usePrecompiledHeader: true

    property string protobufName: "libprotobufd"
    property stringList zlib : ["zlibd", "cares", "crypto"]
    property stringList ssl : ["ssl"]
}

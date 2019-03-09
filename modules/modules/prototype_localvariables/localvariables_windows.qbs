import qbs
Module {
	Depends { name: "cpp" }
    property string debugAppend: ""
    property bool logger : true
    property bool grpc: true
    property bool usePrecompiledHeader: true

    property string projectPath:  "F:/Programming/Flipper" 
    property string protoc: "E:/Programming/protobuf/protoc.exe"
    property string grpcPlugin: "E:/Programming/protobuf/grpc_cpp_plugin.exe"

    property string protobufName: "libprotobufd"
    property stringList zlib : ["zlib", "cares", "crypto"] 
    property stringList ssl : ["ssl"] 

}

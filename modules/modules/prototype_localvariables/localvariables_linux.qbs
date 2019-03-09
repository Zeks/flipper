import qbs
Module {
	Depends { name: "cpp" }
    property string debugAppend: ""
    property bool logger : true
    property bool grpc: true
    property bool usePrecompiledHeader: true

    property string projectPath:  "/home/zekses/flipper"
    property string protoc: "/home/zekses/Downloads/grpc/bins/opt/protobuf/protoc"
    property string grpcPlugin: "/home/zekses/Downloads/grpc/bins/opt/grpc_cpp_plugin"

    property string protobufName: "libprotobufd"
    property stringList zlib :  []
    property stringList ssl : []

}

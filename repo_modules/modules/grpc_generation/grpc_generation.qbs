import qbs
import qbs.FileInfo
import qbs.File
import qbs.Utilities

Module {
    property string rootDir: ""
    property var toolchain: []
    property string generationDir: ""
    property string protobufDependencyDir: ""

    Rule {
        inputs: ["grpc"]
        Artifact {
            filePath: FileInfo.path(input.filePath)  + '/' + FileInfo.baseName(input.fileName) + '.grpc.pb.cc'
            fileTags: "cpp"
            cpp.defines: product.moduleProperty('cpp', 'defines').concat(["_SCL_SECURE_NO_WARNINGS"])
            cpp.cxxFlags: {
                var flags = []
                if(!input.moduleProperty('grpc_generation', 'toolchain').contains("msvc"))
                    flags = [ "-Wno-unused-variable", "-Wno-unused-parameter"]
                else
                    flags = [ "/w"]
                return flags
            }
        }

        prepare: {
            var protoc = product.moduleProperty('conditionals', 'protoc');

            // use of canonicalFilePath is discourage because it might break on symlinks
            var rootDir = File.canonicalFilePath(input.moduleProperty('grpc_generation', 'rootDir'))
            //console.error("roo dir: " + rootDir)
            var generationDir = File.canonicalFilePath(input.moduleProperty('grpc_generation', 'generationDir'))
            var pbDependencyDir = File.canonicalFilePath(input.moduleProperty('grpc_generation', 'protobufDependencyDir'))
            if(generationDir === "")
                generationDir = rootDir
            //console.error("dep dir: " + pbDependencyDir)
            var gprcArgs = [];
            gprcArgs = gprcArgs.concat(["--proto_path=" + rootDir]);
            gprcArgs = gprcArgs.concat(["--proto_path=" + pbDependencyDir]);
            gprcArgs = gprcArgs.concat(["--grpc_out=" + generationDir]);
            gprcArgs = gprcArgs.concat(["--plugin=protoc-gen-grpc=" + product.moduleProperty('conditionals', 'grpcPlugin') ]);
            gprcArgs = gprcArgs.concat([input.filePath]);

            var grpcCommand = new Command(protoc, gprcArgs);
            grpcCommand.description = 'Generating service from: ' + input.fileName;
            //grpcCommand.description = 'Generating service from: ' + rootDir + " pb: " + pbDependencyDir;

            return [grpcCommand];
        }
    }
}


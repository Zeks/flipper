import qbs
import qbs.FileInfo
import qbs.File
import qbs.Utilities
import qbs.Environment
import "../../../BuildHelpers.js" as Funcs

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

        Artifact {
            filePath: FileInfo.path(input.filePath)  + '/' + FileInfo.baseName(input.fileName) + '.grpc.pb.h'
        }

        prepare: {
            var protoc = Funcs.getEnvOrDie("PROTOC")

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
            var grpcPluginPath = Funcs.getEnvOrDie("GRPC_PLUGIN")
            gprcArgs = gprcArgs.concat(["--plugin=protoc-gen-grpc=" + grpcPluginPath]);
            gprcArgs = gprcArgs.concat([input.filePath]);

            var grpcCommand = new Command(protoc, gprcArgs);
            grpcCommand.description = 'protoc-grpc ' + input.fileName;

            return [grpcCommand];
        }
    }
}


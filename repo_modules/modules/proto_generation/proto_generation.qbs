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
        inputs: ["proto"]
        Artifact {
            filePath: FileInfo.path(input.filePath)  + '/' + FileInfo.baseName(input.fileName) + '.pb.cc'
            fileTags: "cpp"
            cpp.defines: product.moduleProperty('cpp', 'defines').concat(["_SCL_SECURE_NO_WARNINGS"])
            cpp.cxxFlags: {
                var flags = []
                if(!input.moduleProperty('proto_generation', 'toolchain').contains("msvc"))
                    flags = [ "-Wno-unused-variable", "-Wno-unused-parameter"]
                else
                    flags = [ "/w"]
                return flags
            }
        }

        Artifact {
            filePath: FileInfo.path(input.filePath)  + '/' + FileInfo.baseName(input.fileName) + '.pb.h'
        }

        prepare: {
            var protoc = Funcs.getEnvOrDie("PROTOC")

            // use of canonicalFilePath is discourage because it might break on symlinks

            var rootDir = File.canonicalFilePath(input.moduleProperty('proto_generation', 'rootDir'))
            var generationDir = File.canonicalFilePath(input.moduleProperty('proto_generation', 'generationDir'))
            var pbDependencyDir = File.canonicalFilePath(input.moduleProperty('proto_generation', 'protobufDependencyDir'))
            if(generationDir === "")
                generationDir = rootDir

            var protoArgs = [];
            protoArgs = protoArgs.concat(["--proto_path=" + rootDir]);
            protoArgs = protoArgs.concat(["--proto_path=" + pbDependencyDir]);
            protoArgs = protoArgs.concat(["--cpp_out=" + generationDir]);
            protoArgs = protoArgs.concat(input.filePath);

            //console.error("rdir: " + rootDir)
//            console.error("relativeDir: " + FileInfo.path(input.fileName))
//            console.error(protoArgs.join())

            var protoCommand = new Command(protoc, protoArgs);
            protoCommand.description = 'protoc-cpp ' + input.fileName;

            return [protoCommand];
        }
    }
}

import qbs
import "../../BuildHelpers.js" as Funcs
import "../../BaseDefines.qbs" as Application
Module {
    FileTagger {
        patterns: ["*.branch.dummy"] // bundle
        fileTags: ["branch.dummy"]
    }
    Rule {
        inputs: ["branch.dummy"]
        Artifact {
            filePath: "dummy"
            fileTags: "branchSetter"
        }
        prepare: {
            var cmd = new Command(inputs["application"][0].filePath);
            cmd.description = "Calling application that runs forever";
            return cmd;
        }
    }
}


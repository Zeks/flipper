import qbs
    Group
    {
        name: "pch"
        files: {
            var filepath = ""
            var filename = ""

            if(projecttype.useGuiLib)
                filename = "include/pch.h"
            else
                filename = "include/pch_core.h"
            return [filepath + filename]
        }
        fileTags: ["cpp_pch_src"]
    }

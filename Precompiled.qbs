import qbs
    Group
    {
        name: "pch"
        files: {
            var filepath = ""
            var filename = ""

            if(projecttype.useGuiLib)
                filename = "include/precompiled/pch.h"
            else
                filename = "include/precompiled/pch_core.h"
            return [filepath + filename]
        }
        fileTags: ["cpp_pch_src"]
    }

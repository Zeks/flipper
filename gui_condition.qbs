import qbs 1.0
Product{
name: "projecttype"
Export {
        Depends { name: "cpp" }
        property bool useGuiLib: true
    }
}

var Process = require("qbs.Process");
var Environment = require("qbs.Environment");

function getEnvOrDie(name) {
    var result = Environment.getEnv(name)
    if (!result)
        throw "Environment variable '" + name + "' undefined"
    return result
}


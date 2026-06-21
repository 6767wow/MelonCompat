#pragma once

#include <string>

namespace rosev
{
    struct CompileOptions
    {
        std::string inputPath;
        std::string outputPath;
        std::string forcedNamespace;
        std::string forcedClassName;
        bool writeDebugComments = true;
    };

    int Compile(const CompileOptions& options, std::string& error);
}

#include "rosev_compiler.h"

#include <iostream>
#include <string>

namespace
{
    void PrintHelp()
    {
        std::cout
            << "RoseV native compiler\n\n"
            << "Usage:\n"
            << "  RoseV.exe compile <input.rosev> -o <output.cs> [--namespace Name] [--class Name]\n\n"
            << "RoseV is a separate source language for Unity mods. This compiler is native C/C++/ASM\n"
            << "and currently emits C# for the Unity/MelonLoader/BepInEx/RoseMod backend.\n";
    }

    bool NeedValue(int index, int argc, const std::string& option, std::string& error)
    {
        if (index + 1 < argc)
            return true;

        error = "Missing value for " + option + ".";
        return false;
    }
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        PrintHelp();
        return 0;
    }

    const std::string command = argv[1];
    if (command == "--help" || command == "-h" || command == "help")
    {
        PrintHelp();
        return 0;
    }

    if (command != "compile")
    {
        std::cerr << "Unknown command: " << command << "\n";
        PrintHelp();
        return 2;
    }

    rosev::CompileOptions options;
    std::string error;

    if (argc < 3)
    {
        std::cerr << "Missing input .rosev file.\n";
        return 2;
    }

    options.inputPath = argv[2];

    for (int i = 3; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (arg == "-o" || arg == "--out")
        {
            if (!NeedValue(i, argc, arg, error))
                break;
            options.outputPath = argv[++i];
        }
        else if (arg == "--namespace")
        {
            if (!NeedValue(i, argc, arg, error))
                break;
            options.forcedNamespace = argv[++i];
        }
        else if (arg == "--class")
        {
            if (!NeedValue(i, argc, arg, error))
                break;
            options.forcedClassName = argv[++i];
        }
        else if (arg == "--no-debug-comments")
        {
            options.writeDebugComments = false;
        }
        else
        {
            error = "Unknown option: " + arg;
            break;
        }
    }

    if (!error.empty())
    {
        std::cerr << error << "\n";
        return 2;
    }

    if (options.outputPath.empty())
    {
        std::cerr << "Missing output path. Use -o <output.cs>.\n";
        return 2;
    }

    const int result = rosev::Compile(options, error);
    if (result != 0)
    {
        std::cerr << error << "\n";
        return result;
    }

    std::cout << "RoseV compiled " << options.inputPath << " -> " << options.outputPath << "\n";
    return 0;
}

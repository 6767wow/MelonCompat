#include "rosev_compiler.h"
#include "rosev_lexer.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <vector>

namespace rosev
{
    namespace
    {
        struct SourceLine
        {
            int number = 0;
            std::string text;
        };

        struct Setting
        {
            std::string name;
            std::string type;
            std::string defaultValue;
            std::string description;
        };

        struct Field
        {
            std::string name;
            std::string type;
            std::string defaultValue;
        };

        struct Import
        {
            std::string name;
            std::string condition;
        };

        struct NativeUnit
        {
            std::string language;
            std::string path;
            std::string alias;
        };

        struct NativeCall
        {
            std::string alias;
            std::string functionName;
        };

        enum class StatementKind
        {
            Say,
            Warn,
            Error,
            Emit,
            CSharp,
            Unity,
            Let,
            Set,
            Add,
            Subtract,
            Multiply,
            Divide,
            Call,
            If,
            ElseIf,
            Repeat,
            While,
            Return,
            Throw,
            Try,
            Every,
            Key,
            NativeCall
        };

        struct Statement
        {
            StatementKind kind = StatementKind::Say;
            int line = 0;
            int numberValue = 0;
            int counterIndex = -1;
            std::string value;
            std::string value2;
            std::vector<Statement> children;
        };

        struct EventBlock
        {
            std::string name;
            std::string parameter;
            int line = 0;
            std::vector<Statement> statements;
        };

        struct FunctionBlock
        {
            std::string name;
            int line = 0;
            std::vector<Statement> statements;
        };

        struct Program
        {
            std::string name = "RoseV Mod";
            std::string id = "rosev.mod";
            std::string version = "0.1.0";
            std::string author = "";
            std::string description = "";
            std::string nameSpace = "RoseVGenerated";
            std::string className = "RoseVGeneratedMod";
            bool useUnity = false;
            bool useMelonLoader = true;
            bool useBepInEx = true;
            bool useRoseMod = true;
            uint64_t sourceHash = 0;
            std::vector<Import> imports;
            std::vector<NativeUnit> nativeUnits;
            std::vector<std::string> memberBlocks;
            std::vector<Field> fields;
            std::vector<Setting> settings;
            std::vector<EventBlock> events;
            std::vector<FunctionBlock> functions;
        };

        std::string Trim(const std::string& value)
        {
            std::vector<char> buffer(value.begin(), value.end());
            buffer.push_back('\0');
            rosev_trim_in_place(buffer.data());
            return std::string(buffer.data());
        }

        bool StartsWith(const std::string& value, const std::string& prefix)
        {
            return rosev_starts_with(value.c_str(), prefix.c_str()) != 0;
        }

        std::string StripComment(const std::string& value)
        {
            const auto trimmed = Trim(value);
            if (StartsWith(trimmed, "#if") ||
                StartsWith(trimmed, "#else") ||
                StartsWith(trimmed, "#elif") ||
                StartsWith(trimmed, "#endif") ||
                StartsWith(trimmed, "#define") ||
                StartsWith(trimmed, "#undef") ||
                StartsWith(trimmed, "#nullable") ||
                StartsWith(trimmed, "#pragma") ||
                StartsWith(trimmed, "#region") ||
                StartsWith(trimmed, "#endregion") ||
                StartsWith(trimmed, "#line") ||
                StartsWith(trimmed, "#warning") ||
                StartsWith(trimmed, "#error"))
                return value;

            bool inString = false;
            bool escaped = false;
            for (size_t i = 0; i < value.size(); ++i)
            {
                const char ch = value[i];
                if (escaped)
                {
                    escaped = false;
                    continue;
                }

                if (ch == '\\')
                {
                    escaped = true;
                    continue;
                }

                if (ch == '"')
                {
                    inString = !inString;
                    continue;
                }

                if (!inString && ch == '#')
                    return value.substr(0, i);
            }

            return value;
        }

        std::string ToLower(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            });
            return value;
        }

        bool EndsWithBrace(const std::string& value)
        {
            const auto trimmed = Trim(value);
            return !trimmed.empty() && trimmed.back() == '{';
        }

        std::string RemoveTrailingBrace(const std::string& value)
        {
            auto trimmed = Trim(value);
            if (!trimmed.empty() && trimmed.back() == '{')
            {
                trimmed.pop_back();
                trimmed = Trim(trimmed);
            }
            return trimmed;
        }

        int BraceDelta(const std::string& value)
        {
            bool inString = false;
            bool inChar = false;
            bool escaped = false;
            int delta = 0;
            for (const char ch : value)
            {
                if (escaped)
                {
                    escaped = false;
                    continue;
                }

                if (ch == '\\')
                {
                    escaped = true;
                    continue;
                }

                if (!inChar && ch == '"')
                {
                    inString = !inString;
                    continue;
                }

                if (!inString && ch == '\'')
                {
                    inChar = !inChar;
                    continue;
                }

                if (inString || inChar)
                    continue;

                if (ch == '{')
                    ++delta;
                else if (ch == '}')
                    --delta;
            }

            return delta;
        }

        bool IsRawBlockStart(const std::string& text, const std::string& command)
        {
            return text == command + " {" || text == command + "{" || StartsWith(text, command + " {");
        }

        bool TryParseSynvert(const std::string& text, std::string& mode)
        {
            auto header = RemoveTrailingBrace(text);
            const auto equals = header.find('=');
            if (equals == std::string::npos)
                return false;

            const auto name = ToLower(Trim(header.substr(0, equals)));
            if (name != "synvert")
                return false;

            mode = ToLower(Trim(header.substr(equals + 1)));
            return !mode.empty();
        }

        bool IsRoseVSynvertMode(const std::string& mode)
        {
            return mode == "rosev" || mode == "rose" || mode == "easy";
        }

        bool IsCSharpSynvertMode(const std::string& mode)
        {
            return mode == "csharp" || mode == "cs" || mode == "c#";
        }

        bool IsUnitySynvertMode(const std::string& mode)
        {
            return mode == "unity" || mode == "unity-csharp" || mode == "unity_cs";
        }

        bool IsLoaderSynvertMode(const std::string& mode)
        {
            return IsUnitySynvertMode(mode) ||
                mode == "melonloader" ||
                mode == "melon" ||
                mode == "bepinex" ||
                mode == "rosemod" ||
                mode == "il2cpp" ||
                mode == "harmony";
        }

        std::string ReadQuoted(const std::string& value, size_t start, int lineNumber)
        {
            const auto first = value.find('"', start);
            if (first == std::string::npos)
                throw std::runtime_error("Line " + std::to_string(lineNumber) + ": expected quoted text.");

            std::ostringstream result;
            bool escaped = false;
            for (size_t i = first + 1; i < value.size(); ++i)
            {
                const char ch = value[i];
                if (escaped)
                {
                    switch (ch)
                    {
                    case 'n':
                        result << '\n';
                        break;
                    case 't':
                        result << '\t';
                        break;
                    case '"':
                    case '\\':
                        result << ch;
                        break;
                    default:
                        result << ch;
                        break;
                    }
                    escaped = false;
                    continue;
                }

                if (ch == '\\')
                {
                    escaped = true;
                    continue;
                }

                if (ch == '"')
                    return result.str();

                result << ch;
            }

            throw std::runtime_error("Line " + std::to_string(lineNumber) + ": quoted text was not closed.");
        }

        std::string ReadNamedQuoted(const std::string& value, const std::string& name, const std::string& fallback, int lineNumber)
        {
            const auto token = " " + name + " ";
            auto index = value.find(token);
            if (index == std::string::npos)
            {
                if (StartsWith(value, name + " "))
                    index = 0;
                else
                    return fallback;
            }

            return ReadQuoted(value, index + token.size(), lineNumber);
        }

        std::vector<std::string> SplitWords(const std::string& value)
        {
            std::vector<std::string> words;
            std::istringstream stream(value);
            std::string word;
            while (stream >> word)
                words.push_back(word);
            return words;
        }

        std::string SanitizeIdentifier(const std::string& value, const std::string& fallback)
        {
            std::string result;
            for (const unsigned char ch : value)
            {
                if (std::isalnum(ch) || ch == '_')
                    result.push_back(static_cast<char>(ch));
                else if (ch == '.' || ch == '-' || std::isspace(ch))
                    result.push_back('_');
            }

            while (!result.empty() && result.front() == '_')
                result.erase(result.begin());

            if (result.empty())
                result = fallback;

            if (std::isdigit(static_cast<unsigned char>(result.front())))
                result = "_" + result;

            return result;
        }

        std::string SanitizeNamespace(const std::string& value)
        {
            std::ostringstream output;
            std::string part;
            std::istringstream stream(value);
            bool first = true;
            while (std::getline(stream, part, '.'))
            {
                if (!first)
                    output << ".";
                output << SanitizeIdentifier(part, "Generated");
                first = false;
            }

            return output.str().empty() ? "RoseVGenerated" : output.str();
        }

        std::string EscapeCSharpString(const std::string& value)
        {
            std::ostringstream output;
            output << '"';
            for (const char ch : value)
            {
                switch (ch)
                {
                case '\\':
                    output << "\\\\";
                    break;
                case '"':
                    output << "\\\"";
                    break;
                case '\n':
                    output << "\\n";
                    break;
                case '\r':
                    output << "\\r";
                    break;
                case '\t':
                    output << "\\t";
                    break;
                default:
                    output << ch;
                    break;
                }
            }
            output << '"';
            return output.str();
        }

        std::string RoseTypeToCSharp(const std::string& type)
        {
            const auto lower = ToLower(type);
            if (lower == "var" || lower == "auto")
                return "var";
            if (lower == "bool" || lower == "boolean")
                return "bool";
            if (lower == "int" || lower == "integer")
                return "int";
            if (lower == "long")
                return "long";
            if (lower == "short")
                return "short";
            if (lower == "byte")
                return "byte";
            if (lower == "float" || lower == "number")
                return "float";
            if (lower == "double")
                return "double";
            if (lower == "string" || lower == "text")
                return "string";
            if (lower == "object" || lower == "any")
                return "object";

            throw std::runtime_error("Unknown RoseV setting type: " + type);
        }

        std::string RoseValueToCSharp(const std::string& type, const std::string& value)
        {
            const auto csharpType = RoseTypeToCSharp(type);
            if (csharpType == "bool")
            {
                const auto lower = ToLower(value);
                return (lower == "true" || lower == "yes" || lower == "on") ? "true" : "false";
            }

            if (csharpType == "int")
                return value.empty() ? "0" : value;

            if (csharpType == "float")
            {
                auto number = value.empty() ? "0" : value;
                if (number.find('.') == std::string::npos)
                    number += ".0";
                return number + "f";
            }

            if (csharpType == "double")
                return value.empty() ? "0.0" : value;

            if (csharpType == "long")
                return value.empty() ? "0L" : value + "L";

            if (csharpType == "short" || csharpType == "byte")
                return value.empty() ? "0" : value;

            if (csharpType == "string")
            {
                if (!value.empty() && value.front() == '"')
                    return value;
                return EscapeCSharpString(value);
            }

            return EscapeCSharpString(value);
        }

        std::string CleanImportName(std::string value)
        {
            value = Trim(value);
            if (!value.empty() && value.back() == ';')
                value.pop_back();
            return Trim(value);
        }

        std::string BaseName(const std::string& path)
        {
            const auto slash = path.find_last_of("/\\");
            return slash == std::string::npos ? path : path.substr(slash + 1);
        }

        void AddImport(std::vector<Import>& imports, const std::string& name, const std::string& condition = "")
        {
            const auto cleaned = CleanImportName(name);
            if (cleaned.empty())
                return;

            imports.push_back({ cleaned, condition });
        }

        bool TryAddExplicitImport(std::vector<Import>& imports, const std::string& value)
        {
            for (const auto& marker : { std::string(" when "), std::string(" if ") })
            {
                const auto markerIndex = value.find(marker);
                if (markerIndex == std::string::npos)
                    continue;

                const auto name = Trim(value.substr(0, markerIndex));
                const auto condition = Trim(value.substr(markerIndex + marker.size()));
                if (name.empty() || condition.empty())
                    throw std::runtime_error("Expected import <Namespace> when <SYMBOL>.");

                AddImport(imports, name, condition);
                return true;
            }

            return false;
        }

        void AddImportPack(std::vector<Import>& imports, const std::string& pack)
        {
            const auto lower = ToLower(pack);
            if (lower == "all")
            {
                AddImportPack(imports, "csharp");
                AddImportPack(imports, "unity");
                AddImportPack(imports, "melonloader");
                AddImportPack(imports, "bepinex");
                AddImportPack(imports, "rosemod");
                return;
            }

            if (lower == "everything")
            {
                AddImportPack(imports, "all");
                AddImportPack(imports, "unity_everything");
                AddImportPack(imports, "harmony");
                AddImportPack(imports, "il2cpp");
                AddImportPack(imports, "standardassets");
                AddImportPack(imports, "json");
                return;
            }

            if (lower == "csharp" || lower == "dotnet" || lower == "system")
            {
                AddImport(imports, "System");
                AddImport(imports, "System.Collections");
                AddImport(imports, "System.Collections.Generic");
                AddImport(imports, "System.Diagnostics");
                AddImport(imports, "System.IO");
                AddImport(imports, "System.Linq");
                AddImport(imports, "System.Reflection");
                AddImport(imports, "System.Runtime.CompilerServices");
                AddImport(imports, "System.Threading");
                AddImport(imports, "System.Threading.Tasks");
                AddImport(imports, "System.Text");
                return;
            }

            if (lower == "unity")
            {
                AddImport(imports, "UnityEngine", "UNITY_REFERENCES");
                AddImport(imports, "UnityEngine.AI", "UNITY_REFERENCES");
                AddImport(imports, "UnityEngine.Events", "UNITY_REFERENCES");
                AddImport(imports, "UnityEngine.SceneManagement", "UNITY_REFERENCES");
                AddImport(imports, "UnityEngine.UI", "UNITY_REFERENCES");
                return;
            }

            if (lower == "unity.core" || lower == "unityengine")
            {
                AddImport(imports, "UnityEngine", "UNITY_REFERENCES");
                return;
            }

            if (lower == "unity.ai" || lower == "unity.nav" || lower == "unity.navmesh")
            {
                AddImport(imports, "UnityEngine.AI", "UNITY_REFERENCES");
                return;
            }

            if (lower == "unity.events")
            {
                AddImport(imports, "UnityEngine.Events", "UNITY_REFERENCES");
                return;
            }

            if (lower == "unity.scene" || lower == "unity.scenes" || lower == "unity.scenemanagement")
            {
                AddImport(imports, "UnityEngine.SceneManagement", "UNITY_REFERENCES");
                return;
            }

            if (lower == "unity.ui")
            {
                AddImport(imports, "UnityEngine.UI", "UNITY_REFERENCES");
                AddImport(imports, "UnityEngine.EventSystems", "UNITY_UI");
                return;
            }

            if (lower == "unity.imgui" || lower == "unity.gui")
            {
                AddImport(imports, "UnityEngine", "UNITY_REFERENCES");
                return;
            }

            if (lower == "unity.input" || lower == "unity.legacyinput")
            {
                AddImport(imports, "UnityEngine", "UNITY_REFERENCES");
                return;
            }

            if (lower == "unity.inputsystem")
            {
                AddImport(imports, "UnityEngine.InputSystem", "UNITY_INPUT_SYSTEM");
                AddImport(imports, "UnityEngine.InputSystem.Controls", "UNITY_INPUT_SYSTEM");
                AddImport(imports, "UnityEngine.InputSystem.Layouts", "UNITY_INPUT_SYSTEM");
                AddImport(imports, "UnityEngine.InputSystem.LowLevel", "UNITY_INPUT_SYSTEM");
                AddImport(imports, "UnityEngine.InputSystem.UI", "UNITY_INPUT_SYSTEM");
                return;
            }

            if (lower == "unity.rendering")
            {
                AddImport(imports, "UnityEngine.Rendering", "UNITY_REFERENCES");
                return;
            }

            if (lower == "unity.urp")
            {
                AddImport(imports, "UnityEngine.Rendering.Universal", "UNITY_URP");
                return;
            }

            if (lower == "unity.hdrp")
            {
                AddImport(imports, "UnityEngine.Rendering.HighDefinition", "UNITY_HDRP");
                return;
            }

            if (lower == "unity.textmeshpro" || lower == "unity.tmp" || lower == "tmp")
            {
                AddImport(imports, "TMPro", "UNITY_TEXTMESHPRO");
                return;
            }

            if (lower == "unity.textcore")
            {
                AddImport(imports, "UnityEngine.TextCore", "UNITY_TEXTCORE");
                AddImport(imports, "UnityEngine.TextCore.LowLevel", "UNITY_TEXTCORE");
                return;
            }

            if (lower == "unity.xr")
            {
                AddImport(imports, "UnityEngine.XR", "UNITY_XR");
                return;
            }

            if (lower == "unity.xr.management")
            {
                AddImport(imports, "UnityEngine.XR.Management", "UNITY_XR_MANAGEMENT");
                return;
            }

            if (lower == "unity.timeline")
            {
                AddImport(imports, "UnityEngine.Timeline", "UNITY_TIMELINE");
                return;
            }

            if (lower == "unity.vfx")
            {
                AddImport(imports, "UnityEngine.VFX", "UNITY_VFX");
                return;
            }

            if (lower == "unity.video")
            {
                AddImport(imports, "UnityEngine.Video", "UNITY_VIDEO");
                return;
            }

            if (lower == "unity.tilemaps")
            {
                AddImport(imports, "UnityEngine.Tilemaps", "UNITY_TILEMAPS");
                return;
            }

            if (lower == "unity.webrequest" || lower == "unity.networking")
            {
                AddImport(imports, "UnityEngine.Networking", "UNITY_WEBREQUEST");
                return;
            }

            if (lower == "unity.jobs")
            {
                AddImport(imports, "UnityEngine.Jobs", "UNITY_JOBS");
                return;
            }

            if (lower == "unity.lowlevel")
            {
                AddImport(imports, "UnityEngine.LowLevel", "UNITY_REFERENCES");
                return;
            }

            if (lower == "unity.playables")
            {
                AddImport(imports, "UnityEngine.Playables", "UNITY_REFERENCES");
                return;
            }

            if (lower == "unity.pool")
            {
                AddImport(imports, "UnityEngine.Pool", "UNITY_REFERENCES");
                return;
            }

            if (lower == "unity.profiling")
            {
                AddImport(imports, "UnityEngine.Profiling", "UNITY_REFERENCES");
                return;
            }

            if (lower == "unity.serialization")
            {
                AddImport(imports, "UnityEngine.Serialization", "UNITY_REFERENCES");
                return;
            }

            if (lower == "unity.scripting")
            {
                AddImport(imports, "UnityEngine.Scripting", "UNITY_REFERENCES");
                return;
            }

            if (lower == "unity.addressables")
            {
                AddImport(imports, "UnityEngine.AddressableAssets", "UNITY_ADDRESSABLES");
                AddImport(imports, "UnityEngine.ResourceManagement", "UNITY_ADDRESSABLES");
                return;
            }

            if (lower == "unity.localization")
            {
                AddImport(imports, "UnityEngine.Localization", "UNITY_LOCALIZATION");
                return;
            }

            if (lower == "unity.ai.navigation")
            {
                AddImport(imports, "Unity.AI.Navigation", "UNITY_AI_NAVIGATION");
                return;
            }

            if (lower == "unity all" || lower == "unity_all" || lower == "unity.everything" || lower == "unity_everything")
            {
                AddImportPack(imports, "unity");
                AddImport(imports, "UnityEngine.Accessibility", "UNITY_ACCESSIBILITY");
                AddImport(imports, "UnityEngine.AddressableAssets", "UNITY_ADDRESSABLES");
                AddImport(imports, "UnityEngine.Analytics", "UNITY_ANALYTICS");
                AddImport(imports, "UnityEngine.Android", "UNITY_ANDROID");
                AddImport(imports, "UnityEngine.Animations", "UNITY_REFERENCES");
                AddImport(imports, "UnityEngine.Animations.Rigging", "UNITY_ANIMATION_RIGGING");
                AddImport(imports, "UnityEngine.Assertions", "UNITY_REFERENCES");
                AddImport(imports, "UnityEngine.Audio", "UNITY_REFERENCES");
                AddImport(imports, "UnityEngine.Build", "UNITY_BUILD");
                AddImport(imports, "UnityEngine.CrashReportHandler", "UNITY_REFERENCES");
                AddImport(imports, "UnityEngine.Device", "UNITY_REFERENCES");
                AddImport(imports, "UnityEngine.Diagnostics", "UNITY_REFERENCES");
                AddImport(imports, "UnityEngine.EventSystems", "UNITY_UI");
                AddImport(imports, "UnityEngine.Experimental.AI", "UNITY_EXPERIMENTAL");
                AddImport(imports, "UnityEngine.Experimental.Animations", "UNITY_EXPERIMENTAL");
                AddImport(imports, "UnityEngine.Experimental.AssetBundlePatching", "UNITY_EXPERIMENTAL");
                AddImport(imports, "UnityEngine.Experimental.Audio", "UNITY_EXPERIMENTAL");
                AddImport(imports, "UnityEngine.Experimental.GlobalIllumination", "UNITY_EXPERIMENTAL");
                AddImport(imports, "UnityEngine.Experimental.LowLevel", "UNITY_EXPERIMENTAL");
                AddImport(imports, "UnityEngine.Experimental.Playables", "UNITY_EXPERIMENTAL");
                AddImport(imports, "UnityEngine.Experimental.Rendering", "UNITY_EXPERIMENTAL");
                AddImport(imports, "UnityEngine.Experimental.SceneManagement", "UNITY_EXPERIMENTAL");
                AddImport(imports, "UnityEngine.Experimental.U2D", "UNITY_EXPERIMENTAL");
                AddImport(imports, "UnityEngine.Experimental.UIElements", "UNITY_UIELEMENTS");
                AddImport(imports, "UnityEngine.GameCenter", "UNITY_GAMECENTER");
                AddImport(imports, "UnityEngine.InputSystem", "UNITY_INPUT_SYSTEM");
                AddImport(imports, "UnityEngine.InputSystem.Controls", "UNITY_INPUT_SYSTEM");
                AddImport(imports, "UnityEngine.InputSystem.Layouts", "UNITY_INPUT_SYSTEM");
                AddImport(imports, "UnityEngine.InputSystem.LowLevel", "UNITY_INPUT_SYSTEM");
                AddImport(imports, "UnityEngine.InputSystem.UI", "UNITY_INPUT_SYSTEM");
                AddImport(imports, "UnityEngine.Jobs", "UNITY_JOBS");
                AddImport(imports, "UnityEngine.Localization", "UNITY_LOCALIZATION");
                AddImport(imports, "UnityEngine.LowLevel", "UNITY_REFERENCES");
                AddImport(imports, "UnityEngine.Networking", "UNITY_WEBREQUEST");
                AddImport(imports, "UnityEngine.ParticleSystemJobs", "UNITY_PARTICLE_JOBS");
                AddImport(imports, "UnityEngine.Playables", "UNITY_REFERENCES");
                AddImport(imports, "UnityEngine.Pool", "UNITY_REFERENCES");
                AddImport(imports, "UnityEngine.Profiling", "UNITY_REFERENCES");
                AddImport(imports, "UnityEngine.Rendering", "UNITY_REFERENCES");
                AddImport(imports, "UnityEngine.Rendering.Universal", "UNITY_URP");
                AddImport(imports, "UnityEngine.Rendering.HighDefinition", "UNITY_HDRP");
                AddImport(imports, "UnityEngine.ResourceManagement", "UNITY_ADDRESSABLES");
                AddImport(imports, "UnityEngine.SceneManagement", "UNITY_REFERENCES");
                AddImport(imports, "UnityEngine.Scripting", "UNITY_REFERENCES");
                AddImport(imports, "UnityEngine.Search", "UNITY_SEARCH");
                AddImport(imports, "UnityEngine.Serialization", "UNITY_REFERENCES");
                AddImport(imports, "UnityEngine.SocialPlatforms", "UNITY_REFERENCES");
                AddImport(imports, "UnityEngine.Sprites", "UNITY_REFERENCES");
                AddImport(imports, "UnityEngine.SubsystemsImplementation", "UNITY_REFERENCES");
                AddImport(imports, "UnityEngine.TerrainTools", "UNITY_TERRAIN_TOOLS");
                AddImport(imports, "UnityEngine.TestTools", "UNITY_TEST_TOOLS");
                AddImport(imports, "UnityEngine.TextCore", "UNITY_TEXTCORE");
                AddImport(imports, "UnityEngine.TextCore.LowLevel", "UNITY_TEXTCORE");
                AddImport(imports, "UnityEngine.Tilemaps", "UNITY_TILEMAPS");
                AddImport(imports, "UnityEngine.Timeline", "UNITY_TIMELINE");
                AddImport(imports, "UnityEngine.UIElements", "UNITY_UIELEMENTS");
                AddImport(imports, "UnityEngine.VFX", "UNITY_VFX");
                AddImport(imports, "UnityEngine.Video", "UNITY_VIDEO");
                AddImport(imports, "UnityEngine.Windows", "UNITY_WINDOWS");
                AddImport(imports, "UnityEngine.XR", "UNITY_XR");
                AddImport(imports, "UnityEngine.XR.Management", "UNITY_XR_MANAGEMENT");
                AddImport(imports, "Unity.AI.Navigation", "UNITY_AI_NAVIGATION");
                AddImport(imports, "TMPro", "UNITY_TEXTMESHPRO");
                return;
            }

            if (lower == "melonloader" || lower == "melon")
            {
                AddImport(imports, "MelonLoader", "MELONLOADER");
                AddImport(imports, "MelonLoader.Utils", "MELONLOADER");
                return;
            }

            if (lower == "melonloader.core" || lower == "melon.core")
            {
                AddImport(imports, "MelonLoader", "MELONLOADER");
                return;
            }

            if (lower == "melonloader.utils" || lower == "melon.utils")
            {
                AddImport(imports, "MelonLoader.Utils", "MELONLOADER");
                return;
            }

            if (lower == "harmony" || lower == "harmonylib")
            {
                AddImport(imports, "HarmonyLib", "HARMONY");
                return;
            }

            if (lower == "il2cpp" || lower == "interop")
            {
                AddImport(imports, "Il2Cpp", "IL2CPP_REFERENCES");
                AddImport(imports, "Il2CppInterop.Runtime", "IL2CPP_REFERENCES");
                AddImport(imports, "Il2CppInterop.Runtime.Attributes", "IL2CPP_REFERENCES");
                AddImport(imports, "Il2CppInterop.Runtime.Injection", "IL2CPP_REFERENCES");
                AddImport(imports, "Il2CppInterop.Runtime.InteropTypes.Arrays", "IL2CPP_REFERENCES");
                return;
            }

            if (lower == "il2cpp.game")
            {
                AddImport(imports, "Il2Cpp", "IL2CPP_REFERENCES");
                return;
            }

            if (lower == "il2cpp.runtime" || lower == "interop.runtime")
            {
                AddImport(imports, "Il2CppInterop.Runtime", "IL2CPP_REFERENCES");
                return;
            }

            if (lower == "il2cpp.attributes" || lower == "interop.attributes")
            {
                AddImport(imports, "Il2CppInterop.Runtime.Attributes", "IL2CPP_REFERENCES");
                return;
            }

            if (lower == "il2cpp.injection" || lower == "interop.injection")
            {
                AddImport(imports, "Il2CppInterop.Runtime.Injection", "IL2CPP_REFERENCES");
                return;
            }

            if (lower == "il2cpp.arrays" || lower == "interop.arrays")
            {
                AddImport(imports, "Il2CppInterop.Runtime.InteropTypes.Arrays", "IL2CPP_REFERENCES");
                return;
            }

            if (lower == "unitystandardassets" || lower == "standardassets")
            {
                AddImport(imports, "UnityStandardAssets.Characters.FirstPerson", "UNITY_STANDARD_ASSETS");
                return;
            }

            if (lower == "json" || lower == "newtonsoft" || lower == "newtonsoft.json")
            {
                AddImport(imports, "Newtonsoft.Json", "NEWTONSOFT_JSON");
                AddImport(imports, "Newtonsoft.Json.Linq", "NEWTONSOFT_JSON");
                return;
            }

            if (lower == "json.core" || lower == "newtonsoft.core")
            {
                AddImport(imports, "Newtonsoft.Json", "NEWTONSOFT_JSON");
                return;
            }

            if (lower == "json.linq" || lower == "newtonsoft.linq")
            {
                AddImport(imports, "Newtonsoft.Json.Linq", "NEWTONSOFT_JSON");
                return;
            }

            if (lower == "bepinex")
            {
                AddImport(imports, "BepInEx", "BEPINEX_MONO || BEPINEX_IL2CPP");
                AddImport(imports, "BepInEx.Configuration", "BEPINEX_MONO || BEPINEX_IL2CPP");
                AddImport(imports, "BepInEx.Logging", "BEPINEX_MONO || BEPINEX_IL2CPP");
                AddImport(imports, "BepInEx.Unity.Mono", "BEPINEX_MONO");
                AddImport(imports, "BepInEx.Unity.IL2CPP", "BEPINEX_IL2CPP");
                return;
            }

            if (lower == "bepinex.core")
            {
                AddImport(imports, "BepInEx", "BEPINEX_MONO || BEPINEX_IL2CPP");
                return;
            }

            if (lower == "bepinex.config")
            {
                AddImport(imports, "BepInEx.Configuration", "BEPINEX_MONO || BEPINEX_IL2CPP");
                return;
            }

            if (lower == "bepinex.logging")
            {
                AddImport(imports, "BepInEx.Logging", "BEPINEX_MONO || BEPINEX_IL2CPP");
                return;
            }

            if (lower == "bepinex.mono")
            {
                AddImport(imports, "BepInEx.Unity.Mono", "BEPINEX_MONO");
                return;
            }

            if (lower == "bepinex.il2cpp")
            {
                AddImport(imports, "BepInEx.Unity.IL2CPP", "BEPINEX_IL2CPP");
                return;
            }

            if (lower == "rosemod" || lower == "devkit")
            {
                AddImport(imports, "RoseMod.DevKit");
                return;
            }

            AddImport(imports, pack);
        }

        std::string InferCSharpValue(const std::string& value)
        {
            auto trimmed = Trim(value);
            if (trimmed.empty())
                return "string.Empty";

            if (trimmed.front() == '"')
                return EscapeCSharpString(ReadQuoted(trimmed, 0, 0));

            const auto lower = ToLower(trimmed);
            if (lower == "true" || lower == "false")
                return lower;

            bool numeric = true;
            bool hasDot = false;
            size_t start = trimmed.front() == '-' ? 1 : 0;
            for (size_t i = start; i < trimmed.size(); ++i)
            {
                if (trimmed[i] == '.')
                {
                    hasDot = true;
                    continue;
                }

                if (!std::isdigit(static_cast<unsigned char>(trimmed[i])))
                {
                    numeric = false;
                    break;
                }
            }

            if (numeric && start < trimmed.size())
                return hasDot ? trimmed + "f" : trimmed;

            return trimmed;
        }

        std::string RoseExpressionToCSharp(const std::string& expression)
        {
            auto trimmed = Trim(expression);
            if (trimmed.empty())
                return "true";

            const auto words = SplitWords(trimmed);
            if (words.size() == 3)
            {
                const auto op = ToLower(words[1]);
                if (op == "is")
                    return words[0] + " == " + InferCSharpValue(words[2]);
                if (op == "isnt")
                    return words[0] + " != " + InferCSharpValue(words[2]);
                if (op == "more")
                    return words[0] + " > " + InferCSharpValue(words[2]);
                if (op == "less")
                    return words[0] + " < " + InferCSharpValue(words[2]);
                if (op == "atleast")
                    return words[0] + " >= " + InferCSharpValue(words[2]);
                if (op == "atmost")
                    return words[0] + " <= " + InferCSharpValue(words[2]);
            }

            return trimmed;
        }

        std::string SettingFieldName(const Setting& setting)
        {
            return "setting_" + SanitizeIdentifier(setting.name, "Value");
        }

        std::vector<SourceLine> LoadSource(const std::string& path)
        {
            std::ifstream input(path);
            if (!input)
                throw std::runtime_error("Could not open RoseV source file: " + path);

            std::vector<SourceLine> lines;
            std::string line;
            int lineNumber = 0;
            while (std::getline(input, line))
            {
                ++lineNumber;
                auto cleaned = Trim(StripComment(line));
                if (!cleaned.empty())
                    lines.push_back({ lineNumber, cleaned });
            }

            return lines;
        }

        Statement ParseStatement(const std::vector<SourceLine>& lines, size_t& index);

        std::string ParseRawBlock(const std::vector<SourceLine>& lines, size_t& index, const std::string& blockName)
        {
            if (index >= lines.size())
                throw std::runtime_error("Unexpected end of RoseV file; expected " + blockName + " block.");
            if (!EndsWithBrace(lines[index].text))
                throw std::runtime_error("Line " + std::to_string(lines[index].number) + ": " + blockName + " blocks must end with {.");

            int depth = BraceDelta(lines[index].text);
            ++index;
            std::ostringstream raw;
            while (index < lines.size())
            {
                const auto& rawLine = lines[index];
                const auto delta = BraceDelta(rawLine.text);
                if (depth + delta <= 0)
                {
                    ++index;
                    return raw.str();
                }

                raw << rawLine.text << "\n";
                depth += delta;
                ++index;
            }

            throw std::runtime_error("Unexpected end of RoseV file; missing } for " + blockName + " block.");
        }

        std::string ParseRawUntilRoseV(const std::vector<SourceLine>& lines, size_t& index, const std::string& blockName, bool stopAtParentBrace)
        {
            int rawDepth = 0;
            std::ostringstream raw;
            while (index < lines.size())
            {
                const auto& rawLine = lines[index];
                std::string synvertMode;
                if (rawDepth == 0 && TryParseSynvert(rawLine.text, synvertMode) && IsRoseVSynvertMode(synvertMode))
                {
                    ++index;
                    return raw.str();
                }

                if (stopAtParentBrace && rawDepth == 0 && rawLine.text == "}")
                    return raw.str();

                raw << rawLine.text << "\n";
                rawDepth += BraceDelta(rawLine.text);
                if (rawDepth < 0)
                    rawDepth = 0;
                ++index;
            }

            if (stopAtParentBrace)
                throw std::runtime_error("Unexpected end of RoseV file while reading " + blockName + "; expected synvert = rosev or }.");

            return raw.str();
        }

        std::vector<Statement> ParseStatementBlock(const std::vector<SourceLine>& lines, size_t& index)
        {
            std::vector<Statement> statements;
            while (index < lines.size())
            {
                std::string synvertMode;
                if (TryParseSynvert(lines[index].text, synvertMode) && IsRoseVSynvertMode(synvertMode))
                {
                    ++index;
                    continue;
                }

                if (lines[index].text == "}")
                {
                    ++index;
                    return statements;
                }

                statements.push_back(ParseStatement(lines, index));
            }

            throw std::runtime_error("Unexpected end of RoseV file; missing }.");
        }

        Statement ParseStatement(const std::vector<SourceLine>& lines, size_t& index)
        {
            const auto& line = lines[index];
            const auto text = line.text;
            std::string synvertMode;

            if (TryParseSynvert(text, synvertMode))
            {
                if (IsRoseVSynvertMode(synvertMode))
                {
                    ++index;
                    return { StatementKind::CSharp, line.number, 0, -1, "", "", {} };
                }

                if (IsCSharpSynvertMode(synvertMode) || IsLoaderSynvertMode(synvertMode))
                {
                    const auto raw = EndsWithBrace(text)
                        ? ParseRawBlock(lines, index, "synvert = " + synvertMode)
                        : (++index, ParseRawUntilRoseV(lines, index, "synvert = " + synvertMode, true));
                    return { StatementKind::CSharp, line.number, 0, -1, raw, "", {} };
                }

                throw std::runtime_error("Line " + std::to_string(line.number) + ": unknown synvert language: " + synvertMode + ". Use rosev, csharp, cs, unity, melonloader, bepinex, rosemod, il2cpp, or harmony.");
            }

            if (StartsWith(text, "say "))
            {
                ++index;
                return { StatementKind::Say, line.number, 0, -1, ReadQuoted(text, 4, line.number), "", {} };
            }

            if (StartsWith(text, "warn "))
            {
                ++index;
                return { StatementKind::Warn, line.number, 0, -1, ReadQuoted(text, 5, line.number), "", {} };
            }

            if (StartsWith(text, "error "))
            {
                ++index;
                return { StatementKind::Error, line.number, 0, -1, ReadQuoted(text, 6, line.number), "", {} };
            }

            if (StartsWith(text, "unity "))
            {
                ++index;
                return { StatementKind::Unity, line.number, 0, -1, ReadQuoted(text, 6, line.number), "", {} };
            }

            if (IsRawBlockStart(text, "csharp") || IsRawBlockStart(text, "cs"))
            {
                const auto raw = ParseRawBlock(lines, index, "csharp");
                return { StatementKind::CSharp, line.number, 0, -1, raw, "", {} };
            }

            if (StartsWith(text, "cs "))
            {
                ++index;
                return { StatementKind::CSharp, line.number, 0, -1, ReadQuoted(text, 3, line.number), "", {} };
            }

            if (StartsWith(text, "csharp "))
            {
                ++index;
                return { StatementKind::CSharp, line.number, 0, -1, ReadQuoted(text, 7, line.number), "", {} };
            }

            if (StartsWith(text, "let "))
            {
                const auto without = Trim(text.substr(4));
                const auto words = SplitWords(without);
                if (words.size() < 3 || words[1] != "=")
                    throw std::runtime_error("Line " + std::to_string(line.number) + ": expected let <name> = <value>.");

                const auto equals = without.find('=');
                ++index;
                return { StatementKind::Let, line.number, 0, -1, SanitizeIdentifier(words[0], "value"), Trim(without.substr(equals + 1)), {} };
            }

            if (StartsWith(text, "set "))
            {
                const auto without = Trim(text.substr(4));
                const auto words = SplitWords(without);
                if (words.size() < 3 || words[1] != "=")
                    throw std::runtime_error("Line " + std::to_string(line.number) + ": expected set <name> = <value>.");

                const auto equals = without.find('=');
                ++index;
                return { StatementKind::Set, line.number, 0, -1, SanitizeIdentifier(words[0], "value"), Trim(without.substr(equals + 1)), {} };
            }

            if (StartsWith(text, "add "))
            {
                const auto words = SplitWords(text);
                if (words.size() != 3)
                    throw std::runtime_error("Line " + std::to_string(line.number) + ": expected add <name> <amount>.");

                ++index;
                return { StatementKind::Add, line.number, 0, -1, SanitizeIdentifier(words[1], "value"), words[2], {} };
            }

            if (StartsWith(text, "sub "))
            {
                const auto words = SplitWords(text);
                if (words.size() != 3)
                    throw std::runtime_error("Line " + std::to_string(line.number) + ": expected sub <name> <amount>.");

                ++index;
                return { StatementKind::Subtract, line.number, 0, -1, SanitizeIdentifier(words[1], "value"), words[2], {} };
            }

            if (StartsWith(text, "mul "))
            {
                const auto words = SplitWords(text);
                if (words.size() != 3)
                    throw std::runtime_error("Line " + std::to_string(line.number) + ": expected mul <name> <amount>.");

                ++index;
                return { StatementKind::Multiply, line.number, 0, -1, SanitizeIdentifier(words[1], "value"), words[2], {} };
            }

            if (StartsWith(text, "div "))
            {
                const auto words = SplitWords(text);
                if (words.size() != 3)
                    throw std::runtime_error("Line " + std::to_string(line.number) + ": expected div <name> <amount>.");

                ++index;
                return { StatementKind::Divide, line.number, 0, -1, SanitizeIdentifier(words[1], "value"), words[2], {} };
            }

            if (StartsWith(text, "call "))
            {
                const auto name = SanitizeIdentifier(Trim(text.substr(5)), "function");
                ++index;
                return { StatementKind::Call, line.number, 0, -1, name, "", {} };
            }

            if (StartsWith(text, "if "))
            {
                if (!EndsWithBrace(text))
                    throw std::runtime_error("Line " + std::to_string(line.number) + ": if blocks must end with {.");

                Statement statement;
                statement.kind = StatementKind::If;
                statement.line = line.number;
                statement.value = Trim(RemoveTrailingBrace(text).substr(3));
                ++index;
                statement.children = ParseStatementBlock(lines, index);
                return statement;
            }

            if (StartsWith(text, "repeat "))
            {
                if (!EndsWithBrace(text))
                    throw std::runtime_error("Line " + std::to_string(line.number) + ": repeat blocks must end with {.");

                const auto header = RemoveTrailingBrace(text);
                const auto words = SplitWords(header);
                if (words.size() != 2)
                    throw std::runtime_error("Line " + std::to_string(line.number) + ": expected repeat <count> {.");

                Statement statement;
                statement.kind = StatementKind::Repeat;
                statement.line = line.number;
                statement.value = words[1];
                ++index;
                statement.children = ParseStatementBlock(lines, index);
                return statement;
            }

            if (StartsWith(text, "while "))
            {
                if (!EndsWithBrace(text))
                    throw std::runtime_error("Line " + std::to_string(line.number) + ": while blocks must end with {.");

                Statement statement;
                statement.kind = StatementKind::While;
                statement.line = line.number;
                statement.value = Trim(RemoveTrailingBrace(text).substr(6));
                ++index;
                statement.children = ParseStatementBlock(lines, index);
                return statement;
            }

            if (StartsWith(text, "try "))
            {
                if (!EndsWithBrace(text))
                    throw std::runtime_error("Line " + std::to_string(line.number) + ": try blocks must end with {.");

                Statement statement;
                statement.kind = StatementKind::Try;
                statement.line = line.number;
                ++index;
                statement.children = ParseStatementBlock(lines, index);
                return statement;
            }

            if (StartsWith(text, "return"))
            {
                const auto value = Trim(text.substr(6));
                ++index;
                return { StatementKind::Return, line.number, 0, -1, value, "", {} };
            }

            if (StartsWith(text, "throw "))
            {
                ++index;
                return { StatementKind::Throw, line.number, 0, -1, ReadQuoted(text, 6, line.number), "", {} };
            }

            if (StartsWith(text, "native call "))
            {
                const auto target = Trim(text.substr(12));
                const auto dot = target.find('.');
                const auto alias = dot == std::string::npos
                    ? std::string()
                    : SanitizeIdentifier(target.substr(0, dot), "Native");
                const auto name = dot == std::string::npos
                    ? SanitizeIdentifier(target, "NativeCall")
                    : SanitizeIdentifier(target.substr(dot + 1), "NativeCall");
                ++index;
                return { StatementKind::NativeCall, line.number, 0, -1, alias, name, {} };
            }

            if (StartsWith(text, "emit "))
            {
                const auto topic = ReadQuoted(text, 5, line.number);
                const auto topicEnd = text.find('"', text.find('"', 5) + 1);
                const auto payloadStart = topicEnd == std::string::npos ? std::string::npos : text.find('"', topicEnd + 1);
                const auto payload = payloadStart == std::string::npos ? std::string() : ReadQuoted(text, payloadStart, line.number);
                ++index;
                return { StatementKind::Emit, line.number, 0, -1, topic, payload, {} };
            }

            if (StartsWith(text, "every "))
            {
                if (!EndsWithBrace(text))
                    throw std::runtime_error("Line " + std::to_string(line.number) + ": every blocks must end with {.");

                const auto header = RemoveTrailingBrace(text);
                const auto words = SplitWords(header);
                if (words.size() != 2)
                    throw std::runtime_error("Line " + std::to_string(line.number) + ": expected every <frames> {.");

                Statement statement;
                statement.kind = StatementKind::Every;
                statement.line = line.number;
                statement.numberValue = std::stoi(words[1]);
                ++index;
                statement.children = ParseStatementBlock(lines, index);
                return statement;
            }

            if (StartsWith(text, "key "))
            {
                if (!EndsWithBrace(text))
                    throw std::runtime_error("Line " + std::to_string(line.number) + ": key blocks must end with {.");

                const auto header = RemoveTrailingBrace(text);
                const auto words = SplitWords(header);
                if (words.size() != 2)
                    throw std::runtime_error("Line " + std::to_string(line.number) + ": expected key <UnityKeyCode> {.");

                Statement statement;
                statement.kind = StatementKind::Key;
                statement.line = line.number;
                statement.value = words[1];
                ++index;
                statement.children = ParseStatementBlock(lines, index);
                return statement;
            }

            throw std::runtime_error("Line " + std::to_string(line.number) + ": unknown RoseV statement: " + text);
        }

        void ParseHeader(Program& program, const SourceLine& line)
        {
            const auto text = line.text;
            if (StartsWith(text, "import "))
            {
                auto importName = Trim(text.substr(7));
                if (importName.empty())
                    throw std::runtime_error("Line " + std::to_string(line.number) + ": expected import <Namespace>.");

                if (!TryAddExplicitImport(program.imports, importName))
                    AddImportPack(program.imports, importName);
                return;
            }

            if (StartsWith(text, "rosev "))
            {
                program.name = ReadQuoted(text, 6, line.number);
                program.id = ReadNamedQuoted(text, "id", program.id, line.number);
                program.version = ReadNamedQuoted(text, "version", program.version, line.number);
                program.author = ReadNamedQuoted(text, "author", program.author, line.number);
                program.description = ReadNamedQuoted(text, "description", program.description, line.number);
                program.className = SanitizeIdentifier(program.name, "RoseVGeneratedMod");
                return;
            }

            if (StartsWith(text, "namespace "))
            {
                program.nameSpace = SanitizeNamespace(Trim(text.substr(10)));
                return;
            }

            if (StartsWith(text, "class "))
            {
                program.className = SanitizeIdentifier(Trim(text.substr(6)), "RoseVGeneratedMod");
                return;
            }

            if (StartsWith(text, "use "))
            {
                const auto target = ToLower(Trim(text.substr(4)));
                if (target == "unity")
                    program.useUnity = true;
                else if (target == "melonloader")
                    program.useMelonLoader = true;
                else if (target == "bepinex")
                    program.useBepInEx = true;
                else if (target == "rosemod")
                    program.useRoseMod = true;
                else
                    throw std::runtime_error("Line " + std::to_string(line.number) + ": unknown use target: " + target);
                return;
            }

            if (StartsWith(text, "setting "))
            {
                const auto firstQuote = text.find('"');
                std::string prefix = firstQuote == std::string::npos ? text : text.substr(0, firstQuote);
                const auto words = SplitWords(prefix);
                if (words.size() < 4)
                    throw std::runtime_error("Line " + std::to_string(line.number) + ": expected setting <name> <type> <default> \"description\".");

                Setting setting;
                setting.name = SanitizeIdentifier(words[1], "Setting");
                setting.type = words[2];
                setting.defaultValue = words[3];
                setting.description = firstQuote == std::string::npos ? "" : ReadQuoted(text, firstQuote, line.number);
                program.settings.push_back(setting);
                return;
            }

            if (StartsWith(text, "field "))
            {
                const auto words = SplitWords(text);
                if (words.size() < 5 || words[3] != "=")
                    throw std::runtime_error("Line " + std::to_string(line.number) + ": expected field <name> <type> = <value>.");

                Field field;
                field.name = SanitizeIdentifier(words[1], "field");
                field.type = words[2];
                const auto equals = text.find('=');
                field.defaultValue = Trim(text.substr(equals + 1));
                program.fields.push_back(field);
                return;
            }

            if (StartsWith(text, "native "))
            {
                const auto firstQuote = text.find('"');
                const auto prefix = firstQuote == std::string::npos ? text : text.substr(0, firstQuote);
                const auto words = SplitWords(prefix);
                if (words.size() < 2 || (firstQuote == std::string::npos && words.size() < 3))
                    throw std::runtime_error("Line " + std::to_string(line.number) + ": expected native <c|cpp|asm> \"path\" [as Alias].");

                NativeUnit unit;
                unit.language = ToLower(words[1]);
                unit.path = firstQuote == std::string::npos ? words[2] : ReadQuoted(text, firstQuote, line.number);
                if (unit.language != "c" && unit.language != "cpp" && unit.language != "asm")
                    throw std::runtime_error("Line " + std::to_string(line.number) + ": native language must be c, cpp, or asm.");

                std::string alias;
                if (firstQuote == std::string::npos)
                {
                    alias = words.size() >= 5 && words[3] == "as" ? words[4] : "";
                }
                else
                {
                    const auto closingQuote = text.find('"', firstQuote + 1);
                    const auto suffix = closingQuote == std::string::npos ? std::string() : Trim(text.substr(closingQuote + 1));
                    const auto suffixWords = SplitWords(suffix);
                    alias = suffixWords.size() >= 2 && suffixWords[0] == "as" ? suffixWords[1] : "";
                }

                unit.alias = !alias.empty()
                    ? SanitizeIdentifier(alias, "Native")
                    : SanitizeIdentifier(BaseName(unit.path), "Native");
                program.nativeUnits.push_back(unit);
                return;
            }

            throw std::runtime_error("Line " + std::to_string(line.number) + ": unknown RoseV header: " + text);
        }

        EventBlock ParseEvent(const std::vector<SourceLine>& lines, size_t& index)
        {
            const auto& line = lines[index];
            if (!EndsWithBrace(line.text))
                throw std::runtime_error("Line " + std::to_string(line.number) + ": when blocks must end with {.");

            const auto header = RemoveTrailingBrace(line.text);
            const auto words = SplitWords(header);
            if (words.size() < 2 || words[0] != "when")
                throw std::runtime_error("Line " + std::to_string(line.number) + ": expected when <event> {.");

            EventBlock block;
            block.line = line.number;
            block.name = ToLower(words[1]);
            if (words.size() >= 3)
                block.parameter = words[2];

            ++index;
            block.statements = ParseStatementBlock(lines, index);
            return block;
        }

        FunctionBlock ParseFunction(const std::vector<SourceLine>& lines, size_t& index)
        {
            const auto& line = lines[index];
            if (!EndsWithBrace(line.text))
                throw std::runtime_error("Line " + std::to_string(line.number) + ": function blocks must end with {.");

            const auto header = RemoveTrailingBrace(line.text);
            const auto words = SplitWords(header);
            if (words.size() != 2 || words[0] != "make")
                throw std::runtime_error("Line " + std::to_string(line.number) + ": expected make <functionName> {.");

            FunctionBlock block;
            block.line = line.number;
            block.name = SanitizeIdentifier(words[1], "function");
            ++index;
            block.statements = ParseStatementBlock(lines, index);
            return block;
        }

        Program ParseProgram(const std::vector<SourceLine>& lines, const CompileOptions& options)
        {
            Program program;
            for (const auto& line : lines)
                program.sourceHash = rosev_text_hash((std::to_string(program.sourceHash) + line.text).c_str());

            size_t index = 0;
            while (index < lines.size())
            {
                const auto& text = lines[index].text;
                std::string synvertMode;
                if (TryParseSynvert(text, synvertMode))
                {
                    if (IsRoseVSynvertMode(synvertMode))
                    {
                        ++index;
                        continue;
                    }

                    if (IsCSharpSynvertMode(synvertMode) || IsLoaderSynvertMode(synvertMode))
                    {
                        program.memberBlocks.push_back(EndsWithBrace(text)
                            ? ParseRawBlock(lines, index, "synvert = " + synvertMode)
                            : (++index, ParseRawUntilRoseV(lines, index, "synvert = " + synvertMode, false)));
                        continue;
                    }

                    throw std::runtime_error("Line " + std::to_string(lines[index].number) + ": unknown synvert language: " + synvertMode + ". Use rosev, csharp, cs, unity, melonloader, bepinex, rosemod, il2cpp, or harmony.");
                }

                if (IsRawBlockStart(text, "members") ||
                    IsRawBlockStart(text, "member") ||
                    IsRawBlockStart(text, "csharp members") ||
                    IsRawBlockStart(text, "cs members") ||
                    IsRawBlockStart(text, "csharp") ||
                    IsRawBlockStart(text, "cs"))
                {
                    program.memberBlocks.push_back(ParseRawBlock(lines, index, "members"));
                    continue;
                }

                if (StartsWith(lines[index].text, "make "))
                {
                    program.functions.push_back(ParseFunction(lines, index));
                    continue;
                }

                if (StartsWith(lines[index].text, "when "))
                {
                    program.events.push_back(ParseEvent(lines, index));
                    continue;
                }

                ParseHeader(program, lines[index]);
                ++index;
            }

            if (!options.forcedNamespace.empty())
                program.nameSpace = SanitizeNamespace(options.forcedNamespace);
            if (!options.forcedClassName.empty())
                program.className = SanitizeIdentifier(options.forcedClassName, "RoseVGeneratedMod");

            return program;
        }

        void AssignCounters(std::vector<Statement>& statements, int& counter)
        {
            for (auto& statement : statements)
            {
                if (statement.kind == StatementKind::Every)
                    statement.counterIndex = counter++;
                AssignCounters(statement.children, counter);
            }
        }

        int CountCounters(const std::vector<Statement>& statements)
        {
            int count = 0;
            for (const auto& statement : statements)
            {
                if (statement.kind == StatementKind::Every)
                    ++count;
                count += CountCounters(statement.children);
            }
            return count;
        }

        void CollectNativeCalls(const std::vector<Statement>& statements, std::vector<NativeCall>& calls)
        {
            for (const auto& statement : statements)
            {
                if (statement.kind == StatementKind::NativeCall)
                    calls.push_back({ statement.value, statement.value2 });
                CollectNativeCalls(statement.children, calls);
            }
        }

        std::vector<NativeCall> CollectNativeCalls(const Program& program)
        {
            std::vector<NativeCall> calls;
            for (const auto& block : program.events)
                CollectNativeCalls(block.statements, calls);
            for (const auto& block : program.functions)
                CollectNativeCalls(block.statements, calls);

            std::vector<NativeCall> unique;
            std::unordered_set<std::string> seen;
            for (auto call : calls)
            {
                if (call.functionName.empty())
                    continue;

                const auto key = call.alias + "|" + call.functionName;
                if (seen.insert(key).second)
                    unique.push_back(call);
            }

            return unique;
        }

        std::string NativeLibraryForAlias(const Program& program, const std::string& alias)
        {
            for (const auto& nativeUnit : program.nativeUnits)
            {
                if (nativeUnit.alias == alias)
                    return nativeUnit.alias;
            }

            return alias.empty() && !program.nativeUnits.empty()
                ? program.nativeUnits.front().alias
                : alias;
        }

        std::string MethodNameForEvent(const std::string& eventName)
        {
            if (eventName == "load")
                return "OnLoad";
            if (eventName == "start")
                return "OnStart";
            if (eventName == "update")
                return "OnUpdate";
            if (eventName == "fixed_update")
                return "OnFixedUpdate";
            if (eventName == "late_update")
                return "OnLateUpdate";
            if (eventName == "scene_loaded")
                return "OnSceneLoaded";
            if (eventName == "scene_unloaded")
                return "OnSceneUnloaded";
            if (eventName == "gui")
                return "OnGui";
            if (eventName == "quit")
                return "OnApplicationQuit";
            if (eventName == "unload")
                return "OnUnload";

            throw std::runtime_error("Unknown RoseV event: " + eventName);
        }

        std::string MethodSignatureForEvent(const std::string& eventName)
        {
            const auto method = MethodNameForEvent(eventName);
            if (eventName == "scene_loaded" || eventName == "scene_unloaded")
                return "public override void " + method + "(int buildIndex, string sceneName)";

            return "public override void " + method + "()";
        }

        std::string TextArgsForEvent(const std::string& eventName)
        {
            if (eventName == "scene_loaded" || eventName == "scene_unloaded")
                return "buildIndex, sceneName";

            return "-1, string.Empty";
        }

        void EmitStatements(std::ostream& output, const std::vector<Statement>& statements, const std::string& eventName, int indent);

        void Indent(std::ostream& output, int indent)
        {
            for (int i = 0; i < indent; ++i)
                output << "    ";
        }

        void EmitStatements(std::ostream& output, const std::vector<Statement>& statements, const std::string& eventName, int indent)
        {
            const auto textArgs = TextArgsForEvent(eventName);
            for (const auto& statement : statements)
            {
                switch (statement.kind)
                {
                case StatementKind::Say:
                    Indent(output, indent);
                    output << "Log.Info(RoseVText(" << EscapeCSharpString(statement.value) << ", " << textArgs << "));\n";
                    break;
                case StatementKind::Warn:
                    Indent(output, indent);
                    output << "Log.Warning(RoseVText(" << EscapeCSharpString(statement.value) << ", " << textArgs << "));\n";
                    break;
                case StatementKind::Error:
                    Indent(output, indent);
                    output << "Log.Error(RoseVText(" << EscapeCSharpString(statement.value) << ", " << textArgs << "));\n";
                    break;
                case StatementKind::Emit:
                    Indent(output, indent);
                    output << "Events.Publish(" << EscapeCSharpString(statement.value) << ", RoseVText(" << EscapeCSharpString(statement.value2) << ", " << textArgs << "));\n";
                    break;
                case StatementKind::CSharp:
                    if (Trim(statement.value).empty())
                        break;
                    Indent(output, indent);
                    output << "// RoseV raw C# block\n";
                    {
                        std::istringstream raw(statement.value);
                        std::string rawLine;
                        while (std::getline(raw, rawLine))
                        {
                            if (rawLine.empty())
                                continue;
                            Indent(output, indent);
                            output << rawLine << "\n";
                        }
                    }
                    break;
                case StatementKind::Unity:
                    Indent(output, indent);
                    output << "#if UNITY_REFERENCES\n";
                    {
                        std::istringstream raw(statement.value);
                        std::string rawLine;
                        while (std::getline(raw, rawLine))
                        {
                            if (rawLine.empty())
                                continue;
                            Indent(output, indent);
                            output << rawLine << "\n";
                        }
                    }
                    Indent(output, indent);
                    output << "#endif\n";
                    break;
                case StatementKind::Let:
                    Indent(output, indent);
                    output << "var " << statement.value << " = " << InferCSharpValue(statement.value2) << ";\n";
                    break;
                case StatementKind::Set:
                    Indent(output, indent);
                    output << statement.value << " = " << InferCSharpValue(statement.value2) << ";\n";
                    break;
                case StatementKind::Add:
                    Indent(output, indent);
                    output << statement.value << " += " << InferCSharpValue(statement.value2) << ";\n";
                    break;
                case StatementKind::Subtract:
                    Indent(output, indent);
                    output << statement.value << " -= " << InferCSharpValue(statement.value2) << ";\n";
                    break;
                case StatementKind::Multiply:
                    Indent(output, indent);
                    output << statement.value << " *= " << InferCSharpValue(statement.value2) << ";\n";
                    break;
                case StatementKind::Divide:
                    Indent(output, indent);
                    output << statement.value << " /= " << InferCSharpValue(statement.value2) << ";\n";
                    break;
                case StatementKind::Call:
                    Indent(output, indent);
                    output << statement.value << "();\n";
                    break;
                case StatementKind::If:
                    Indent(output, indent);
                    output << "if (" << RoseExpressionToCSharp(statement.value) << ")\n";
                    Indent(output, indent);
                    output << "{\n";
                    EmitStatements(output, statement.children, eventName, indent + 1);
                    Indent(output, indent);
                    output << "}\n";
                    break;
                case StatementKind::Repeat:
                    Indent(output, indent);
                    output << "for (var __rosevRepeat = 0; __rosevRepeat < " << InferCSharpValue(statement.value) << "; __rosevRepeat++)\n";
                    Indent(output, indent);
                    output << "{\n";
                    EmitStatements(output, statement.children, eventName, indent + 1);
                    Indent(output, indent);
                    output << "}\n";
                    break;
                case StatementKind::While:
                    Indent(output, indent);
                    output << "while (" << RoseExpressionToCSharp(statement.value) << ")\n";
                    Indent(output, indent);
                    output << "{\n";
                    EmitStatements(output, statement.children, eventName, indent + 1);
                    Indent(output, indent);
                    output << "}\n";
                    break;
                case StatementKind::Return:
                    Indent(output, indent);
                    output << "return";
                    if (!statement.value.empty())
                        output << " " << InferCSharpValue(statement.value);
                    output << ";\n";
                    break;
                case StatementKind::Throw:
                    Indent(output, indent);
                    output << "throw new InvalidOperationException(" << EscapeCSharpString(statement.value) << ");\n";
                    break;
                case StatementKind::Try:
                    Indent(output, indent);
                    output << "try\n";
                    Indent(output, indent);
                    output << "{\n";
                    EmitStatements(output, statement.children, eventName, indent + 1);
                    Indent(output, indent);
                    output << "}\n";
                    Indent(output, indent);
                    output << "catch (Exception ex)\n";
                    Indent(output, indent);
                    output << "{\n";
                    Indent(output, indent + 1);
                    output << "Log.Error(ex, \"RoseV try block failed.\");\n";
                    Indent(output, indent);
                    output << "}\n";
                    break;
                case StatementKind::NativeCall:
                    Indent(output, indent);
                    output << "RoseNative." << (statement.value.empty() ? "" : statement.value + "_") << statement.value2 << "();\n";
                    break;
                case StatementKind::Every:
                    Indent(output, indent);
                    output << "__rosevEvery" << statement.counterIndex << "++;\n";
                    Indent(output, indent);
                    output << "if (__rosevEvery" << statement.counterIndex << " % " << statement.numberValue << " == 0)\n";
                    Indent(output, indent);
                    output << "{\n";
                    EmitStatements(output, statement.children, eventName, indent + 1);
                    Indent(output, indent);
                    output << "}\n";
                    break;
                case StatementKind::Key:
                    Indent(output, indent);
                    output << "#if UNITY_REFERENCES\n";
                    Indent(output, indent);
                    output << "if (UnityEngine.Input.GetKeyDown(UnityEngine.KeyCode." << SanitizeIdentifier(statement.value, "None") << "))\n";
                    Indent(output, indent);
                    output << "{\n";
                    EmitStatements(output, statement.children, eventName, indent + 1);
                    Indent(output, indent);
                    output << "}\n";
                    Indent(output, indent);
                    output << "#endif\n";
                    break;
                }
            }
        }

        bool HasEvent(const Program& program, const std::string& eventName)
        {
            return std::any_of(program.events.begin(), program.events.end(), [&](const EventBlock& block) {
                return block.name == eventName;
            });
        }

        void EmitSettingsBind(std::ostream& output, const Program& program)
        {
            for (const auto& setting : program.settings)
            {
                const auto type = RoseTypeToCSharp(setting.type);
                Indent(output, 3);
                output << SettingFieldName(setting) << " = Config.Bind<" << type << ">(\"General\", "
                    << EscapeCSharpString(setting.name) << ", "
                    << RoseValueToCSharp(setting.type, setting.defaultValue) << ", "
                    << EscapeCSharpString(setting.description) << ");\n";
            }
        }

        void EmitEvent(std::ostream& output, const Program& program, const EventBlock& block)
        {
            Indent(output, 2);
            output << MethodSignatureForEvent(block.name) << "\n";
            Indent(output, 2);
            output << "{\n";
            if (block.name == "load")
                EmitSettingsBind(output, program);

            EmitStatements(output, block.statements, block.name, 3);
            Indent(output, 2);
            output << "}\n\n";
        }

        void EmitProgram(const Program& program, const CompileOptions& options)
        {
            const auto nativeCalls = CollectNativeCalls(program);
            if (!nativeCalls.empty() && program.nativeUnits.empty())
                throw std::runtime_error("RoseV native call used without a native <c|cpp|asm> declaration.");

            std::ofstream output(options.outputPath);
            if (!output)
                throw std::runtime_error("Could not create output file: " + options.outputPath);

            output << "// <auto-generated by RoseV native compiler>\n";
            output << "// Source hash: 0x" << std::hex << std::uppercase << program.sourceHash << std::dec << "\n";
            if (options.writeDebugComments)
                output << "// Generated C# backend for Unity/RoseMod/MelonLoader/BepInEx.\n";
            output << "#nullable enable\n";
            std::unordered_set<std::string> emittedImports;
            output << "using System;\n";
            output << "using System.Runtime.InteropServices;\n";
            emittedImports.insert("System|");
            emittedImports.insert("System.Runtime.InteropServices|");
            auto imports = program.imports;
            AddImportPack(imports, "csharp");
            if (program.useUnity)
                AddImportPack(imports, "unity");
            if (program.useMelonLoader)
                AddImportPack(imports, "melonloader");
            if (program.useBepInEx)
                AddImportPack(imports, "bepinex");
            if (program.useRoseMod)
                AddImportPack(imports, "rosemod");
            for (const auto& importName : imports)
            {
                if (!emittedImports.insert(importName.name + "|" + importName.condition).second)
                    continue;
                if (!importName.condition.empty())
                    output << "#if " << importName.condition << "\n";
                output << "using " << importName.name << ";\n";
                if (!importName.condition.empty())
                    output << "#endif\n";
            }
            if (emittedImports.insert("RoseMod.DevKit|").second)
                output << "using RoseMod.DevKit;\n";
            output << "\n";
            output << "namespace " << program.nameSpace << "\n";
            output << "{\n";
            Indent(output, 1);
            output << "[RoseModMetadata("
                << EscapeCSharpString(program.id) << ", "
                << EscapeCSharpString(program.name) << ", "
                << EscapeCSharpString(program.version) << ", "
                << EscapeCSharpString(program.author) << ", Description = "
                << EscapeCSharpString(program.description) << ")]\n";
            Indent(output, 1);
            output << "public sealed class " << program.className << " : RoseModBase\n";
            Indent(output, 1);
            output << "{\n";

            int counter = 0;
            for (const auto& block : program.events)
                counter += CountCounters(block.statements);
            for (int i = 0; i < counter; ++i)
            {
                Indent(output, 2);
                output << "private int __rosevEvery" << i << ";\n";
            }

            for (const auto& field : program.fields)
            {
                Indent(output, 2);
                output << "private " << RoseTypeToCSharp(field.type) << " " << field.name << " = " << RoseValueToCSharp(field.type, field.defaultValue) << ";\n";
            }

            for (const auto& setting : program.settings)
            {
                Indent(output, 2);
                output << "private RoseConfigEntry<" << RoseTypeToCSharp(setting.type) << ">? " << SettingFieldName(setting) << ";\n";
            }

            for (const auto& nativeUnit : program.nativeUnits)
            {
                Indent(output, 2);
                output << "// RoseV native " << nativeUnit.language << " companion: " << nativeUnit.path << "\n";
                Indent(output, 2);
                output << "// Build this into " << nativeUnit.alias << ".dll and keep it beside the managed mod DLL.\n";
            }

            if (!nativeCalls.empty())
            {
                Indent(output, 2);
                output << "private static class RoseNative\n";
                Indent(output, 2);
                output << "{\n";
                for (const auto& call : nativeCalls)
                {
                    const auto library = NativeLibraryForAlias(program, call.alias);
                    const auto wrapperName = call.alias.empty() ? call.functionName : call.alias + "_" + call.functionName;
                    Indent(output, 3);
                    output << "[DllImport(" << EscapeCSharpString(library) << ", EntryPoint = "
                        << EscapeCSharpString(call.functionName) << ", CallingConvention = CallingConvention.Cdecl)]\n";
                    Indent(output, 3);
                    output << "internal static extern void " << wrapperName << "();\n";
                }
                Indent(output, 2);
                output << "}\n";
            }

            for (const auto& memberBlock : program.memberBlocks)
            {
                Indent(output, 2);
                output << "// RoseV raw C# members\n";
                std::istringstream raw(memberBlock);
                std::string rawLine;
                while (std::getline(raw, rawLine))
                {
                    if (rawLine.empty())
                        continue;
                    Indent(output, 2);
                    output << rawLine << "\n";
                }
            }

            if (counter > 0 || !program.settings.empty() || !program.nativeUnits.empty() || !program.memberBlocks.empty())
                output << "\n";

            if (!program.settings.empty() && !HasEvent(program, "load"))
            {
                Indent(output, 2);
                output << "public override void OnLoad()\n";
                Indent(output, 2);
                output << "{\n";
                EmitSettingsBind(output, program);
                Indent(output, 2);
                output << "}\n\n";
            }

            for (const auto& block : program.events)
                EmitEvent(output, program, block);

            for (const auto& function : program.functions)
            {
                Indent(output, 2);
                output << "private void " << function.name << "()\n";
                Indent(output, 2);
                output << "{\n";
                EmitStatements(output, function.statements, "load", 3);
                Indent(output, 2);
                output << "}\n\n";
            }

            Indent(output, 2);
            output << "private string RoseVText(string value, int buildIndex, string sceneName)\n";
            Indent(output, 2);
            output << "{\n";
            Indent(output, 3);
            output << "return value\n";
            Indent(output, 4);
            output << ".Replace(\"{loader}\", Context.Loader.ToString())\n";
            Indent(output, 4);
            output << ".Replace(\"{backend}\", Context.Backend.ToString())\n";
            Indent(output, 4);
            output << ".Replace(\"{game}\", Context.GameRoot)\n";
            Indent(output, 4);
            output << ".Replace(\"{mod}\", Context.Metadata.Name)\n";
            Indent(output, 4);
            output << ".Replace(\"{version}\", Context.Metadata.Version)\n";
            Indent(output, 4);
            output << ".Replace(\"{scene}\", sceneName ?? string.Empty)\n";
            Indent(output, 4);
            output << ".Replace(\"{buildIndex}\", buildIndex.ToString());\n";
            Indent(output, 2);
            output << "}\n";

            Indent(output, 1);
            output << "}\n";
            output << "}\n";
        }
    }

    int Compile(const CompileOptions& options, std::string& error)
    {
        try
        {
            auto lines = LoadSource(options.inputPath);
            auto program = ParseProgram(lines, options);

            int counter = 0;
            for (auto& block : program.events)
                AssignCounters(block.statements, counter);

            EmitProgram(program, options);
            return 0;
        }
        catch (const std::exception& ex)
        {
            error = ex.what();
            return 1;
        }
    }
}

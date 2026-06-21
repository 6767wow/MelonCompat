#ifdef _WIN32
#define ROSEV_EXPORT extern "C" __declspec(dllexport)
#else
#define ROSEV_EXPORT extern "C"
#endif

ROSEV_EXPORT void RoseVNativeCppPing()
{
    /*
       C++ companions should export C ABI functions so C# P/Invoke can find
       the exact entry point name.
    */
}

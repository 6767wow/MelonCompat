#ifdef _WIN32
#define ROSEV_EXPORT __declspec(dllexport)
#else
#define ROSEV_EXPORT
#endif

ROSEV_EXPORT void RoseVNativePing(void)
{
    /*
       Build this file into RoseVNativeSample.dll when you want RoseV to call
       native C code through `native call RoseVNativeSample.RoseVNativePing`.
    */
}

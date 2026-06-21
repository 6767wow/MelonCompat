# RoseV Native Companions

RoseV can declare C, C++, and x64 assembly companion files:

```rosev
native c "Native/RoseVNativeSample.c" as RoseVNativeSample
native call RoseVNativeSample.RoseVNativePing
```

The RoseV compiler generates C# P/Invoke declarations. You still build the native files into DLLs with MSVC, clang, or another native toolchain, then place the DLL beside the managed mod DLL.

Rules:

- The alias should match the DLL name without `.dll`.
- Export C ABI functions for C++ by using `extern "C"`.
- Keep native calls tiny and fast. Unity object access should normally stay in C#/RoseV.

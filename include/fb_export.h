#ifndef FB_EXPORT_H
#define FB_EXPORT_H

#if defined(_WIN32) || defined(_WIN64)
  #define API_EXPORT __declspec(dllexport)
#else
  #define API_EXPORT __attribute__((visibility("default")))
#endif

#endif // FB_EXPORT_H

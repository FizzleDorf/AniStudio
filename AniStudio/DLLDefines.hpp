#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#ifdef ANI_EXPORTS
#define ANI_API __declspec(dllexport)
#else
#define ANI_API __declspec(dllimport)
#endif
#else
#define ANI_API
#endif

#ifdef __cplusplus
}
#endif

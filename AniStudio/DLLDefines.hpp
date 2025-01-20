// DLLDefines.hpp
#pragma once

#ifdef _WIN32
#ifdef ANI_EXPORTS
#define ANI_API __declspec(dllexport)
#else
#define ANI_API __declspec(dllimport)
#endif

#ifdef PLUGIN_EXPORTS
#define PLUGIN_API __declspec(dllexport)
#else
#define PLUGIN_API __declspec(dllimport)
#endif
#else
#define ANI_API
#define PLUGIN_API
#endif
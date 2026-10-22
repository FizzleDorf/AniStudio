#pragma once

#ifdef _WIN32
#  ifdef BUILDING_ANIENGINE
#    define ANI_ENGINE_API __declspec(dllexport)
#  else
#    define ANI_ENGINE_API __declspec(dllimport)
#  endif
#else
#  ifdef BUILDING_ANIENGINE
#    define ANI_ENGINE_API __attribute__((visibility("default")))
#  else
#    define ANI_ENGINE_API
#  endif
#endif
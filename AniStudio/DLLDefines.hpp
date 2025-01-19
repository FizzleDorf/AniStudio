// DLLDefines.hpp
#pragma once

#ifdef _WIN32
#ifdef ANI_EXPORTS
#define ANI_API __declspec(dllexport)
#else
#define ANI_API __declspec(dllimport)
#endif
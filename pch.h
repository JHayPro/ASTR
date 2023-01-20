// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define _CRT_SECURE_NO_WARNINGS 1
#define _WIN32_WINNT_WIN10 0x0A00

#define PLUGIN_VERSION 1
#define PLUGIN_NAME "ASTR"

#define PLUGIN_PATH_LEN 21

#include <windows.h>
#include <comdef.h>
#include <shlobj.h>]
#include <shlwapi.h>

#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <algorithm>

#include <random>
#include <unordered_map>
#include <vector>
#include <array>
#include <utility> 

#include "F4SE/PluginAPI.h"
#include "f4se_common/f4se_version.h"

#pragma comment(lib, "Shlwapi.lib")

using namespace std;
#endif //PCH_H


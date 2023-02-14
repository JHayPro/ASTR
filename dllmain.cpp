// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "PathDestroyer.h"
#include "PathData.h"
#include "MusicConvert.h"

IDebugLog glog;
PluginHandle g_pluginHandle = kPluginHandle_Invalid;

__declspec(dllexport) PathDataParent* PathDataCreate()
{
	return new PathData();
}
__declspec(dllexport) MusicConvertParent* MusicConvertCreate(PathDataParent& _pathData)
{
	return new MusicConvert(_pathData);
}
__declspec(dllexport) PathDestoryerParent* PathDestoryerCreate(PathDataParent& _pathData)
{
	return new PathDestoryer(_pathData);
}
PathDataParent* pathData = PathDataCreate();

extern "C"
{
	__declspec(dllexport) bool F4SEPlugin_Query(const F4SEInterface* f4se, PluginInfo* info)
	{
		char logPath[MAX_PATH];
		sprintf_s(logPath, MAX_PATH, "%s%s.log", "\\My Games\\Fallout4\\F4SE\\", PLUGIN_NAME);
		glog.OpenRelative(CSIDL_MYDOCUMENTS, logPath);

		_MESSAGE("%s", PLUGIN_NAME, PLUGIN_VERSION);

		info->infoVersion = PluginInfo::kInfoVersion;
		info->name = PLUGIN_NAME;
		info->version = PLUGIN_VERSION;

		g_pluginHandle = f4se->GetPluginHandle();

		if (f4se->runtimeVersion < RUNTIME_VERSION_1_10_163) {
			_MESSAGE("ERROR: Version ", CURRENT_RELEASE_RUNTIME, " Required!");
			return false;
		}

		if (f4se->isEditor)
		{
			_MESSAGE("ERROR: isEditor Is True");
			return false;
		}

		return true;
	}

	__declspec(dllexport) bool F4SEPlugin_Load(const F4SEInterface* f4se)
	{
		MusicConvertParent* MCinst = MusicConvertCreate(*pathData);

		for(const auto& errorString : pathData->errorReport)
		_MESSAGE(errorString.c_str());

		if (pathData->errorReport.size() > 0) 
			_MESSAGE("%s loaded with warning", PLUGIN_NAME);
		else
			_MESSAGE("%s loaded ", PLUGIN_NAME);

		delete MCinst;
		delete pathData;

		return true;
	}
};

//BOOL APIENTRY DllMain( HMODULE hModule,
//                       DWORD  ul_reason_for_call,
//                       LPVOID lpReserved
//                     )
//{
//    switch (ul_reason_for_call)
//    {
//    case DLL_PROCESS_ATTACH:
//
//        break;
//
//    case DLL_THREAD_ATTACH:
//
//        break;
//
//    case DLL_THREAD_DETACH:
//
//        break;
//
//    case DLL_PROCESS_DETACH:
//
//		if (pathData->eDelete) {
//			PathDestoryerParent* PDinst = PathDestoryerCreate(*pathData);
//			delete PDinst;
//		}
//		message::displayMessage("b");
//		delete pathData;
//        break;
//    }
//    return TRUE;
//}


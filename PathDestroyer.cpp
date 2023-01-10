#include "pch.h"
#include "PathDestroyer.h"

PathDestoryer::PathDestoryer(PathDataParent& _pathData)
{
	//while (true) {
	if (_pathData.eRMenuM) {
		//if (__std_fs_remove(path::to_wstring(_pathData.outputPath).c_str())._Error == __std_win_error::_Success)
		__std_fs_remove(path::to_wstring(_pathData.outputPath).c_str())._Error == __std_win_error::_Success;
		//	break;
	}
	else {
		bool pass = false;
		for (const auto& entry : filesystem::recursive_directory_iterator(_pathData.outputPath)) {
			if (entry.path().filename().string() == "special")
				if (path::toLower(entry.path().filename().string()).at(4) == 'm')
					continue;
			if (__std_fs_remove(path::to_wstring(entry.path().string()).c_str())._Error == __std_win_error::_Success)
				pass = true;
			else
				break;
		}
		//if (pass)
			//break;
	}
	//Sleep(0.25);
//}
}

#include "pch.h"
#include "PathBuilder.h"

PathBuilder::PathBuilder(PathDataParent& _pathData) : pathData(&_pathData)
{
	findDLLPath();
	readInis();

	if(rebuildIniFlag) rebuildIni();

}

PathBuilder::~PathBuilder()
{
	delete pathData;
}

void PathBuilder::findDLLPath()
{
	TCHAR path[_MAX_PATH + 1];
	message::checkForError(GetModuleFileName(GetModuleHandle(("ASTR.dll")), path, sizeof(path) / sizeof(path[0])));
	for (int i = 0; path[i + PLUGIN_PATH_LEN] != 0; i++)
		dataPathSS << path[i];

	pathData->inputPath = dataPathSS.str() + "F4SE\\Plugins\\ASTR_Music_Dir\\";
	pathData->outputPath = dataPathSS.str() + "Music\\";
	pathData->iniPath = dataPathSS.str() + "F4SE\\Plugins\\ASTR.ini";

}

void PathBuilder::readInis()
{
	ifstream in(pathData->iniPath);
	message::checkForFalseError(in.is_open(), "ASTR.ini Not Found");

	in.ignore((numeric_limits<streamsize>::max)(), ':');

	if (in.get() == '0')
		firstTimeCheck();

	in.ignore((numeric_limits<streamsize>::max)(), ':');
	pathData->dMusicRD = in.get() == '1' ? true : false;

	in.ignore((numeric_limits<streamsize>::max)(), ':');
	pathData->eMainM = in.get() == '1' ? true : false;

	in.ignore((numeric_limits<streamsize>::max)(), ':');
	pathData->eCombatM = in.get() == '1' ? true : false;

	in.ignore((numeric_limits<streamsize>::max)(), ':');
	pathData->eSpecialM = in.get() == '1' ? true : false;

	in.ignore((numeric_limits<streamsize>::max)(), ':');
	pathData->eMenuM = in.get() == '1' ? true : false;

	in.close();
}

void PathBuilder::firstTimeCheck()
{
	if(!(path::verifyPath(dataPathSS.str() + "ASTR - All DLC.esp") || path::verifyPath(dataPathSS.str()) + "ASTR - No DLC.esp"))
		message::displayErrorMessage("ASTR esp is missing, check the mod was installed properly");

	rebuildIniFlag = true;
}

void PathBuilder::rebuildIni()
{
	ofstream outF(pathData->iniPath);
	outF << string(";First Time Install Check Success:1") +
		"\n\n;Disable music reload and delete:" + to_string(pathData->dMusicRD) +
		"\n\n;Enable default main music:" + to_string(pathData->eMainM) +
		"\n;Enable default combat music:" + to_string(pathData->eCombatM) +
		"\n;Enable default special music:" + to_string(pathData->eSpecialM) +
		"\n\n;Enable menu music replacement:" + to_string(pathData->eMenuM);
	outF.close();
}

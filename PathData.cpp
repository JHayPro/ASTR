#include "pch.h"
#include "PathData.h"

PathData::PathData()
{
	rebuildIniFlag = false;
	findDLLPath();
	readInis();

	if (rebuildIniFlag) rebuildIni();

}

void PathData::findDLLPath()
{
	TCHAR path[_MAX_PATH + 1];
	message::checkForError(GetModuleFileName(GetModuleHandle(("ASTR.dll")), path, sizeof(path) / sizeof(path[0])));
	for (int i = 0; path[i + PLUGIN_PATH_LEN] != 0; i++)
		dataPathSS << path[i];

	inputPath = dataPathSS.str() + "F4SE\\Plugins\\ASTR_Music_Dir\\";
	outputPath = dataPathSS.str() + "Music\\";
	iniPath = dataPathSS.str() + "F4SE\\Plugins\\ASTR.ini";
	iniTrackPath = dataPathSS.str() + "F4SE\\Plugins\\ASTR_Music_Dir\\ASTR_Track_Settings.ini";
	silenceTrackPath = dataPathSS.str() + "F4SE\\Plugins\\ASTR_Music_Dir\\_ASTR_Assets\\silence.xwm";
}

void PathData::readInis()
{
	ifstream in(iniPath);
	message::checkForFalseError(in.is_open(), "ASTR.ini Not Found");

	in.ignore((numeric_limits<streamsize>::max)(), ':');

	if (in.get() == '0')
		firstTimeCheck();

	in.ignore((numeric_limits<streamsize>::max)(), ':');
	eVerifyM = in.get() == '1' ? true : false;

	in.ignore((numeric_limits<streamsize>::max)(), ':');
	eTrackNumRan = in.get() == '1' ? true : false;

	//in.ignore((numeric_limits<streamsize>::max)(), ':');
	//eDelete = in.get() == '1' ? true : false;

	in.ignore((numeric_limits<streamsize>::max)(), ':');
	eDefMusic.insert({ "ambient", in.get() == '1' ? true : false });

	in.ignore((numeric_limits<streamsize>::max)(), ':');
	eDefMusic.insert({ "combat", in.get() == '1' ? true : false });

	in.ignore((numeric_limits<streamsize>::max)(), ':');
	eDefMusic.insert({ "special", in.get() == '1' ? true : false });

	in.ignore((numeric_limits<streamsize>::max)(), ':');
	eDefMusic.insert({ "palette", in.get() == '1' ? true : false });

	in.ignore((numeric_limits<streamsize>::max)(), ':');
	eDefMusic.insert({ "menu", in.get() == '1' ? true : false });

	in.ignore((numeric_limits<streamsize>::max)(), ':');
	dCustM = in.get() == '1' ? true : false;

	in.ignore((numeric_limits<streamsize>::max)(), ':');
	eRMenuM = in.get() == '1' ? true : false;

	in.ignore((numeric_limits<streamsize>::max)(), ':');
	eRanMenuM = in.get() == '1' ? true : false;
}

void PathData::firstTimeCheck()
{
	if (!path::verifyPath(inputPath) || !path::verifyPath(iniPath) || !path::verifyPath(iniTrackPath) || !path::verifyPath(silenceTrackPath))
		message::displayErrorMessage("ASTR_Music_Dir, ASTR.ini, or ASTR_Track_Settings.ini was not found, please re-install the mod or set ASTR.ini ;First Time Install Check Success:1 to bypass this message (likely will lead to other errors)");
	rebuildIniFlag = true;
}

void PathData::rebuildIni()
{
	ofstream outF(iniPath);
	outF << string(";First Time Install Check Success:1") +
		"\n\n;Verify music:" + to_string(eVerifyM) +
		"\n;Ignore track numbers (Recommended for better randomization):" + to_string(eTrackNumRan) +
		//"\n;Delete music on game exit (Skips menu music when replacement disabled):" + to_string(eDelete) +
		"\n\nSet all to 0 to remove all default music"
		"\n;Allow inclusion of default ambient music:" + to_string(eDefMusic.at("ambient")) +
		"\n;Allow inclusion of default combat music:" + to_string(eDefMusic.at("combat")) +
		"\n;Allow inclusion of default special music:" + to_string(eDefMusic.at("special")) +
		"\n;Allow inclusion of default palette music:" + to_string(eDefMusic.at("palette")) +
		"\n;Allow inclusion of default menu music:" + to_string(eDefMusic.at("menu")) +
		"\n\n;Disallow inclusion of default music listed in custom:" + to_string(dCustM) +
		"\n\n;Enable menu music replacement (Must be .wav):" + to_string(eRMenuM) +
		"\n;Enable selection of random ambient music to replace menu music (Above setting must also be enabled):" + to_string(eRanMenuM);

	outF.close();
}
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

	dataPath = dataPathSS.str();
	inputPath = dataPath + "F4SE\\Plugins\\ASTR_Music_Dir\\";
	outputPath = dataPath + "Music\\";
	iniPath = dataPath + "F4SE\\Plugins\\ASTR.ini";
	iniTrackPath = dataPath + "F4SE\\Plugins\\ASTR_Music_Dir\\ASTR_Track_Settings.ini";
	silenceTrackPath = dataPath + "F4SE\\Plugins\\ASTR_Music_Dir\\_ASTR_Assets\\silence.xwm";
}

void PathData::readInis()
{
	bool firstTimeCheckFlag = false;
	ifstream in(iniPath);
	message::checkForFalseError(in.is_open(), "ASTR.ini Not Found");

	in.ignore((numeric_limits<streamsize>::max)(), ':');
	firstTimeCheckFlag = in.get() == '0' ? true : false;

	in.ignore((numeric_limits<streamsize>::max)(), ':');
	eVerifyM = in.get() == '1' ? true : false;

	in.ignore((numeric_limits<streamsize>::max)(), ':');
	eTrackNumRan = in.get() == '1' ? true : false;

	in.ignore((numeric_limits<streamsize>::max)(), ':');
	eExtTrackCount = in.get() == '1' ? true : false;

	in.ignore((numeric_limits<streamsize>::max)(), ':');
	eWavCon = in.get() == '1' ? true : false;

	in.ignore((numeric_limits<streamsize>::max)(), ':');
	string inputxwmaEncodePath; 
	getline(in, inputxwmaEncodePath);
	
	if (inputxwmaEncodePath.find("default") != string::npos) {
		string mainDirPath = dataPath.substr(0, dataPath.size() - 5);
		xwmaEncodePath = mainDirPath + "Tools\\Audio\\xwmaencode.exe";
		xwmaEcondeExists = filesystem::exists(xwmaEncodePath);
		rebuildIniFlag = true;
	}
	else {
		xwmaEncodePath = inputxwmaEncodePath;
		xwmaEcondeExists = filesystem::exists(xwmaEncodePath);
	}
	//in.ignore((numeric_limits<streamsize>::max)(), ':');
	//eDelete = in.get() == '1' ? true : false;

	in.ignore((numeric_limits<streamsize>::max)(), ';');
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

	if(firstTimeCheckFlag)
		firstTimeCheck();
}

void PathData::firstTimeCheck()
{
	if (!path::verifyPath(inputPath) || !path::verifyPath(iniPath) || !path::verifyPath(iniTrackPath) || !path::verifyPath(silenceTrackPath))
		message::displayErrorMessage("ASTR_Music_Dir, ASTR.ini, or ASTR_Track_Settings.ini was not found, please re-install the mod or set ASTR.ini ;First Time Install Check Success:1 to bypass this message (likely will lead to other errors)");
	rebuildIniFlag = true;

	if (!xwmaEcondeExists) {
		message::displayMessage("ASTR One Time Warning", "xwmaEncode.exe was not found in \n Fallout 4\\Tools\\Audio\\xwmaencode.exe \n It is HIGHLY recommended!\n Download the creation kit through steam or add it's path to ASTR.ini \n disabling this setting");
		eWavCon = false;
		rebuildIniFlag = true;
	}
	if (eExtTrackCount) {
		string pluginPathDef = dataPath + "ASTR - Extended Track Count.esp";
		string pluginPathDLC = dataPath + "ASTR - All DLC - Extended Track Count.esp";
		if (!filesystem::exists(pluginPathDef) && !filesystem::exists(pluginPathDLC)) {
			message::displayMessage("ASTR One Time Warning", "Extended Track Count is set to enabled, but the required .esp is not installed, disabling this setting");
			eExtTrackCount = false;
			rebuildIniFlag = true;
		}
	}
}

void PathData::rebuildIni()
{
	ofstream outF(iniPath);
	outF << string(";First Time Install Check Success:1") +
		"\n\n;Verify music:" + to_string(eVerifyM) +
		"\n;Ignore track numbers (Recommended for better randomization):" + to_string(eTrackNumRan) +
		"\n;Extend output track count x2 (.esp required):" + to_string(eExtTrackCount) +
		"\n;Enable conversion of input tracks to xwm (Recommended for better performance):" + to_string(eWavCon) +
		"\n;xwmaEncode Path(write \"default\" to reset to creation kit path):" + (xwmaEcondeExists ? xwmaEncodePath : "default") +
		//"\n;Delete music on game exit (Skips menu music when replacement disabled):" + to_string(eDelete) +
		"\n\nSet all to 0 to remove all default music"
		"\n;Allow inclusion of default ambient music:" + to_string(eDefMusic.at("ambient")) +
		"\n;Allow inclusion of default combat music:" + to_string(eDefMusic.at("combat")) +
		"\n;Allow inclusion of default special music:" + to_string(eDefMusic.at("special")) +
		"\n;Allow inclusion of default palette music:" + to_string(eDefMusic.at("palette")) +
		"\n;Allow inclusion of default menu music:" + to_string(eDefMusic.at("menu")) +
		"\n\n;Disallow inclusion of default music listed in custom:" + to_string(dCustM) +
		"\n\n;Enable menu music replacement:" + to_string(eRMenuM) +
		"\n;Enable selection of random ambient music to replace menu music (Above setting must also be enabled):" + to_string(eRanMenuM);

	outF.close();
}
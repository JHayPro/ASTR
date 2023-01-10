#pragma once
#include "pch.h"
#include "cmd.cpp"

class PathDataParent {

public:
	string iniPath, outputPath, inputPath, iniTrackPath, silenceTrackPath, errorReport;

	unordered_map<string, bool> eDefMusic;
	bool eVerifyM, eRanMenuM, eRMenuM, dCustM, eDelete, eTrackNumRan;

	PathDataParent() {
		eVerifyM = eRanMenuM = eRMenuM = dCustM = false;
		errorReport = "";
	}

};

class PathData : public PathDataParent {

private:
	void findDLLPath(), readInis(), firstTimeCheck(), rebuildIni();
	ostringstream dataPathSS;
	bool rebuildIniFlag;

public:
	PathData();
};
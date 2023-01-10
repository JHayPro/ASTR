#pragma once
#include "pch.h"
#include "cmd.cpp"
#include "PathData.h"

class PathBuilderParent {
public:

};

class PathBuilder : public PathBuilderParent {

private:
	PathDataParent* pathData = NULL;
	ostringstream dataPathSS;
	string inputPath, outputPath;

	void findDLLPath(), readInis(), firstTimeCheck(), rebuildIni();
	bool rebuildIniFlag;

public:

	PathBuilder(PathDataParent& _pathData);
	~PathBuilder();
};
#pragma once
#include "pch.h"
#include "cmd.cpp"
#include "PathData.h"

class MusicConvertParent {
public:

};

class TrackData {

public:
	string path = "", album = "", ext = "", outputPath = "", name = "";
	bool defaultAlbum = false;
	bool silence = false;
	TrackData *finale = NULL;

	TrackData() {};
	TrackData(bool _silence) : silence(_silence) {};
	TrackData(string _path, string _ext, string _album) : path(_path), ext(_ext), album(_album){};
	TrackData(string _path, string _ext, string _album, string _outputPath) : path(_path), ext(_ext), album(_album), outputPath(_outputPath) {};
	TrackData(string _path, string _album) : path(_path), album(_album) {};
	TrackData(string _path, string _ext, string _album, TrackData &_finale) : path(_path), ext(_ext), album(_album), finale(&_finale) {};
	TrackData(string _album, bool _defaultAlbum) : album(_album), defaultAlbum(_defaultAlbum) {};

private:

};

class MusicConvert : public MusicConvertParent {

public:
	MusicConvert(PathDataParent& _pathData);

private:
	PathDataParent* pathData = NULL;

	unordered_map<string, vector<TrackData>> inputTrackDataHash;
	unordered_map<string, TrackData> outputTrackDataHash, defaultTrackDataHash, failedTrackDataHash, paletteTrackDataHash;

	unordered_map<string,string> relativeTrackPathsHash;

	array<string, 7> defaultAlbums{ "custom", "ambient", "combat", "special", "menu", "palette", "asset"};

	void buildInputTrackDataHash(), buildOutputTrackDataHash(),
		buildOutputDir(), copyTracks(), buildDefaultTrackDataHash(),
		readTracks(string _line, string _album), createErrorReport(),
		removePalettes();
		//,trackSort(map<string, vector<TrackData>>& _hash);

	bool rMenuMPass;
};
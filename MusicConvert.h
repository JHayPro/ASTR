#pragma once
#include "pch.h"
#include "cmd.cpp"
#include "PathData.h"

class MusicConvertParent {
public:

};

class TrackData {

public:
	string path = "", album = "", ext = "", outputPath = "";
	bool defaultAlbum = false, extendCount = false;
	TrackData *finale = NULL;
	int numberedTrackCount = 0;

	TrackData() {};
	TrackData(string _path, string _ext, string _album) : path(_path), ext(_ext), album(_album){};
	TrackData(string _path, string _ext, string _album, string _outputPath) : path(_path), ext(_ext), album(_album), outputPath(_outputPath) {};
	TrackData(string _path, string _ext, string _album, TrackData &_finale) : path(_path), ext(_ext), album(_album), finale(&_finale) {};
	TrackData(string _album, bool _defaultAlbum, bool _extendCount, int _numberedTrackCount) : album(_album), defaultAlbum(_defaultAlbum), extendCount(_extendCount) { incrementTrackCount(_numberedTrackCount); };

	void incrementTrackCount(int _trackCount) {
		numberedTrackCount = extendCount ? 2  * _trackCount : _trackCount;
	};

private:

};

struct CategoryTrackDataContainer {

	unordered_map<string, vector<TrackData>> inputTrackDataHash;
	unordered_map<string, TrackData> outputTrackDataHash, defaultTrackDataHash;

	CategoryTrackDataContainer() {};
};

class MusicConvert : public MusicConvertParent {

public:
	MusicConvert(PathDataParent& _pathData);

private:
	PathDataParent* pathData = NULL;

	unordered_map <string, CategoryTrackDataContainer> trackDataHash;
	unordered_map<string,string> relativeTrackPathsHash;
	unordered_map<string, vector<string>> failedTrackDataHash;

	array<string, 7> musicCategories{ "custom", "ambient", "combat", "special", "menu", "palette", "asset"};

	void buildInputTrackDataHash(), buildOutputTrackDataHash(),
		buildOutputDir(), copyTracks(), buildDefaultTrackDataHash(),
		createErrorReport(),convertEncoding(string _inputTrack, string _outputTrack),
		buildMenuOutputTrackDataHash(), removeUnusedDir(),
		insertDefaultTrack(string _relativeFullPath, unordered_map<std::string, TrackData> &_defaultTrackDataHash, int _trackCount, string _category, bool _addPalettePath);

	bool rMenuMPass;
};
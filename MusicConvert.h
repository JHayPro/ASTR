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
	bool defaultAlbum = false, extendCount = false, numberedTrack = false;
	TrackData* finale = NULL;
	int numberedTrackCount = 0;

	TrackData() {};
	TrackData(string _path, string _ext, string _album) : path(_path), ext(_ext), album(_album) {};
	TrackData(string _path, string _ext, string _album, string _outputPath) : path(_path), ext(_ext), album(_album), outputPath(_outputPath) {};
	TrackData(string _path, string _ext, string _album, TrackData& _finale) : path(_path), ext(_ext), album(_album), finale(&_finale) {};
	TrackData(string _album, bool _defaultAlbum, bool _extendCount, int _numberedTrackCount, bool _numberedTrack) :
		album(_album), defaultAlbum(_defaultAlbum), extendCount(_extendCount), numberedTrack(_numberedTrack) {
		incrementTrackCount(_numberedTrackCount);
	};

	void incrementTrackCount(int _trackCount) {
		numberedTrackCount = extendCount ? 2 * _trackCount : _trackCount;
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

	unordered_map <string, CategoryTrackDataContainer> trackDataHash, nonReplacerTrackDataHash;
	unordered_map<string, string> relativeTrackPathsHash;
	unordered_map<string, vector<string>> failedTrackDataHash;

	array<string, 7> musicCategories{ "custom", "ambient", "combat", "special", "menu", "palette", "asset" };

	int randNum(int _max);

	void buildInputTrackDataHash(), buildOutputTrackDataHash(),
		buildOutputDir(), copyTracks(), buildDefaultTrackDataHash(),
		createErrorReport(), convertEncoding(string _inputTrack, string _outputTrack),
		buildMenuOutputTrackDataHash(), removeUnusedDir(),
		buildNonReplacerTrackDataHash(), assignNonReplacerTrackData(vector<string>& _dirStack, const string &_musicCategoryInputDir, const string &_musicCategory),
		insertDefaultTrack(const string &_relativeFullPath, unordered_map<std::string, TrackData>& _defaultTrackDataHash, const int &_trackCount, const string &_category, const bool &_addPalettePath, const bool &_numberedTrack),
		insertFailedTrack(const string& _album, const string& _fullPath, const string& _parentDir, const bool& _removeExe),
		trackConvert(string &_trackPath, string &_ext), verifyNonEmptyInput(),
		insertTrackData(const string &_relativePath, const TrackData& _trackData, unordered_map<string, vector<TrackData>>& _inputTrackDataHash);

	bool convRelativePathToNumbered(string& _relativeFullPath, const unordered_map<std::string, TrackData>& _defaultTrackDataHash),
		rMenuMPass, convertWarn;

	TrackData* createFinaleTrackData(const string& _relativeFullPath, const string& _trackPath, const string& _album);
};
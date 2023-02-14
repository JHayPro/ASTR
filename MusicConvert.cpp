#include "pch.h"
#include "MusicConvert.h"

MusicConvert::MusicConvert(PathDataParent& _pathData) : pathData(&_pathData)
{
	rMenuMPass = convertWarn = false;

	buildDefaultTrackDataHash();
	buildNonReplacerTrackDataHash();
	buildInputTrackDataHash();
	verifyNonEmptyInput();
	buildMenuOutputTrackDataHash();
	buildOutputTrackDataHash();
	buildOutputDir();
	copyTracks();
	if (pathData->eVerifyM)
		createErrorReport();
	removeUnusedDir();
}

int MusicConvert::randNum(int _max)
{
	static random_device rd;
	static mt19937 g(rd());

	uniform_int_distribution<int> dist(0, _max);
	return dist(g);
}

void MusicConvert::trackConvert(string& _trackPath, string& _ext)
{
	if (_ext == ".wav" && pathData->xwmaEcondeExists) {
		if (!convertWarn && pathData->eWavCon)
			convertWarn = pathData->eWavCon = !(message::displayMessageReturn("ASTR Message", "Converting .wav files to .xwm, expect moderate delay, do not force quit, press ok to continue\n press cancel to quit, this option can be disabled with ;Enable conversion of input tracks to xwm:0") == 2);

		if (pathData->eWavCon) {
			string originalTrackPath = _trackPath;
			_ext = ".xwm";
			_trackPath = _trackPath.substr(0, _trackPath.size() - 4) + _ext;
			convertEncoding(originalTrackPath, _trackPath);
			message::checkForError(filesystem::remove(originalTrackPath));
		}
	}
}

void MusicConvert::insertFailedTrack(const string& _album, const string& _fullPath, const string& _parentDir, const bool& _removeExe)
{
	if (pathData->eVerifyM)
		if (failedTrackDataHash.find(_album) == failedTrackDataHash.end())
			failedTrackDataHash.insert({ _album, {_fullPath.substr(_parentDir.size(), _fullPath.size() - _parentDir.size() - (int(_removeExe) * 6))} });
		else
			failedTrackDataHash.at(_album).emplace_back(_fullPath.substr(_parentDir.size(), _fullPath.size() - _parentDir.size() - (int(_removeExe) * 6)));
}

void MusicConvert::createErrorReport()
{
	if (!failedTrackDataHash.empty()) {

		pathData->errorReport.emplace_back("Tracks which were not recognized by the game:");
		for (auto errorData : failedTrackDataHash) {
			ostringstream errorSS;
			errorSS << "\n\nTracks from album: " + errorData.first + "\n";

			const auto& errorVector = errorData.second;
			for (const auto& relativeFullPath : errorVector)
				errorSS << relativeFullPath + "\n";

			errorSS << "\n";
			pathData->errorReport.emplace_back(errorSS.str());
		}

		message::displayMessage("ALR_WARNING", "Some Tracks were not recognized by the game, view ASTR f4se log for details or disable this message with ;Verify music:0");
	}
	if (!rMenuMPass && pathData->eRMenuM)
		message::displayMessage("ALR_WARNING", "No viable tracks were found to replace main menu music, ensure .wav files are included in selected track pool, or install the creation kit to enable xwm conversion");
}

void MusicConvert::buildDefaultTrackDataHash()
{
	ifstream in(pathData->iniTrackPath);
	message::checkForFalseError(in.is_open(), "ASTR.ini Not Found");

	string line;
	for (const auto& category : musicCategories) {
		trackDataHash.insert({ category, CategoryTrackDataContainer() });
		auto& defaultTrackDataHash = trackDataHash.at(category).defaultTrackDataHash;
		bool addPalettePath = category == "palette" && !pathData->eDefMusic.at("palette");

		in.ignore((numeric_limits<streamsize>::max)(), ':');
		getline(in, line);

		for (int i = 0; i < line.size(); i++) {
			if (line.at(i) != ' ') {
				ostringstream trackSS;

				for (i = i; i < line.size(); i++) {
					char curChar = line.at(i);
					if (curChar != ',')
						trackSS << curChar;
					else
						break;
				}

				const auto& trackDataInput = trackSS.str();
				const auto& trackCountString = path::findPrecedingToChar(trackDataInput, '.');
				const auto& trackCount = stoi(trackCountString);
				const auto& relativeFullPath = trackDataInput.substr(0, trackDataInput.size() - (trackCountString.size() + 1));
				if (!pathData->eTrackNumRan)
					if (trackCount > 1 || trackDataInput.at(trackDataInput.size() - 3) == '_')
						for (int i = 0; i < trackCount * 2; i++)
							insertDefaultTrack(relativeFullPath + path::intToTwoDigitString(i + 1), defaultTrackDataHash, 1, category, addPalettePath, true);
					else
						insertDefaultTrack(trackDataInput.substr(0, trackDataInput.size() - 2), defaultTrackDataHash, 1, category, addPalettePath, false);
				else
					if (trackCount == 1 && trackDataInput.at(trackDataInput.size() - 3) != '_')
						insertDefaultTrack(relativeFullPath, defaultTrackDataHash, trackCount, category, addPalettePath, false);
					else
						insertDefaultTrack(relativeFullPath, defaultTrackDataHash, trackCount, category, addPalettePath, true);
			}
		}
	}
	in.close();
}

void MusicConvert::insertDefaultTrack(const string& _relativeFullPath, unordered_map<std::string, TrackData>& _defaultTrackDataHash, const int& _trackCount, const string& _category, const bool& _addPalettePath, const bool& _numberedTrack)
{
	if (_defaultTrackDataHash.find(_relativeFullPath) == _defaultTrackDataHash.end())
		_defaultTrackDataHash.insert({ _relativeFullPath, {TrackData(_category, true, pathData->eExtTrackCount && pathData->eTrackNumRan, _trackCount, _numberedTrack)} });

	if (_addPalettePath) {
		string relativeTrackPath = _relativeFullPath.substr(0, _relativeFullPath.size() - path::findPrecedingToChar(_relativeFullPath, '\\').size());
		if (relativeTrackPathsHash.find(relativeTrackPath) == relativeTrackPathsHash.end())
			relativeTrackPathsHash.insert({ relativeTrackPath, pathData->outputPath + relativeTrackPath });
	}
}

void MusicConvert::buildNonReplacerTrackDataHash()
{
	if (!filesystem::exists(pathData->nonReplacerInputPath))
		filesystem::create_directory(pathData->nonReplacerInputPath);

	string nonReplacerMusicCategories[4] = { "ambient", "combat", "special", "menu" };
	for (const auto& musicCategory : nonReplacerMusicCategories) {
		string musicCategoryInputDir = pathData->nonReplacerInputPath + musicCategory;

		if (!filesystem::exists(musicCategoryInputDir))
			filesystem::create_directory(musicCategoryInputDir);

		for (const auto& defaultTrackData : trackDataHash.at(musicCategory).defaultTrackDataHash) {
			auto relativeFullPath = defaultTrackData.first;

			if (!pathData->eTrackNumRan && path::fileContainsNumber(relativeFullPath, relativeFullPath.size() - 1))
				relativeFullPath = relativeFullPath.substr(0, relativeFullPath.size() - 2);

			string relativePath = path::getDirectoryParent(relativeFullPath) + "\\";
			string fileName = relativeFullPath.substr(relativePath.size(), relativeFullPath.size());
			string inputPath = musicCategoryInputDir + relativePath + "_" + fileName;

			if (!filesystem::exists(inputPath))
				filesystem::create_directories(inputPath);
		}
	}

	for (const auto& musicCategory : nonReplacerMusicCategories) {
		string musicCategoryInputDir = pathData->nonReplacerInputPath + musicCategory;

		if (filesystem::exists(musicCategoryInputDir)) {
			vector<string> dirStack = { musicCategoryInputDir };
			while (!dirStack.empty())
				assignNonReplacerTrackData(dirStack, musicCategoryInputDir, musicCategory);
		}
	}
}

void MusicConvert::assignNonReplacerTrackData(vector<string>& _dirStack, const string& _musicCategoryInputDir, const string& _musicCategory)
{
	vector<pair<string, string>> trackFullPaths, trackDirFullPaths;
	string searchPath = _dirStack.back();

	_dirStack.pop_back();

	for (const auto& entry : filesystem::directory_iterator{ searchPath }) {
		const auto& fullPath = path::toLower(entry.path().string());

		if (entry.is_directory()) {
			if (entry.path().filename().string().at(0) == '_') {
				string relativeFullPath = path::removeLeadingUnderscore(fullPath.substr(_musicCategoryInputDir.size(), fullPath.size()));

				trackDirFullPaths.emplace_back(fullPath, relativeFullPath);
			}
			else
				_dirStack.emplace_back(fullPath);
		}
		else {
			string ext = "." + path::getExtension(fullPath);

			if (ext == ".wav" || ext == ".xwm")
				trackFullPaths.emplace_back(fullPath, ext);
		}
	}

	int finalTrackDirIndex = trackDirFullPaths.size() - 1;
	auto& inputTrackDataHash = trackDataHash.at(_musicCategory).inputTrackDataHash;
	const auto& defaultTrackDataHash = trackDataHash.at(_musicCategory).defaultTrackDataHash;
	bool combatMusicCategory = _musicCategory == "combat";

	for (const auto& fullPathData : trackFullPaths) {
		string trackPath = fullPathData.first;
		string ext = fullPathData.second;
		const auto& album = "Non_Replacer_" + path::getDirectory(trackPath);

		if (!combatMusicCategory || trackPath.find("_finale") == string::npos) {
			string relativeFullPath = "";

			while (finalTrackDirIndex >= 0) {
				const auto& outputTrackDirIndex = randNum(finalTrackDirIndex);
				relativeFullPath = trackDirFullPaths.at(outputTrackDirIndex).second;

				if (!pathData->eTrackNumRan && !convRelativePathToNumbered(relativeFullPath, defaultTrackDataHash))
					insertFailedTrack(album, trackPath, searchPath, false);

				if (defaultTrackDataHash.find(relativeFullPath) != defaultTrackDataHash.end())
					break;

				trackDirFullPaths.erase(trackDirFullPaths.begin() + outputTrackDirIndex);
				finalTrackDirIndex--;
			}

			if (finalTrackDirIndex >= 0) {
				trackConvert(trackPath, ext);
				TrackData* finaleTrackData = combatMusicCategory ? createFinaleTrackData(relativeFullPath, trackPath, album) : NULL;
				TrackData trackData = TrackData(trackPath, ext, album, *finaleTrackData);
				insertTrackData(relativeFullPath, trackData, inputTrackDataHash);
			}
			else
				insertFailedTrack(album, trackPath, searchPath, false);
		}
	}

	for (const auto& trackDirFullData : trackDirFullPaths) {
		string trackPathDir = trackDirFullData.first;
		string relativeFullPath = trackDirFullData.second;
		string album = "Non_Replacer_" + relativeFullPath;

		if (!combatMusicCategory || trackPathDir.find("_finale") == string::npos) {
			if (!pathData->eTrackNumRan && !convRelativePathToNumbered(relativeFullPath, defaultTrackDataHash))
				insertFailedTrack(album, trackPathDir, searchPath, false);

			if (defaultTrackDataHash.find(relativeFullPath) != defaultTrackDataHash.end()) {
				for (const auto& trackEntry : filesystem::directory_iterator{ trackPathDir }) {
					auto ext = trackEntry.path().extension().string();

					if (ext == ".wav" || ext == ".xwm") {
						string trackPath = trackEntry.path().string();
						trackConvert(trackPath, ext);
						TrackData* finaleTrackData = combatMusicCategory ? createFinaleTrackData(relativeFullPath, trackPath, album) : NULL;
						TrackData trackData = TrackData(trackPath, ext, album, *finaleTrackData);
						insertTrackData(relativeFullPath, trackData, inputTrackDataHash);
					}
				}
			}
			else
				insertFailedTrack(album, trackPathDir, searchPath, false);
		}
	}
}

void MusicConvert::buildInputTrackDataHash()
{
	if (!path::verifyPath(pathData->silenceTrackPath))
		message::displayMessage("ASTR_WARNING", "_ASTR_Assets\\silence.xwm has been deleted, please reinstall the mod");

	for (const auto& albumDirEntry : filesystem::directory_iterator{ pathData->inputPath }) {
		if (albumDirEntry.is_directory()) {
			string album = albumDirEntry.path().filename().string();

			if (album != "_Non_Replacer_Mod_Dir") {
				string albumPath = albumDirEntry.path().string();

				for (const auto& trackEntry : filesystem::recursive_directory_iterator{ albumPath }) {
					string ext = path::toLower(trackEntry.path().filename().extension().string());

					if (ext == ".wav" || ext == ".xwm") {
						string trackPath = path::toLower(trackEntry.path().string());
						pair <const string, CategoryTrackDataContainer>* trackDataCategory = NULL;
						int trackNumberIndex = trackPath.size() - 5;
						bool useNumTrack = pathData->eTrackNumRan && path::fileContainsNumber(trackPath, trackNumberIndex);
						int relativeFullPathSubtract = useNumTrack ? 6 : 4;
						string relativeFullPath = trackPath.substr(albumPath.size(), trackPath.size() - albumPath.size() - relativeFullPathSubtract);

						if (relativeFullPath.find("finale") == string::npos) {

							for (auto& _trackDataCategory : trackDataHash) {
								const auto& defaultTrackDataHash = _trackDataCategory.second.defaultTrackDataHash;
								if (defaultTrackDataHash.find(relativeFullPath) != defaultTrackDataHash.end())
									trackDataCategory = &_trackDataCategory;
							}

							if (trackDataCategory != NULL) {
								trackConvert(trackPath, ext);
								TrackData* finaleTrackData = trackDataCategory->first == "combat" ? createFinaleTrackData(relativeFullPath, trackPath, album) : NULL;
								TrackData trackData = TrackData(trackPath, ext, album, *finaleTrackData);
								insertTrackData(relativeFullPath, trackData, trackDataCategory->second.inputTrackDataHash);
							}
							else
								insertFailedTrack(album, trackPath, albumPath, true);
						}
					}
				}
			}
		}
	}
}

void MusicConvert::verifyNonEmptyInput()
{
	string checkMusicCategories[5] = { "ambient", "combat", "special", "menu", "palette" };
	for (const auto& category : checkMusicCategories)
		if (!trackDataHash.at(category).inputTrackDataHash.empty())
			break;
		else if (category == checkMusicCategories[4])
			message::displayMessage("ASTR_WARNING", "No tracks were found in \"Data\\F4SE\\Plugins\\ASTR_Music_dir\\\"\nEnsure albums were installed and in the correct location");
}

void MusicConvert::insertTrackData(const string& _relativeFullPath, const TrackData& _trackData, unordered_map<string, vector<TrackData>>& _inputTrackDataHash)
{
	if (_inputTrackDataHash.find(_relativeFullPath) == _inputTrackDataHash.end())
		_inputTrackDataHash.insert({ _relativeFullPath, {_trackData} });
	else
		_inputTrackDataHash.at(_relativeFullPath).emplace_back(_trackData);

	string relativeTrackPath = path::getDirectoryParent(_relativeFullPath);
	if (relativeTrackPathsHash.find(relativeTrackPath) == relativeTrackPathsHash.end())
		relativeTrackPathsHash.insert({ relativeTrackPath, pathData->outputPath + relativeTrackPath });
}

bool MusicConvert::convRelativePathToNumbered(string& _relativeFullPath, const unordered_map<std::string, TrackData>& _defaultTrackDataHash)
{
	if (_defaultTrackDataHash.find(_relativeFullPath) == _defaultTrackDataHash.end()) {
		if (_defaultTrackDataHash.find(_relativeFullPath + "01") == _defaultTrackDataHash.end())
			return false;

		int finalTrackIndex = 0;
		for (finalTrackIndex = 0; _defaultTrackDataHash.find(_relativeFullPath + path::intToTwoDigitString(finalTrackIndex + 1)) != _defaultTrackDataHash.end(); finalTrackIndex++) {}
		
		int randomTrackNumber = (finalTrackIndex > 1) ? randNum(finalTrackIndex - 1) : 0;
		_relativeFullPath = _relativeFullPath + path::intToTwoDigitString(randomTrackNumber + 1);
	}
	return true;
}

TrackData* MusicConvert::createFinaleTrackData(const string& _relativeFullPath, const string& _trackPath, const string& _album)
{
	if ((_relativeFullPath.find("boss") != string::npos) ||
		(pathData->eTrackNumRan ? (_relativeFullPath == "\\mus_combat_") :
			(_relativeFullPath.find("\\mus_combat_" + _relativeFullPath.at(_relativeFullPath.size() - 2)) != string::npos))) {

		string finaleTrackPath = _trackPath.substr(0, _trackPath.size() - 4) + "_finale";
		string wavFinaleTrackPath = finaleTrackPath + ".wav";
		string xwmFinaleTrackPath = finaleTrackPath + ".xwm";

		if (filesystem::exists(xwmFinaleTrackPath))
			return new TrackData(xwmFinaleTrackPath, ".xwm", _album);
		else if (filesystem::exists(wavFinaleTrackPath))
			return new TrackData(wavFinaleTrackPath, ".wav", _album);
		else
			return new TrackData(pathData->silenceTrackPath, ".xwm", "_ASTR_Assets");
	}
	return NULL;
}

void MusicConvert::buildMenuOutputTrackDataHash()
{
	string relativeFullPath = "\\special\\mus_maintheme";

	if (pathData->eRanMenuM && pathData->eRMenuM) {
		const auto& inputTrackDataHash = trackDataHash.at("ambient").inputTrackDataHash;
		vector<string> relativeFullPathVector;
		random_device rd;
		mt19937 g(rd());

		for (const auto& inputTrackData : inputTrackDataHash)
			relativeFullPathVector.emplace_back(inputTrackData.first);

		shuffle(relativeFullPathVector.begin(), relativeFullPathVector.end(), g);

		for (const auto& ambientRelativeFullPath : relativeFullPathVector) {
			auto trackDataVector = inputTrackDataHash.at(ambientRelativeFullPath);

			if (!pathData->xwmaEcondeExists) {
				for (int i = 0; i < trackDataVector.size(); i++)
					if (trackDataVector.at(i).ext != ".wav") {
						trackDataVector.erase(trackDataVector.begin() + i);
						i--;
					}

				if (trackDataVector.size() == 0)
					continue;
			}

			if (pathData->eDefMusic.at("menu") && (trackDataHash.at("custom").defaultTrackDataHash.find(relativeFullPath) == trackDataHash.at("custom").defaultTrackDataHash.end()))
				trackDataVector.emplace_back(trackDataHash.at("menu").defaultTrackDataHash.at(relativeFullPath));

			int finalTrackIndex = trackDataVector.size() - 1;
			int outputAlbumIndex = (finalTrackIndex > 0) ? randNum(finalTrackIndex) : 0;
			auto trackData = trackDataVector.at(outputAlbumIndex);
			trackData.outputPath = pathData->outputPath + "special\\mus_maintheme";

			trackDataHash.at("menu").outputTrackDataHash.insert({ relativeFullPath, trackData });
			rMenuMPass = true;
			break;
		}
	}
	else if (pathData->eRMenuM) {
		auto& categoryTrackDataHash = trackDataHash.at("menu");
		const auto& inputTrackDataHash = categoryTrackDataHash.inputTrackDataHash;

		if (inputTrackDataHash.find(relativeFullPath) != inputTrackDataHash.end()) {
			auto trackDataVector = inputTrackDataHash.at(relativeFullPath);
			auto defaultTrack = categoryTrackDataHash.defaultTrackDataHash.at(relativeFullPath);

			if (!pathData->xwmaEcondeExists) {
				for (int i = 0; i < trackDataVector.size(); i++)
					if (trackDataVector.at(i).ext != ".wav") {
						trackDataVector.erase(trackDataVector.begin() + i);
						i--;
					}

				if (trackDataVector.size() == 0)
					trackDataVector.emplace_back(defaultTrack);
			}

			if (pathData->eDefMusic.at("menu") && (trackDataHash.at("custom").defaultTrackDataHash.find(relativeFullPath) == trackDataHash.at("custom").defaultTrackDataHash.end()))
				trackDataVector.emplace_back(defaultTrack);

			int finalTrackIndex = trackDataVector.size() - 1;
			int outputAlbumIndex = (finalTrackIndex > 0) ? randNum(finalTrackIndex) : 0;
			auto trackData = trackDataVector.at(outputAlbumIndex);
			trackData.outputPath = pathData->outputPath + "special\\mus_maintheme";

			trackDataHash.at("menu").outputTrackDataHash.insert({ relativeFullPath, trackData });
			rMenuMPass = true;
		}
	}
	else {
		auto trackData = trackDataHash.at("menu").defaultTrackDataHash.at(relativeFullPath);
		trackData.outputPath = pathData->outputPath + "special\\mus_maintheme";
		trackDataHash.at("menu").outputTrackDataHash.insert({ relativeFullPath, trackData });
	}
}

void MusicConvert::buildOutputTrackDataHash()
{
	string outputMusicCategories[4] = { "ambient", "combat", "special", "palette" };

	for (const auto& category : outputMusicCategories) {
		auto& categoryTrackDataHash = trackDataHash.at(category);
		const auto& inputTrackDataHash = categoryTrackDataHash.inputTrackDataHash;
		const auto& defaultTrackDataHash = categoryTrackDataHash.defaultTrackDataHash;
		auto& outputTrackDataHash = categoryTrackDataHash.outputTrackDataHash;

		for (const auto& defaultTrackData : defaultTrackDataHash) {
			const auto& relativeFullPath = defaultTrackData.first;
			auto defaultTrack = defaultTrackDataHash.at(relativeFullPath);
			const auto& numberedTrackCount = defaultTrackData.second.numberedTrackCount;
			bool useNumTrack = pathData->eTrackNumRan && defaultTrackData.second.numberedTrack;

			if (inputTrackDataHash.find(relativeFullPath) != inputTrackDataHash.end()) {
				auto trackDataVector = inputTrackDataHash.at(relativeFullPath);
				bool includeDefaultTrack = pathData->eDefMusic.at(category) && (trackDataHash.at("custom").defaultTrackDataHash.find(relativeFullPath) == trackDataHash.at("custom").defaultTrackDataHash.end());

				if (includeDefaultTrack) {
					auto sizeTrackDataVector = trackDataVector.size() - 1; //lower weight by one
					int i = 0;

					do {
						trackDataVector.emplace_back(defaultTrack);
						i++;
					} while ((i < numberedTrackCount) && (i < sizeTrackDataVector));
				}

				bool reuseTracks = numberedTrackCount > trackDataVector.size();
				for (int i = int(useNumTrack); i < numberedTrackCount; i++) {

					int finalTrackIndex = trackDataVector.size() - 1;
					int outputAlbumIndex = (finalTrackIndex > 0) ? randNum(finalTrackIndex) : 0;
					auto trackData = trackDataVector.at(outputAlbumIndex);
					TrackData outputTrackDataCopy = TrackData(trackData);
					string outputRelativeFullPath = relativeFullPath + (useNumTrack ? path::intToTwoDigitString(i) : "");
					outputTrackDataCopy.outputPath = pathData->outputPath + outputRelativeFullPath;

					if (trackData.finale != NULL)
						outputTrackDataCopy.finale = new TrackData(trackData.finale->path, trackData.finale->ext, trackData.finale->album, outputTrackDataCopy.outputPath + "_finale");

					outputTrackDataHash.insert({ outputRelativeFullPath, outputTrackDataCopy });

					if (!reuseTracks)
						trackDataVector.erase(trackDataVector.begin() + outputAlbumIndex);
				}
			}
			else {
				bool silentPalette = category == "palette" && !pathData->eDefMusic.at(category);
				int i = 0;

				do {
					i++;
					string outputRelativeFullPath = relativeFullPath + (useNumTrack ? path::intToTwoDigitString(i) : "");
					const auto& outputTrackData = silentPalette ? TrackData(pathData->silenceTrackPath, ".xwm", category, pathData->outputPath + outputRelativeFullPath) : defaultTrack;

					outputTrackDataHash.insert({ outputRelativeFullPath, outputTrackData });
				} while (i < numberedTrackCount);
			}
		}
	}
}


void MusicConvert::buildOutputDir()
{
	if (pathData->eDefMusic.at("palette") && trackDataHash.at("palette").inputTrackDataHash.empty()) {
		string removePaths[3] = { pathData->outputPath + "palettes\\", pathData->outputPath + "dlc03\\palettes\\", pathData->outputPath + "dlc04\\palette\\" };
		for (const auto& removePath : removePaths)
			if (filesystem::is_directory(removePath))
				message::checkForError(filesystem::remove_all(removePath));
	}

	if (!filesystem::is_directory(pathData->outputPath))
		message::checkForError(filesystem::create_directories(pathData->outputPath));

	if (pathData->eRanMenuM) {
		string specialPath = pathData->outputPath + "\\special";
		if (!filesystem::is_directory(specialPath))
			message::checkForError(filesystem::create_directories(specialPath));
	}

	for (const auto& relativeTrackPath : relativeTrackPathsHash) {
		const auto& outputDir = relativeTrackPath.second;

		if (!filesystem::is_directory(outputDir))
			message::checkForError(filesystem::create_directories(outputDir));
	}
}

void MusicConvert::copyTracks()
{
	if (pathData->eRMenuM) {
		for (const auto& outputTrackData : trackDataHash.at("menu").outputTrackDataHash) {
			auto outputPath = outputTrackData.second.outputPath;
			string wavOutputPath = outputPath + ".wav";
			string xwmOutputPath = outputPath + ".xwm";

			if (filesystem::exists(wavOutputPath))
				message::checkForError(filesystem::remove(wavOutputPath));

			if (filesystem::exists(xwmOutputPath))
				message::checkForError(filesystem::remove(xwmOutputPath));

			if (!outputTrackData.second.defaultAlbum)
				if (outputTrackData.second.ext == ".xwm" && pathData->xwmaEcondeExists)
					convertEncoding(outputTrackData.second.path, wavOutputPath);
				else
					filesystem::copy(outputTrackData.second.path, outputPath + outputTrackData.second.ext);
		}
	}

	for (const auto& outputTrackData : trackDataHash.at("combat").outputTrackDataHash) {
		auto outputPath = outputTrackData.second.outputPath;
		string wavOutputPath = outputPath + ".wav";
		string xwmOutputPath = outputPath + ".xwm";

		if (filesystem::exists(wavOutputPath))
			message::checkForError(filesystem::remove(wavOutputPath));

		if (filesystem::exists(xwmOutputPath))
			message::checkForError(filesystem::remove(xwmOutputPath));

		if (!outputTrackData.second.defaultAlbum) {
			filesystem::copy(outputTrackData.second.path, outputPath + outputTrackData.second.ext);

			if (outputTrackData.second.finale != NULL) {
				auto finaleOutputTrackData = outputTrackData.second.finale;
				auto finaleOutputPath = finaleOutputTrackData->outputPath;
				string wavFinaleOutputPath = finaleOutputPath + ".wav";
				string xwmFinaleOutputPath = finaleOutputPath + ".xwm";

				if (filesystem::exists(wavFinaleOutputPath))
					message::checkForError(filesystem::remove(wavFinaleOutputPath));

				if (filesystem::exists(xwmFinaleOutputPath))
					message::checkForError(filesystem::remove(xwmFinaleOutputPath));

				if (!finaleOutputTrackData->defaultAlbum)
					filesystem::copy(finaleOutputTrackData->path, finaleOutputPath + finaleOutputTrackData->ext);
			}
		}
	}

	string outputMusicCategories[3] = { "ambient", "special", "palette" };
	for (const auto& category : outputMusicCategories) {
		for (const auto& outputTrackData : trackDataHash.at(category).outputTrackDataHash) {
			auto outputPath = outputTrackData.second.outputPath;
			string wavOutputPath = outputPath + ".wav";
			string xwmOutputPath = outputPath + ".xwm";

			if (filesystem::exists(wavOutputPath))
				message::checkForError(filesystem::remove(wavOutputPath));

			if (filesystem::exists(xwmOutputPath))
				message::checkForError(filesystem::remove(xwmOutputPath));

			if (!outputTrackData.second.defaultAlbum)
				filesystem::copy(outputTrackData.second.path, outputPath + outputTrackData.second.ext);
		}
	}
}

void MusicConvert::convertEncoding(string _inputTrack, string _outputTrack)
{
	STARTUPINFO info = { sizeof(info) };
	PROCESS_INFORMATION processInfo;
	auto cmd = path::toTCHAR(path::quote(pathData->xwmaEncodePath) + " " + path::quote(_inputTrack) + " " + path::quote(_outputTrack));

	if (CreateProcess(NULL, cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &info, &processInfo))
	{
		WaitForSingleObject(processInfo.hProcess, INFINITE);
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
	}
	if (!filesystem::exists(_outputTrack))
		message::displayErrorMessage("Strange Behavior, conversion to " + path::getExtension(_outputTrack) + " failed, report this bug");
}

void MusicConvert::removeUnusedDir()
{
	vector<pair<int, string>> dirPathDataVector;
	for (const auto& fileEntry : filesystem::recursive_directory_iterator{ pathData->outputPath })
		if (fileEntry.is_directory())
			dirPathDataVector.emplace_back(path::countDirectories(fileEntry.path().string()), fileEntry.path().string());

	std::sort(dirPathDataVector.begin(), dirPathDataVector.end(), [](auto& left, auto& right) {return left.first > right.first; });

	for (const auto& dirPathData : dirPathDataVector) {
		const auto& dirPath = dirPathData.second;

		if (PathIsDirectoryEmptyA(path::toTCHAR(dirPath)) == TRUE)
			if (filesystem::exists(dirPath))
				message::checkForError(filesystem::remove(dirPath));
	}
}
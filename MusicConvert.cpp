#include "pch.h"
#include "MusicConvert.h"

MusicConvert::MusicConvert(PathDataParent& _pathData) : pathData(&_pathData)
{
	rMenuMPass = false;

	buildDefaultTrackDataHash();
	buildInputTrackDataHash();
	buildMenuOutputTrackDataHash();
	buildOutputTrackDataHash();
	buildOutputDir();
	copyTracks();
	if (pathData->eVerifyM)
		createErrorReport();
	removeUnusedDir();
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

				string trackDataInput = trackSS.str();
				string trackCountString = path::findPrecedingToChar(trackDataInput, '.');
				int trackCount = stoi(trackCountString);
				string relativeFullPath = trackDataInput.substr(0, trackDataInput.size() - (trackCountString.size() + 1));
				if (!pathData->eTrackNumRan)
					if (trackCount > 1 || trackDataInput.at(trackDataInput.size() - 3) == '_')
						for (int i = 0; i < trackCount * 2; i++)
							insertDefaultTrack(relativeFullPath + path::intToTwoDigitString(i + 1), defaultTrackDataHash, 0, category, addPalettePath);
					else
						insertDefaultTrack(trackDataInput.substr(0, trackDataInput.size() - 2), defaultTrackDataHash, 0, category, addPalettePath);
				else
					insertDefaultTrack(relativeFullPath, defaultTrackDataHash, trackCount, category, addPalettePath);
			}
		}
	}
	in.close();
}

void MusicConvert::insertDefaultTrack(string _relativeFullPath, unordered_map<std::string, TrackData>& _defaultTrackDataHash, int _trackCount, string _category, bool _addPalettePath)
{
	if (_defaultTrackDataHash.find(_relativeFullPath) == _defaultTrackDataHash.end())
		_defaultTrackDataHash.insert({ _relativeFullPath, {TrackData(_category, true, pathData->eExtTrackCount, _trackCount)} });

	if (_addPalettePath) {
		string relativeTrackPath = _relativeFullPath.substr(0, _relativeFullPath.size() - path::findPrecedingToChar(_relativeFullPath, '\\').size());
		if (relativeTrackPathsHash.find(relativeTrackPath) == relativeTrackPathsHash.end())
			relativeTrackPathsHash.insert({ relativeTrackPath, pathData->outputPath + relativeTrackPath });
	}
}


void MusicConvert::buildInputTrackDataHash()
{
	bool convertWarn = false;

	if (!path::verifyPath(pathData->silenceTrackPath))
		message::displayMessage("ASTR_WARNING", "_ASTR_Assets\\silence.xwm has been deleted, please reinstall the mod");

	for (const auto& albumDirEntry : filesystem::directory_iterator{ pathData->inputPath }) {
		if (albumDirEntry.is_directory()) {
			string album = albumDirEntry.path().filename().string();
			string albumPath = albumDirEntry.path().string();

			for (const auto& trackEntry : filesystem::recursive_directory_iterator{ albumPath }) {
				string ext = path::toLower(trackEntry.path().filename().extension().string());

				if (ext == ".wav" || ext == ".xwm") {
					string trackPath = path::toLower(trackEntry.path().string());
					pair <const string, CategoryTrackDataContainer>* trackDataCategory = NULL;
					int trackNumberIndex = trackPath.size() - 5;
					bool useNumTrack = pathData->eTrackNumRan && path::fileContainsNumber(trackPath, trackNumberIndex);;
					int relativeFullPathSubtract = useNumTrack ? 6 : 4;
					string relativeFullPath = trackPath.substr(albumPath.size(), trackPath.size() - albumPath.size() - relativeFullPathSubtract);

					if (relativeFullPath.find("finale") == string::npos) {

						for (auto& _trackDataCategory : trackDataHash) {
							auto& defaultTrackDataHash = _trackDataCategory.second.defaultTrackDataHash;
							if (defaultTrackDataHash.find(relativeFullPath) != defaultTrackDataHash.end())
								trackDataCategory = &_trackDataCategory;
						}

						if (trackDataCategory == NULL) {
							if (pathData->eVerifyM)
								if (failedTrackDataHash.find(album) == failedTrackDataHash.end())
									failedTrackDataHash.insert({ album, {trackPath.substr(albumPath.size(), trackPath.size() - albumPath.size() - 4)} });
								else
									failedTrackDataHash.at(album).emplace_back(trackPath.substr(albumPath.size(), trackPath.size() - albumPath.size() - 4));
						}
						else {
							auto& inputTrackDataHash = trackDataCategory->second.inputTrackDataHash;
							TrackData* finaleTrackData = NULL;

							if (ext == ".wav" && pathData->xwmaEcondeExists) {
								if (!convertWarn && pathData->eWavCon)
									convertWarn = pathData->eWavCon = !(message::displayMessageReturn("ASTR Message", "Converting .wav files to .xwm, expect moderate delay, do not force quit, press ok to continue\n press cancel to quit, this option can be disabled with ;Enable conversion of input tracks to xwm:0") == 2);

								if (pathData->eWavCon) {
									string originalTrackPath = trackPath;
									ext = ".xwm";
									trackPath = trackPath.substr(0, trackPath.size() - 4) + ext;
									convertEncoding(originalTrackPath, trackPath);
									message::checkForError(filesystem::remove(originalTrackPath));
								}
							}

							if (trackDataCategory->first == "combat" &&
								((relativeFullPath.find("boss") != string::npos) ||
									(useNumTrack ? (relativeFullPath == "\\mus_combat_") :
										(relativeFullPath.find("\\mus_combat_" + relativeFullPath.at(relativeFullPath.size() - 2)) != string::npos)))) {

								string finaleTrackPath = trackPath.substr(0, trackPath.size() - 4) + "_finale";
								string wavFinaleTrackPath = finaleTrackPath + ".wav";
								string xwmFinaleTrackPath = finaleTrackPath + ".xwm";

								if (filesystem::exists(xwmFinaleTrackPath))
									finaleTrackData = new TrackData(xwmFinaleTrackPath, ".xwm", album);
								else if (filesystem::exists(wavFinaleTrackPath))
									finaleTrackData = new TrackData(wavFinaleTrackPath, ".wav", album);
								else
									finaleTrackData = new TrackData(pathData->silenceTrackPath, ".xwm", "_ASTR_Assets");
							}

							if (inputTrackDataHash.find(relativeFullPath) == inputTrackDataHash.end())
								inputTrackDataHash.insert({ relativeFullPath, {TrackData(trackPath, ext, album, *finaleTrackData)} });
							else
								inputTrackDataHash.at(relativeFullPath).emplace_back(TrackData(trackPath, ext, album, *finaleTrackData));

							string relativeTrackPath = relativeFullPath.substr(0, relativeFullPath.size() - trackEntry.path().filename().string().size() + relativeFullPathSubtract);
							if (relativeTrackPathsHash.find(relativeTrackPath) == relativeTrackPathsHash.end())
								relativeTrackPathsHash.insert({ relativeTrackPath, pathData->outputPath + relativeTrackPath });
						}
					}
				}
			}
		}
	}
}

void MusicConvert::createErrorReport()
{
	if (!failedTrackDataHash.empty()) {
		ostringstream errorSS;
		errorSS << "Tracks which were not recognized by the game:";
		for (auto errorData : failedTrackDataHash) {
			errorSS << "\n\nTracks from album: " + errorData.first + "\n";
			const auto& errorVector = errorData.second;
			for (const auto& relativeFullPath : errorVector)
				errorSS << relativeFullPath + "\n";
		}


		pathData->errorReport = errorSS.str().substr(0, errorSS.str().size() - 2);

		message::displayMessage("ALR_WARNING", "Some Tracks were not recognized by the game, view ASTR f4se log for details or disable this message with ;Verify music:0");
	}
	if (!rMenuMPass && pathData->eRMenuM)
		message::displayMessage("ALR_WARNING", "No viable tracks were found to replace main menu music, ensure .wav files are included in selected track pool, or install the creation kit to enable xwm conversion");
}

void MusicConvert::buildMenuOutputTrackDataHash()
{
	random_device rd;
	mt19937 g(rd());
	string relativeFullPath = "\\special\\mus_maintheme";

	if (pathData->eRanMenuM && pathData->eRMenuM) {
		const auto& inputTrackDataHash = trackDataHash.at("ambient").inputTrackDataHash;
		vector<string> relativeFullPathVector;

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
			int outputAlbumIndex = 0;

			if (finalTrackIndex > 0) {
				std::uniform_int_distribution<int> dist(0, finalTrackIndex);
				outputAlbumIndex = dist(g);
			}

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
			int outputAlbumIndex = 0;

			if (finalTrackIndex > 0) {
				std::uniform_int_distribution<int> dist(0, finalTrackIndex);
				outputAlbumIndex = dist(g);
			}

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
	random_device rd;
	mt19937 g(rd());
	array<string, 4>outputMusicCategories = { "ambient", "combat", "special", "palette" };

	for (const auto& category : outputMusicCategories) {
		auto& categoryTrackDataHash = trackDataHash.at(category);
		const auto& inputTrackDataHash = categoryTrackDataHash.inputTrackDataHash;
		const auto& defaultTrackDataHash = categoryTrackDataHash.defaultTrackDataHash;
		auto& outputTrackDataHash = categoryTrackDataHash.outputTrackDataHash;

		for (const auto& defaultTrackData : defaultTrackDataHash) {
			const auto& relativeFullPath = defaultTrackData.first;
			auto defaultTrack = defaultTrackDataHash.at(relativeFullPath);
			const auto& numberedTrackCount = defaultTrackData.second.numberedTrackCount;
			bool useNumTrack = pathData->eTrackNumRan && (numberedTrackCount > 0);

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
					int outputAlbumIndex = 0;

					if (finalTrackIndex > 0) {
						uniform_int_distribution<int> dist(0, finalTrackIndex);
						outputAlbumIndex = dist(g);
					}

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

	array<string, 3> outputMusicCategories = { "ambient", "special", "palette" };
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
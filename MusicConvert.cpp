#include "pch.h"
#include "MusicConvert.h"

MusicConvert::MusicConvert(PathDataParent& _pathData) : pathData(&_pathData)
{
	rMenuMPass = false;

	buildDefaultTrackDataHash();
	buildInputTrackDataHash();
	buildOutputTrackDataHash();
	buildOutputDir();
	copyTracks();
	removePalettes();
	if (pathData->eVerifyM)
		createErrorReport();
}

void MusicConvert::buildInputTrackDataHash()
{
	if (!path::verifyPath(pathData->silenceTrackPath))
		message::displayMessage("ASTR_WARNING", "_ASTR_Assets\\silence.xwm has been deleted, please reinstall the mod");

	for (const auto& albumDirEntry : filesystem::directory_iterator{ pathData->inputPath }) {
		if (albumDirEntry.is_directory()) {
			string album = albumDirEntry.path().filename().string();
			string albumPath = albumDirEntry.path().string();

			for (const auto& trackEntry : filesystem::recursive_directory_iterator{ albumPath }) {
				TrackData* finaleTrackData = NULL;
				string ext = path::toLower(trackEntry.path().filename().extension().string());

				if (ext == ".wav" || ext == ".xwm") {
					string trackPath = path::toLower(trackEntry.path().string());
					string relativeFullPath = trackPath.substr(albumPath.size(), trackPath.size() - albumPath.size() - 4);

					if (defaultTrackDataHash.find(relativeFullPath) != defaultTrackDataHash.end() && defaultTrackDataHash.at(relativeFullPath).album == "combat") {
						if (relativeFullPath.size() > 5) {
							char checkChar = relativeFullPath.at(relativeFullPath.size() - 6);

							if (checkChar == 'o' || checkChar == 'b') {
								string finaleTrackPath = trackPath.substr(0, trackPath.size() - 4) + "_finale";
								string wavFinaleTrackPath = finaleTrackPath + ".wav";
								string xwmFinaleTrackPath = finaleTrackPath + ".xwm";

								if (filesystem::exists(wavFinaleTrackPath))
									finaleTrackData = new TrackData(wavFinaleTrackPath, ".wav", album);
								else if (filesystem::exists(xwmFinaleTrackPath))
									finaleTrackData = new TrackData(xwmFinaleTrackPath, ".xwm", album);
								else //if (pathData->eTrackNumRan)
									finaleTrackData = new TrackData(pathData->silenceTrackPath, ".xwm", "_ASTR_Assets");
							//	else
								//	finaleTrackData = new TrackData("blank", true);
							}
							else if (checkChar == 'f')
								continue;
						}
					}
					if (inputTrackDataHash.find(relativeFullPath) == inputTrackDataHash.end())
						inputTrackDataHash.insert({ relativeFullPath, {TrackData(trackPath, ext, album, *finaleTrackData)} });
					else
						inputTrackDataHash.at(relativeFullPath).emplace_back(TrackData(trackPath, ext, album, *finaleTrackData));

					string relativeTrackPath = relativeFullPath.substr(0, relativeFullPath.size() - trackEntry.path().filename().string().size() + 4);

					if (relativeTrackPathsHash.find(relativeTrackPath) == relativeTrackPathsHash.end())
						relativeTrackPathsHash.insert({ relativeTrackPath, pathData->outputPath + relativeTrackPath });

				}
			}
		}
	}
}

void MusicConvert::createErrorReport()
{
	if (!failedTrackDataHash.empty()) {
		ostringstream errorSS;
		errorSS << "Tracks which were not recognized by the game: ";
		for (auto trackData : failedTrackDataHash)
			errorSS << trackData.first + " album " + trackData.second.album + ", ";

		pathData->errorReport = errorSS.str().substr(0, errorSS.str().size() - 2);

		message::displayMessage("ALR_WARNING", "Some Tracks were not recognized by the game, view ASTR f4se log for details or disable this message with ;Verify music:0");
	}
	if (!rMenuMPass && pathData->eRMenuM)
		message::displayMessage("ALR_WARNING", "No viable tracks were found to replace main menu music, ensure .wav files are included in selected track pool");

}

void MusicConvert::buildDefaultTrackDataHash()
{
	ifstream in(pathData->iniTrackPath);
	message::checkForFalseError(in.is_open(), "ASTR.ini Not Found");

	string line;

	for (const auto& album : defaultAlbums) {
		in.ignore((numeric_limits<streamsize>::max)(), ':');
		getline(in, line);
		readTracks(line, album);
	}

	in.close();
}

void MusicConvert::readTracks(string _line, string _album)
{
	for (int i = 0; i < _line.size(); i++) {

		if (_line.at(i) != ' ') {
			ostringstream trackSS;

			for (i = i; i < _line.size(); i++) {
				char curChar = _line.at(i);
				if (curChar != ',')
					trackSS << curChar;
				else
					break;
			}

			string relativeFullPath = trackSS.str();
			if (_album != "palette") {
				if (defaultTrackDataHash.find(relativeFullPath) == defaultTrackDataHash.end())
					defaultTrackDataHash.insert({ relativeFullPath, {TrackData(_album, true)} });
			}
			else {
				if (paletteTrackDataHash.find(relativeFullPath) == paletteTrackDataHash.end())
					paletteTrackDataHash.insert({ relativeFullPath, {TrackData(pathData->outputPath + relativeFullPath, _album)} });

				if (!pathData->eDefMusic.at("palette")) {
					string relativeTrackPath = relativeFullPath.substr(0, relativeFullPath.size() - path::findPrecedingToChar(relativeFullPath, '\\').size());
					if (relativeTrackPathsHash.find(relativeTrackPath) == relativeTrackPathsHash.end())
						relativeTrackPathsHash.insert({ relativeTrackPath, pathData->outputPath + relativeTrackPath });
				}
			}
		}
	}
}

void MusicConvert::buildOutputTrackDataHash()
{
	random_device rd;
	mt19937 g(rd());

	if (pathData->eRanMenuM && pathData->eRMenuM) {

		vector<string> relativeFullPathVector;

		for (const auto& inputTrackData : inputTrackDataHash)
			relativeFullPathVector.emplace_back(inputTrackData.first);

		shuffle(relativeFullPathVector.begin(), relativeFullPathVector.end(), g);

		for (const auto& relativeFullPath : relativeFullPathVector) {

			if (defaultTrackDataHash.find(relativeFullPath) != defaultTrackDataHash.end() &&
				defaultTrackDataHash.at(relativeFullPath).album == "ambient") {

				auto trackDataVector = inputTrackDataHash.at(relativeFullPath);
				for (int i = 0; i < trackDataVector.size(); i++)
					if (trackDataVector.at(i).ext != ".wav") {
						trackDataVector.erase(trackDataVector.begin() + i);
						i--;
					}

				if (trackDataVector.size() > 0) {
					if (pathData->eDefMusic.at("menu"))
						trackDataVector.emplace_back(defaultTrackDataHash.at("\\special\\mus_maintheme"));

					int finalTrackIndex = trackDataVector.size() - 1;
					int outputAlbumIndex = 0;

					if (finalTrackIndex > 0) {
						std::uniform_int_distribution<int> dist(0, finalTrackIndex);
						outputAlbumIndex = dist(g);
					}
					auto trackData = trackDataVector.at(outputAlbumIndex);
					trackData.outputPath = pathData->outputPath + "special\\mus_maintheme";
					outputTrackDataHash.insert({ relativeFullPath, trackData });
					rMenuMPass = true;
					break;
				}
			}
		}
	}

	for (auto inputTrackData : inputTrackDataHash) {
		const auto& relativeFullPath = inputTrackData.first;
		int trackNumberIndex = relativeFullPath.size() - 1;
		bool useNumTrack = pathData->eTrackNumRan && path::fileContainsNumber(relativeFullPath, trackNumberIndex);;

		if (useNumTrack && relativeFullPath.substr(trackNumberIndex - 1, trackNumberIndex) != "01")
			continue;

		auto trackDataVector = inputTrackData.second;

		if (defaultTrackDataHash.find(relativeFullPath) == defaultTrackDataHash.end()) {
			if (pathData->eVerifyM)
				failedTrackDataHash.insert({ relativeFullPath, trackDataVector.at(0) });
		}
		else {
			const auto& defaultTrack = defaultTrackDataHash.at(relativeFullPath);
			const auto& defaultAlbum = defaultTrack.album;

			if (defaultAlbum != "asset" && (!(defaultAlbum == "menu" && (!pathData->eRMenuM || pathData->eRanMenuM)))) {

				if (defaultAlbum == "menu") {
					for (int i = 0; i < trackDataVector.size(); i++)
						if (trackDataVector.at(i).ext != ".wav") {
							trackDataVector.erase(trackDataVector.begin() + i);
							i--;
						}

					if (trackDataVector.size() > 0)
						rMenuMPass = true;
					else
						trackDataVector.emplace_back(defaultTrack);
				}

				bool includeDefaultTrack = (!(pathData->dCustM && defaultAlbum == "custom") && (defaultAlbum == "custom" || pathData->eDefMusic.at(defaultAlbum)));

				int indCount = 1;
				string nonIndRelativeFullPath = "";
				if (useNumTrack) {
					nonIndRelativeFullPath = relativeFullPath.substr(0, relativeFullPath.size() - 2);

					string indRelativeFullPath = nonIndRelativeFullPath + path::intToTwoDigitString(indCount);
					for (indCount = 2; defaultTrackDataHash.find(indRelativeFullPath) != defaultTrackDataHash.end(); indCount++) {

						if (inputTrackDataHash.find(indRelativeFullPath) != inputTrackDataHash.end()) {
							auto indTrackDataVector = inputTrackDataHash.at(indRelativeFullPath);
							trackDataVector.insert(trackDataVector.end(), indTrackDataVector.begin(), indTrackDataVector.end());
						}
						if (includeDefaultTrack)
							trackDataVector.emplace_back(defaultTrackDataHash.at(indRelativeFullPath));

						indRelativeFullPath = nonIndRelativeFullPath + path::intToTwoDigitString(indCount);
					}
					indCount--;
				}
				else if (includeDefaultTrack)
					trackDataVector.emplace_back(defaultTrack);

				bool reuseTracks = indCount > trackDataVector.size();
				for (int i = int(useNumTrack); i < indCount; i++) {

					int finalTrackIndex = trackDataVector.size() - 1;
					int outputAlbumIndex = 0;

					if (finalTrackIndex > 0) {
						uniform_int_distribution<int> dist(0, finalTrackIndex);
						outputAlbumIndex = dist(g);
					}

					auto trackData = trackDataVector.at(outputAlbumIndex);

					if (useNumTrack)
						trackData.outputPath = pathData->outputPath + nonIndRelativeFullPath + path::intToTwoDigitString(i);
					else
						trackData.outputPath = pathData->outputPath + relativeFullPath;

					//if (trackData.finale != NULL)
					//	if (!reuseTracks) {
					//		trackData.finale->outputPath = trackData.outputPath + "_finale";
					//		outputTrackDataHash.insert({ outputTrackRelativeFullPath, trackData });
					//	}
					//	else {
					TrackData outputTrackDataCopy = TrackData(trackData);
					if (trackData.finale != NULL) 
						outputTrackDataCopy.finale = new TrackData(trackData.finale->path, trackData.finale->ext, trackData.finale->album, outputTrackDataCopy.outputPath + "_finale");
					
					outputTrackDataHash.insert({ useNumTrack ? nonIndRelativeFullPath + path::intToTwoDigitString(i) : relativeFullPath, outputTrackDataCopy });
					//	}



					//if (trackData.finale != NULL) {
						//message::displayMessage(outputTrackDataHash.at(useNumTrack ? nonIndRelativeFullPath + path::intToTwoDigitString(i) : relativeFullPath).outputPath);
						//message::displayMessage(outputTrackDataHash.at(useNumTrack ? nonIndRelativeFullPath + path::intToTwoDigitString(i) : relativeFullPath).finale->outputPath);
					//}
					if (!reuseTracks)
						trackDataVector.erase(trackDataVector.begin() + outputAlbumIndex);
				}
			}
		}
	}


}

void MusicConvert::buildOutputDir()
{
	if (!filesystem::is_directory(pathData->outputPath))
		message::checkForError(filesystem::create_directories(pathData->outputPath));

	for (const auto& relativeTrackPath : relativeTrackPathsHash) {
		const auto& outputDir = relativeTrackPath.second;

		if (!filesystem::is_directory(outputDir))
			message::checkForError(filesystem::create_directories(outputDir));
	}
}

void MusicConvert::copyTracks()
{
	for (const auto& outputTrackData : outputTrackDataHash) {
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
				//message::displayMessage(outputTrackData.second.outputPath);
				//message::displayMessage(outputTrackData.second.finale->outputPath);
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
}

void MusicConvert::removePalettes()
{
	string testPalettePath = pathData->outputPath + "palettes\\a\\day\\mus_palette_trees_day_a_01.xwm";

	if (!filesystem::exists(testPalettePath) && !pathData->eDefMusic.at("palette")) {
		for (const auto& paletteTrackData : paletteTrackDataHash) {
			auto outputPath = paletteTrackData.second.path + ".xwm";

			if (filesystem::exists(outputPath))
				message::checkForError(filesystem::remove(outputPath));

			filesystem::copy(pathData->silenceTrackPath, outputPath);
		}
	}
	else if (filesystem::exists(testPalettePath) && pathData->eDefMusic.at("palette")) {
		for (const auto& paletteTrackData : paletteTrackDataHash) {
			auto outputPath = paletteTrackData.second.path + ".xwm";

			if (filesystem::exists(outputPath))
				message::checkForError(filesystem::remove(outputPath));
		}

		string removePaths[3] = { pathData->outputPath + "palettes\\", pathData->outputPath + "dlc03\\palettes\\", pathData->outputPath + "dlc04\\palette\\" };
		for (const auto& removePath : removePaths)
			if (filesystem::is_directory(removePath))
				message::checkForError(filesystem::remove_all(removePath));
	}
}
/*
void MusicConvert::trackSort(map<string, vector<TrackData>>& _hash)
{
	vector<pair<string, vector<TrackData>>> pairVector;
	for (auto& i : _hash)
		pairVector.emplace_back(i);

	sort(pairVector.begin(), pairVector.end(),
		[](const pair<string, vector<TrackData>>& _a, const pair<string, vector<TrackData>>& _b)
		{return path::findAfterChar(_a.first, '\\') < path::findAfterChar(_b.first, '\\'); });
}*/

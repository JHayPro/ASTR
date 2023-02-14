#include "pch.h"

#ifndef GLOB_H
#define GLOB_H

class message {
public:

	static void displayCOMErrorMessage(HRESULT _hr)
	{
		_com_error err(_hr);
		MessageBox(NULL, err.ErrorMessage(), "ASTR_ERROR", MB_SETFOREGROUND);
		exit(EXIT_FAILURE);
	}

	static void displayErrorMessage(string _title, string _message)
	{
		MessageBox(NULL, _message.c_str(), _title.c_str(), MB_SETFOREGROUND);
		exit(EXIT_FAILURE);
	}

	static void displayErrorMessage(string _message)
	{
		MessageBox(NULL, _message.c_str(), "ASTR_ERROR", MB_SETFOREGROUND);
		exit(EXIT_FAILURE);
	}

	static void displayMessage(string _title, string _message)
	{
		MessageBox(NULL, _message.c_str(), _title.c_str(), MB_SETFOREGROUND);
	}

	static int displayMessageReturn(string _title, string _message)
	{
		return MessageBox(NULL, _message.c_str(), _title.c_str(), MB_SETFOREGROUND | MB_OKCANCEL);
	}

	static void displayMessage(string _message)
	{
		MessageBox(NULL, _message.c_str(), "ASTR_MESSAGE", MB_SETFOREGROUND);
	}

	template <typename T> static T displayMessage(T _message)
	{
		MessageBox(NULL, to_string(_message).c_str(), "ASTR_MESSAGE", MB_SETFOREGROUND);
	}

	static void checkForError(HRESULT _hr) {
		if (FAILED(_hr))
			message::displayCOMErrorMessage(_hr);
	}

	static void checkForFalseError(bool _r, string _message) {
		if (!_r)
			message::displayErrorMessage("ASTR_ERROR", _message);
	}

	static void checkForFalse(bool _r, string _message) {
		if (!_r)
			message::displayMessage("ASTR_WARNING", _message);
	}
};

class path {
public:

	static bool verifyPath(string _path)
	{
		return filesystem::exists(_path);
	}

	static void verifyPathError(string _path, string _errorMsg)
	{
		if (!verifyPath(_path)) message::displayErrorMessage(_errorMsg);
	}

	static string toLower(string _input)
	{
		transform(_input.begin(), _input.end(), _input.begin(), [](unsigned char _c) { return tolower(_c); });
		return _input;
	}

	static wstring to_wstring(string _input)
	{
		return wstring(_input.begin(), _input.end());
	}

	static string getDirectoryParent(string _path)
	{
		return removeAfterChar(_path, '\\');
	}

	static string getDirectory(string _path)
	{
		return findPrecedingToChar(_path, '\\');
	}

	static string getDirectoryFromBegin(string _path, int _distance)
	{
		return _path.substr(0, _path.size() - _distance);
	}

	static string getDirectoryFromEnd(string _path, int _distance)
	{
		return _path.substr(_path.size() - _distance, _path.size());
	}

	static string getExtension(string _path)
	{
		return findPrecedingToChar(_path, '.');
	}

	static int countDirectories(string _path) {
		int count = 0;
		for (auto chr : _path)
			if (chr == '\\')
				count++;
		return count;
	}

	static string vectorToString(vector<string> _vector) {
		ostringstream tempSS;
		for (const auto& curString : _vector)
			tempSS << curString + ", ";
		
		return tempSS.str().substr(0, tempSS.str().size() - 2);
	}

	static string removeAfterChar(string _path, char _desChar) {
		return _path.substr(0, _path.size() - 1 - findPrecedingToChar(_path, '\\').size());
	}

	template <typename T> static T findPrecedingToChar(T _path, char _desChar)
	{
		if (_path.back() == _desChar)
			_path.pop_back();

		T tempS;
		int tempI;
		for (tempI = 1; _path.at(_path.size() - tempI) != _desChar; tempI++) {}
		for (int i = 1; i < tempI; i++)
			tempS.push_back(_path.at(_path.size() - tempI + i));
		return tempS;
	}

	template <typename T> static T findAfterChar(T _path, char _desChar)
	{
		if (_path.back() == _desChar)
			_path.pop_back();

		T tempS;
		int tempI;
		for (tempI = 1; _path.at(_path.size() - tempI) != _desChar; tempI++) {}
		return _path.substr(_path.size() , _path.size() - 1);
	}

	static bool fileContainsNumber(string _path, int _pos) {
		return (48 <= int(_path.at(_pos)) && int(_path.at(_pos)) <= 57);
	}

	static string intToTwoDigitString(int _num) {
		return _num > 9 ? to_string(_num) : "0" + to_string(_num);
	}

	static string quote(string _in) {
		return "\"" + _in + "\"";
	}

	static TCHAR* toTCHAR(string _in) {
		TCHAR* temp = new TCHAR[_in.size() + 1];
		temp[_in.size()] = 0;
		copy(_in.begin(), _in.end(), temp);
		return temp;
	}

	static string removeLeadingUnderscore(string _in) {
		auto i = _in.size() - 1;
		for (; _in.at(i) != '\\'; --i) {};
		_in.erase(_in.begin() + i + 1);
		return _in;
	}
};
#endif
#pragma once

#include <string>


class Dialog
{
	//private:
	//	char const * lTheHexColor;
	//	unsigned char lRgbColor[3];

public:
	//from tiny file dialogs
	char const* lTmp = nullptr;
	int lIntValue = 0;
	char const* lWillBeGraphicMode = nullptr;
	char const* lFilterPatterns[4] = { "*.txt", "*.text", "*.json", "*csv" };
	char lBuffer[1024] = {};
	char lString[1024] = {};

	std::string title;
	std::string result;

	Dialog();
	virtual ~Dialog() {};
	void init();
	virtual std::string run() = 0;
};


class InputDialog : public Dialog
{
private:
	//What's the reason asking for input?
	std::string description;
	//What did the user return?
	std::string userInput;

public:
	InputDialog(std::string _description);
	std::string run();
};


class ErrorDialog : public Dialog
{
private:
	std::string message;

public:
	ErrorDialog(std::string _message);
	std::string run();
};


//Yes/No question dialog
class QuestionDialog : public Dialog
{
private:
	std::string question;
	bool bResult = false;

public:
	QuestionDialog(std::string _question);
	bool getBResult();
	std::string run();
};


class FilesystemDialog : public Dialog
{
public:
	//from tiny file dialogs
	FILE* lIn = nullptr;
	char const* lTheSelectFolderName;
	char const* lTheSaveFileName;
	char const* lTheOpenFileName;

	FilesystemDialog(std::string _startFolder);

	//my stuff
	std::string startFolder;
};


class ChooseFolderDialog : public FilesystemDialog
{
public:
	ChooseFolderDialog(std::string _startFolder);
	std::string run();
};


class SaveFileDialog : public FilesystemDialog
{
private:
	std::string data;

public:
	SaveFileDialog(std::string _startFolder, std::string _data);
	std::string run();
};


class LoadFileDialog : public FilesystemDialog
{
private:
	std::string data;

public:
	LoadFileDialog(std::string _startFolder);
	std::string run();
	std::string getData() { return data; }
};

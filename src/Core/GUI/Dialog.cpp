#include "Dialog.hpp"
#include "Utils/Logger.hpp"

//key to everything dialog-related: https://sourceforge.net/projects/tinyfiledialogs/
#include <tinyfiledialogs/tinyfiledialogs.h>

#include <stdio.h>
#include <string.h>

#ifdef _MSC_VER
	#pragma warning(disable : 4996) /* silences warning about strcpy strcat fopen*/
#endif


Dialog::Dialog()
{
	//tinyfd_verbose = argc - 1;
	tinyfd_silent = 1;

#ifdef _WIN32
	tinyfd_winUtf8 = 0; /* on windows, you decide if char holds 0(default): MBCS or 1: UTF-8 */
#endif

	lWillBeGraphicMode = tinyfd_inputBox("tinyfd_query", nullptr, nullptr);
}


void Dialog::init()
{
	strcpy(lBuffer, "v");
	strcat(lBuffer, tinyfd_version);
	//ask whether you want graphic mode
	//if (lWillBeGraphicMode)
	//{
	strcat(lBuffer, "\ngraphic mode: "); //go for graphic mode on default
	//}
	//else
	//{
	//	strcat(lBuffer, "\nconsole mode: ");
	//}
	strcat(lBuffer, tinyfd_response);
	strcat(lBuffer, "\n");
	strcat(lBuffer, tinyfd_needs + 78);
	strcpy(lString, "hello");
	tinyfd_messageBox(lString, lBuffer, "ok", "info", 0);

	tinyfd_notifyPopup("the title", "the message\n\tfrom outer-space", "info");
}


InputDialog::InputDialog(std::string _description)
{
	if (_description.empty())
	{
		LOG(INFO) << "No description passed!\n Better don't enter anything? ;)";
	}
	else
		description = _description;
	title = "Give me some input";
}


std::string InputDialog::run()
{
	userInput = std::string(tinyfd_inputBox(
	    title.c_str(), description.c_str(), nullptr));
	return userInput;
}


ErrorDialog::ErrorDialog(std::string _message)
{
	if (_message.empty())
	{
		LOG(INFO) << "No message passed!";
	}
	else
		message = _message;
	title = "Error!";
}


std::string ErrorDialog::run()
{
	tinyfd_messageBox(
	    title.c_str(),
	    message.c_str(),
	    "ok",
	    "error",
	    1);
	return "error";
}


QuestionDialog::QuestionDialog(std::string _question) :
    Dialog()
{
	if (_question.empty())
	{
		LOG(INFO) << "No question passed!\n Dialog is useless.";
	}
	else
		question = _question;
	title = "Question:";
}


bool QuestionDialog::getBResult()
{
	if (result.compare("Yes") == 0)
		bResult = true;
	else
	{
		if (result.compare("No") == 0)
			bResult = false;
		else
		{
			LOG(INFO) << "Invalid answer.";
		}
	}
	return bResult;
}


std::string QuestionDialog::run()
{
	/*tinyfd_forceConsole = 1;*/
	if (lWillBeGraphicMode && !tinyfd_forceConsole)
	{
		lIntValue = tinyfd_messageBox(title.c_str(),
		                              question.c_str(),
		                              "yesno", "question", 1);
		tinyfd_forceConsole = !lIntValue;

		switch (lIntValue)
		{
			case 0: result = "No"; break;
			case 1: result = "Yes"; break;
		}
		/*lIntValue = tinyfd_messageBox("Hello World",
		"graphic dialogs [yes] / console mode [no]?",
		"yesnocancel", "question", 1);
		tinyfd_forceConsole = (lIntValue == 2);*/
	}
	return result;
}


FilesystemDialog::FilesystemDialog(std::string _startFolder)
{
	if (_startFolder.empty())
	{
		LOG(INFO) << "No start folder passed!\nSetting to c:/.";
		startFolder = "C:/";
	}
	else
	{
		startFolder = _startFolder;
	}
}


//FilesystemDialogs
ChooseFolderDialog::ChooseFolderDialog(std::string _startFolder) :
    FilesystemDialog(_startFolder)
{
	title = "Choose folder..";
}


std::string ChooseFolderDialog::run()
{
	lTheSelectFolderName = tinyfd_selectFolderDialog(
	    "let us just select a directory", nullptr);
	result = std::string(lTheSelectFolderName);
	return result;
}


SaveFileDialog::SaveFileDialog(std::string _startFolder, std::string _data) :
    FilesystemDialog(_startFolder)
{
	data = _data;
	title = "Save file";
}


std::string SaveFileDialog::run()
{
	lTheSaveFileName = tinyfd_saveFileDialog(
	    title.c_str(),
	    startFolder.c_str(),
	    4,
	    lFilterPatterns,
	    nullptr);

	result = std::string(lTheSaveFileName);

	//write actual content
	lIn = fopen(lTheSaveFileName, "w");
	if (!lIn)
	{
		tinyfd_messageBox(
		    "Error",
		    "Can not open this file in write mode",
		    "ok",
		    "error",
		    1);
		return "Error";
	}
	fputs(data.c_str(), lIn);
	fclose(lIn);

	return result;
}


LoadFileDialog::LoadFileDialog(std::string _startFolder) :
    FilesystemDialog(_startFolder)
{
	title = "Load file";
}


std::string LoadFileDialog::run()
{
	lTheOpenFileName = tinyfd_openFileDialog(
	    title.c_str(),
	    "",
	    4,
	    lFilterPatterns,
	    nullptr,
	    0);

	result = std::string(lTheOpenFileName);
	//load actual content
	lIn = fopen(lTheOpenFileName, "r");
	if (!lIn)
	{
		tinyfd_messageBox(
		    "Error",
		    "Can not open this file in write mode",
		    "ok",
		    "error",
		    1);
		return "Error";
	}
	lBuffer[0] = '\0';
	fgets(lBuffer, sizeof(lBuffer), lIn);
	fclose(lIn);

	data = lBuffer;

	return result;
}

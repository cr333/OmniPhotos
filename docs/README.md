# Building Documentation

To build the documentation, you will need to have [installed doxygen](https://www.doxygen.nl/download.html) (making it available on your path).
[The installation instructions](https://www.doxygen.nl/manual/install.html) could be helpful as well.
To check that doxygen is on your path, run this command and make sure it does not return an error:
    
    doxygen --version

Then run the command 
	
	doxygen

or 

    doxygen Doxyfile

in the `docs/` directory.


## Read the Docs

Read the Docs virtual machines read the [sphinx](https://www.sphinx-doc.org/) `conf.py` file and generate the documentation accordingly.
We have injected the call to doxygen into this file.
The doxygen binary is then executed in the docs directory and generates files in the `docs/html/` directory, replacing the generated sphinx html files with those build by doxygen.

The latest generated code documentation is available at https://omniphotos.readthedocs.io/en/latest/.

### Updating the RTD page

In order to set up a new RTD page, you will need to first create a read the docs [account](https://readthedocs.org/accounts/signup/).
Once this is done, go to "My Projects" and "Import a Project".
A list of all your projects should appear here. 
Import the `OmniPhotos` repository you want to generate the documentation for. 
Make sure to have the correct permissions on the repository in order to be able to create a webhook. 

If this is set up correctly, the RTD virtual machine will read the `.readthedocs.yaml` file in the repository where the instructions on how to build the documentaion are located.
In our case, we use the Sphinx API in order to call doxygen and generate files. 
The call to doxygen is located in the `conf.py` file.
Note that, whilst the `index.rst` file is not being used, sphinx will throw an error if it is not present in the directory. 

# Writing Documentation

## Getting started

In order to create a doxygen code stump, one can use the **Doxygen Comments** extension found in the Visual Studio online extension manager.
It is not necessary to use this, but it seems to be the easiest option:

<img src='images/vs_screenshot_1.png'/>

The extension will generate a stub whenever you type `/**` and press enter above a function, variable or class.

The stumps can be customized via Tools->Options and selecting Doxygen in the left-hand pane.

**Other Option:**
Use the Visual Studio Plugin **1-Click Docs**.
You can use the **1-Click Docs** extension to call `doxygen.exe` in Visual Studio.
For more information please refer to https://www.doxygen.nl/manual/starting.html.

There is a demo project (https://github.com/yuanmingze/codestyle_demo) for Doxygen use.


## Doxygen Conventions

### 1. C/C++

All documentation for classes should be located in the header files.

The convention that has been used for the documentation has been the javadoc style:

	/**
	* A brief history of JavaDoc-style (C-style) comments.
	*
	* This is the typical JavaDoc-style C-style comment. It starts with two
	* asterisks.
	*
	* @param theory Even if there is only one possible unified theory. it is just
	* a set of rules and equations.
	*/
	void cstyle(int theory);

The source code and further elaboration on this style can be found here:
https://www.doxygen.nl/manual/docblocks.html

Each class method should have a documentation string one line above it, as shown above.
The description for the function should be succinct and define what purpose of the function, the @param that are being used and why it is there.
If there is a @return value, this should be explained here as well.
The “@” symbol has been used to designate the fields.

Single line comments should be added before class variables as well:

	/** List of supported rendering methods. */
	std::vector<std::string> methods;

Note that for a class to appear as "documented" in the "Class Index" list and "Class Hiearchy" lists in Doxygen, one needs to add a `@brief` descriptor in the line directly before the class declaration (in the header).

	/**
	 * @brief This is a class for ...
	 */
	class MyClass
	{
	}
The `@brief` notation is not strictly necessary here, but might be important to be explicit.


### 2. Python

The convention of coding is based on the [Google Python Style Guide](https://google.github.io/styleguide/pyguide.html).

If you use Visual Studio Code, please refer to the official guide to configure the [automatic linting functionality](https://code.visualstudio.com/docs/python/linting).

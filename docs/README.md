# Documentation

To build the documentation, you'll first of all need to have installed doxygen and make it available on your path.
To check this, run this command and make sure it does not return an error:
    
    doxygen --version

Then simply run the command 
	
	doxygen

or 

    doxygen Doxyfile

in the `docs/` directory.

## Read the docs 

RTD virtual machines read the sphinx `conf.py` file and generate the documentation using this. We've injected the call to doxygen into this file. The doxygen
binary is then executed in the docs directory and generates files in the `docs/html/` directory, replacing the generated sphinx html files with those build by doxygen.

## Doxygen Conventions

Documentation convention (mostly) used can be found [here](https://docs.google.com/document/d/1k36F2nqbyxrLlpo3hOp900BCNME3f3hcAU1IHuv_dSE/edit?usp=sharing)

Note that for a class to appear as "documented" in the "Classe Index" list and "Class Hiearchy" lists in doxygen, you'll need to add a `@brief` descriptor in the line directly before the class declaration (in the hpp). 

	/**
	 * @brief this is a class
	 */
	 class MyClass
	 {

	 }
The `@brief` notation is not strictly necessary, only important for clarity.
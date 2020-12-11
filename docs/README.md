# Documentation

To build the documentation, you'll first of all need to have installed doxygen and make it available on your path.
To check this, run this command and make sure it does not return an error:
    
    doxygen --version

Secondly, you'll need to have installed the sphinx packages in the `requirements.txt` file in this directory.
To do this, generate and activate a Python environment and install the requirements:

    python -m venv sphinxenv
    sphinxenv/Scripts/activate (windows PS)
    source sphinx/bin/activate (linux)
    pip install -r requirements.txt

Then run the CMake GUI from the terminal, with the Python env activated:

    cmake-gui

Set the `WITH_DOCS` flag to true in the CMake GUI, then hit configure + generate.
Build, successively, the doxygen project and then the sphinx project.

**NOTE:** The sphinx project will fail its build if doxygen isn't built first.
If you do not, the Docs section (that uses doxygen) will display a warning of the form:

```
!Warning
oxygenclass: breathe_default_project value ‘OmniPhotos’ does not seem to be a valid key for the breathe_projects dictionary
```

The Doxyfile.in is where you want to make your modifications to the html/xml output from Doxygen.
This will be copied over and filled in by CMake at build time.
To switch this on or off the html generation for doxygen set the `GENERATE_HTML` flag in `Doxyfile.in` and run the configure + generate pipeline again.
There is also a `HIDE_UNDOC_CLASSES` that will reduce the build time if set to "yes".
If you want the standard doxygen HTML files with all the classes, then set this flag to "no" as well as the `GENERATE_HTML` flag to "yes".

To run the Doxygen section of the build on its own, run the following command in 
the `build/docs` directory once you've run CMake:

    doxygen.exe ./Doxyfile

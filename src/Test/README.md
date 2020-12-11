TESTS
=====

Tests are being run by Jenkins on a computer that is connected to the internal University of Bath network.
To add a test, simply add a .cpp file to the directory. CMake will glob this file at the configuration step and build the tests accordingly.


Test Data
---------
The `F:/testDatasets` is the test directory Jenkins can access.

This is updated, via a cron job, every hour from `X:/CompSci/ResearchProjects/CRichardt/TBertel/MegaParallax/testDatasets`.

If there is an error with your unit test concerning data, please contact Reuben Lindroos.

NOTE: Direct access to the X: drive cannot be given to Jenkins, since it is a machine with different permissions from a human user. It is therefore necessary to run this "hack" in order for it to access the data we want. 

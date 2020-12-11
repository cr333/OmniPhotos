import os
import unittest


def find_cmakes(directory):
    """
    finds all occurrences of "CMakeLists.txt"
    in a directory and all its subdirectories and
    returns these as a list of paths (strings)
    :return: list(str)
    """
    cmakes = []
    for file in os.listdir(directory):
        filename = os.path.join(directory, file)

        if file == "CMakeLists.txt":
            cmakes.append(filename)

        if os.path.isdir(filename):
            cmakes.extend(find_cmakes(filename))

    return cmakes


def find_deprecated_funcs(list_of_cmakes, verbose=False):
    """
    :param list_of_cmakes:(str) list of paths to cmakes
    :param verbose: (bool) Prints output to stdout
    :return: (int) number of occurences of deprecated functions
    """
    output = []
    output.append("=================================")
    output.append("Starting diagnosis")
    total_nr_occurences = 0

    for cmakefile in list_of_cmakes:
        printed_header = False
        with open(cmakefile, 'r') as ofile:
            for linenr, line in enumerate(ofile.readlines()):
                line = line.strip()
                for func in deprecated_funcs:
                    if line.startswith(func):
                        if not printed_header:
                            output.append("--------------------------------")
                            output.append("WARNING: Deprecated function found in: ")
                            output.append(cmakefile)
                            printed_header = True
                        output.append(str(linenr) + " <<  " + line)
                        total_nr_occurences += 1

    output.append("total occurences of deprecated funcs was: " + str(total_nr_occurences))
    output.append("=================================")
    output.append("Ending Diagnosis")

    if verbose:
        print('\n'.join(output))

    with open(os.path.join(os.path.dirname(__file__), "cmake_deprecated_functions.log"), "w") as logfile:
        logfile.write('\n'.join(output))

    return total_nr_occurences


class TestNumberOfDeprecatedFuncs(unittest.TestCase):
    def test_max_deprecated_funcs_in_cmake(self):
        max_number = 36  # Tested on 06/12/2019
        self.assertLessEqual(find_deprecated_funcs(all_cmakes, verbose=True), max_number,
                             ' '.join(["new global function was included in the cmake!",
                                       "check:",
                                       os.path.join(os.path.dirname(__file__),
                                                    "cmake_deprecated_functions.log")]))


if __name__ == '__main__':
    project_root_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    src_dir = os.path.join(project_root_dir, "src")

    all_cmakes = find_cmakes(src_dir)
    all_cmakes.append(os.path.join(project_root_dir, "CMakeLists.txt"))

    deprecated_funcs = ["add_compile_options", "include_directories", "link_directories", "link_libraries"]
    find_deprecated_funcs(all_cmakes)

    # NOTE: this runs the unittest and logs the output to an xml file.
    # xml is the type of file jenkins can read for test results. this is
    # also the case for the c++ tests, but the xml API is handled by gtest.
    import xmlrunner
    output = os.path.join(project_root_dir, "build")
    unittest.main(testRunner=xmlrunner.XMLTestRunner(output=output))

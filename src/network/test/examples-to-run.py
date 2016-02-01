#! /usr/bin/env python
## -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

# A list of C++ examples to run in order to ensure that they remain
# buildable and runnable over time.  Each tuple in the list contains
#
#     (example_name, do_run, do_valgrind_run).
#
# See test.py for more information.
cpp_examples = [
    ("main-packet-header", "True", "True"),
    ("main-packet-tag", "True", "True"),
    ("red-tests", "True", "True"),
    ("red_vs_ared --queueType=RED", "True", "True"),
    ("red_vs_ared --queueType=ARED", "True", "True"),
    ("adaptive-red-tests --testNumber=1", "True", "True"),
    ("adaptive-red-tests --testNumber=2", "True", "True"),
    ("adaptive-red-tests --testNumber=6", "True", "True"),
    ("adaptive-red-tests --testNumber=7", "True", "True"),
    ("adaptive-red-tests --testNumber=8", "True", "True"),
    ("adaptive-red-tests --testNumber=9", "True", "True"),
    ("adaptive-red-tests --testNumber=10", "True", "True"),
    ("adaptive-red-tests --testNumber=12", "True", "True"),
    ("adaptive-red-tests --testNumber=13", "True", "True"),
    ("adaptive-red-tests --testNumber=14", "True", "True"),
    ("adaptive-red-tests --testNumber=15", "True", "True"),
]

# A list of Python examples to run in order to ensure that they remain
# runnable over time.  Each tuple in the list contains
#
#     (example_name, do_run).
#
# See test.py for more information.
python_examples = []

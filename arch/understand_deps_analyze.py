#!/usr/bin/python3
#
# Python script to parse dependency export CSV from Understand. It detects
# these common architectural problems:
# * Common code depends on hardware headers
# * HAL unit depends directly on another HAL unit. The dependency should
#   always be via HAL interface.
# * Two units have a circular dependency.
#
# Usage:
#
# understand_deps_analyze.py <deps_export.csv
#
# Notice that the CSV export has to be made by:
# * Create architecture XML with nvgpu_arch.py (command understand)
# * Create nvgpu project in Understand and add nvgpu code, both common and qnx
#   directories
# * Import architecture XML to Understand
# * View Graphs->Dependency graphs->By nvgpu (nvgpu probably comes from the
#   name of the architecture .xml file
# * Create arch dependendency report:
#   * Reports->Dependency->Architecture Dependencies->Export CSV
#   * Choose nvgpu as "Select an architecture to analyze"

import sys
import re

def checkMatch(fromUnit, toUnit):
    ret = None

    if (not fromUnit.startswith("common/hal") and toUnit.startswith("gpu_hw")):
        ret = "Common depends on HW"

    if (fromUnit.startswith("common/hal") and toUnit.startswith("common/hal")):
        ret = "HAL depends on HAL"

    # We have still too many of these problems, and it's not clear if we want
    # to fix them all. Comment out for now.
    # if (fromUnit.startswith("interface") and not toUnit.startswith("interface")):
    #     ret = "Interface depends on non-interface"

    return ret

stream = sys.stdin
deps = set()

depRe = re.compile("(.+),(.+),\d+,\d+,\d+,\d+,\d+")
for line in stream.readlines():
    match = depRe.match(line)
    if match:
        ret = checkMatch(match.group(1), match.group(2))
        if (ret):
            print("%s,%s,%s" % (ret, match.group(1), match.group(2)))

        if "%s,%s" % (match.group(2), match.group(1)) in deps:
            print("Circular dependency,%s,%s" % (match.group(1), match.group(2)))

        deps.add("%s,%s" % (match.group(1), match.group(2)))

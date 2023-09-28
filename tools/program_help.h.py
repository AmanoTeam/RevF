#!/usr/bin/env python3

import argparse
import os
import io

parser = argparse.ArgumentParser(
	prog = "revf",
	description = "Reverse the content of files.",
	add_help = False,
	allow_abbrev = False
)

parser.add_argument(
	"-h",
	"--help",
	required = False,
	action = "store_true",
	help = "Show this help message and exit."
)

parser.add_argument(
	"-v",
	"--version",
	required = False,
	action = "store_true",
	help = "Display the revf version and exit."
)

parser.add_argument(
	"-r",
	"--recursive",
	required = False,
	action = "store_true",
	help = "Recurse down into directories."
)

os.environ["LINES"] = "1000"
os.environ["COLUMNS"] = "1000"

file = io.StringIO()
parser.print_help(file = file)
file.seek(0, io.SEEK_SET)

text = file.read()

header = "/*\nThis file is auto-generated. Use the ../tools/program_help.h.py tool to regenerate.\n*/\n\n#define PROGRAM_HELP \\\n"

for line in text.splitlines():
	header += '\t"%s\\n" \\\n' % line

header += "\n#pragma once\n"

print("Saving to ../src/program_help.h")

with open(file = "../src/program_help.h", mode = "w") as file:
	file.write(header)

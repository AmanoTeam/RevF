# RevF

A tiny CLI tool that reverses the content of files.

## Installation

You can obtain precompiled binaries from the [releases](https://github.com/AmanoTeam/revf/releases) page.

## Building

Clone this repository

```bash
git clone --depth='1' 'https://github.com/AmanoTeam/RevF.git'
```

Configure, build and install:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=MinSizeRel
cmake --build ./build
cmake --install ./build
```

## Usage

Available options:

```
$ revf --help
usage: revf [-h] [-v] [-r]

Reverse the content of files.

options:
  -h, --help       Show this help message and exit.
  -v, --version    Display the revf version and exit.
  -r, --recursive  Recurse down into directories.
```

<img src="./misc/logo.png" width=200 align="right"/>

# cerve

**A multi-worker HTTP server written in C.**

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://github.com/bruttazz/cerve/blob/main/LICENSE)
![unit-test status](https://github.com/bruttazz/cerve/actions/workflows/unit-tests.yml/badge.svg)
[![Release](https://img.shields.io/github/release/bruttazz/cerve.svg?style=flat-square)](https://github.com/bruttazz/cerve/releases/latest)

> Nothing too special, just another personal project of mine!

`cerve` at glance
- portable web-server for unix OSs 
- easy to use minimal CLI
- multi-worker support
- support 62 unique mime-types (some may say it as a gimmick)
- zero third-party dependencies
- highly configurable and extensible (am I repeating my self)
- yet another hackable http-file-server written by a C-lang noob for noobies

## Installation

### 1. Download the latest compiled binary
1. head over to the [release page](https://github.com/bruttazz/cerve/releases/latest) and download the latest compiled binary.
2. make it executable (`chmod +x cerve_v*`)
3. run by executing the binary (`./cerve_v*`)

### 2. Build from source (the nerdy approach)
1. clone the repo
2. make use of the `Makefile` :)
3. (optional) test if everything work fine 
4. (optional) to get a production build execute : `make build`

that's it enjoy the app

## Man 
execute `./cerve --help` for getting the detailed usage info


#### usage: cerve [OPTIONS]

#### Options 
- **-h | --help :**       print help and exit 
- **--version :**       print version and exit 
- **-v | --verbose :**    an integer between 1 and 4, inclusive of those values representing 
                   different log levels. higher values for detailed logs. 1 for minium logging 
                   default to 4 
- **-w | --workers :**    number of workers to be used 
                   defaults to 4 
- **-p | --port :**      system port at which the server to listen for connections. 
                   defaults to 8000 
- **--disable-socket-reuse :**      (flag) if provided the SO_REUSEADDR will not be set. 
- **--res-headers :**   add custom response headers. Defaults to none.
                   expects a newline separated utf-8 text files with colon separated key values.
                   syntax reference: https://httpwg.org/specs/rfc9112.html#rfc.section.2.1 
- **--serve-dir :**     specify custom serve directory.
                   defaults to the current working directory

#### Using a custom response header configuration
cerve accepts custom response headers, which can be specified using the `--res-headers` option. To configure custom headers, create a simple HTTP header file (UTF-8 encoded) without any unnecessary intermediate whitespace, as demonstrated in the example file: `misc/example.http-custom-headerfile.txt`.

> Found a bug?? That you would! Feel free to raise an github issue (or maybe a pull request (only if you got that much time))
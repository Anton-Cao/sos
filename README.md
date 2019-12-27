# SOS (StackOverflow Search)
`sos` is a command line tool for quickly searching error messages on StackOverflow. It parses the output of a given command for error messages, and displays the link to relevant posts on StackOverflow.

## Install
Run
```
make
make install
```
This will install `sos` into `/usr/local/bin`, and create a `.sosrc` file in your home directory.

## Usage
Run `sos <command>` (eg. `sos "python tests/divide_by_zero.py"`) to detect error messages in the output of `<command>`. The `~/.sosrc` file contains the regex patterns that are used to identify error messages. _NOTE:_ each regex pattern must contain exactly one capture group.

Run `sos -o <command>` to automatically open the StackOverflow links in your default browser.

## Example
[![asciicast](https://asciinema.org/a/HbIv8FuwwRS4RPPZ1e7bJyL5I.svg)](https://asciinema.org/a/HbIv8FuwwRS4RPPZ1e7bJyL5I)

## Dev
Run `make` to build binary, `make DEBUG=1` to build with debug statements, `make clean` to remove build files.

## Todo
- [x] Parse `stdout` and `stderr` for error messages
- [x] Display StackOverflow link for error messages
- [x] Open StackOverflow links automatically in browser
- [x] Read regex from `.sosrc` file
- [x] Add custom config file option
- [x] Refactor
- [x] Include tags in search
- [x] Show search query
- [x] Add examples
- [ ] Add tests
- [ ] Distribute somehow
- [ ] Support interactive commands like `python` interpreter (need to unbuffer pseudoterminal)

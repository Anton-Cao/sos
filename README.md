# SOS (StackOverflow Search)
`sos` is a command line tool for quickly searching error messages on StackOverflow.

## Dev
Run `make` to build binary, `make DEBUG=1` to build with debug statements, `make clean` to remove build files.

## Todo
- [x] Parse `stdout` and `stderr` for error messages
- [x] Display StackOverflow link for error messages
- [ ] Open StackOverflow links automatically in browser
- [ ] Read regex from `.sosrc` file
- [ ] Refactor
- [ ] Support interactive commands like `python` interpreter (need to unbuffer pseudoterminal)
- [ ] Add examples
- [ ] Distribute somehow

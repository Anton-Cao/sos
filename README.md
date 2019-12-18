# SOS (StackOverflow Search)
`sos` is a command line tool for quickly searching error messages on StackOverflow.

## Dev
Run `make` to build binary, `make DEBUG=1` to build with debug statements, `make clean` to remove build files.

## Todo
- [ ] Create separate threads to handle `stdin`, `stdout`, and `stderr` of command
- [ ] Parse `stdout` and `stderr` for error messages
- [ ] Display StackOverflow link for error messages
- [ ] Refactor
- [ ] Add examples
- [ ] Distribute somehow
- [ ] Open StackOverflow links automatically in browser

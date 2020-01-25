# fileseqmini
A single-header C++ library for parsing numbered filename sequences

Inspired by https://github.com/justinfx/gofileseq/tree/master/cpp

## Features
* Single-header, no dependencies
* Cross-platform
* Supports
  * Span: `foo/bar.1-3.baz`
  * Step: `foo/bar.1-15x3.baz`
  * Padding: `foo/bar.1-3#.baz`
  * Negative numbers and negative step: `foo/bar.10--5x-2.baz`
  * Comma delimiters: `foo/bar.1-3,7-10.baz`
  * Multiple sequence patterns in a single path: `foo.1-3/bar.22-50#.baz`
  
## Limitations
* No filled patterns (`2-10y3`)
* No staggered patterns (`2-10:3`)
* Fileseqmini cannot create a sequence pattern from a set of filenames
* Fileseqmini has no awareness of a filesystem, it just parses strings

## Usage
You can see the example of library usage in  [misc/fileseq.cpp](misc/fileseq.cpp). Library contains two classes: `FileSequence` and `FileSequenceLazy`, which have almost identical interface:
* `isOk()` - tests if the provided pattern was parsed successfully
* `size()` - returns the total number of paths created from the pattern
* `operator[]` - gets the specific path by index
* `begin()`, `end()` - get const iterators for range-based loops
* `getFullPaths()` - returns all parsed paths
* `getOriginalPattern()` - pretty self explanatory

The difference between the two is that `FileSequenceLazy` does not store the results and will create a string path only when requested, while `FileSequence` holds all the parsed paths as strings in memory all the time. So, you may want to use a 'lazy' approach when dealing with really long sequences.

### Pattern string format
Fileseqmini tries to split the input string by pattern-delimiters. By default, these delimiters are `.`,`/`, and `\`. User may provide its own delimiters set. Each token between two delimiters (or between a delimiter and `\0`) is further split by `,` and the resulted parts are tested against the following format:

`<range start>-<range end>[x<range step>][<padding>]`

Where `<range start>`, `<range end>`, and `<range step>` are integers. Note that, when 'step' is present it has to be negative in case 'end' < 'start'. `<padding>` is described by symbols '@' or '#' (or by the combination of those). '#' stands for 4-pad, while '@' is 1-pad. E.g. `foo.1-2@@` will expand into [`foo.01`, `foo.02`] and `foo.1-2#@` will become [`foo.00001`, `foo.00002`]. User also may provide its own pad-character to use instead of '0'.

> :exclamation: Because fileseqmini does not know anything about the real filesystem, something like `foo.#.ext` is equivalent to `foo.0-9999#.ext`. Beware.

Pattern parts separated by commas are considered the same (sparse) range. All other parts are considered separate ranges and are combined 'by multiplication':
* Pattern `foo.1-3,8-9` will expand into
  * `foo.1`
  * `foo.2`
  * `foo.3`
  * `foo.8`
  * `foo.9`
* Pattern `foo.1-3/8-9` will expand into
  * `foo.1/8`
  * `foo.1/9`
  * `foo.2/8`
  * `foo.2/9`
  * `foo.3/8`
  * `foo.3/9`
* Pattern `foo.1-3.8-9` will expand into
  * `foo.1.8`
  * `foo.1.9`
  * `foo.2.8`
  * `foo.2.9`
  * `foo.3.8`
  * `foo.3.9`
 

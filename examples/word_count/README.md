# Word Count Example

This example demonstrates how to use `libmr` to count the occurrences of each word in one or more text files. It processes input line by line, extracts alphanumeric tokens, converts them to lowercase, and produces a summary of word frequencies.

## Overview

- **Mapper**: Tokenizes each line into alphanumeric words (ignoring case) and emits them.
- **Reducer**: Aggregates the counts for each unique word.
- **Main**: Coordinates the MapReduce job and prints the final results from the binary output file.

## Building the Example

To build the `word_count` executable, run the following command from the root of the project:

```bash
make examples
```

Alternatively, you can build it directly from this directory:

```bash
make
```

*Note: This will also build the `libmr` library in debug mode if it hasn't been built yet.*

## Running the Example

The `word_count` program takes two arguments: an input path (a file or a directory) and an output path.

```bash
./word_count INPUT_PATH OUTPUT_PATH
```

### Example Usage

1. Run the provided script to build and execute the example:
   ```bash
   ./run_example.sh
   ```

2. Alternatively, run it manually:
   ```bash
   ./word_count input output.bin
   ```

3. The program will process all files in the `input/` directory and print the results:
   ```text
   Starting MapReduce on 'input' -> 'output.bin'
   MapReduce finished successfully.

   --- Results ---
   the: 3
   quick: 1
   brown: 1
   ...
   ----------------
   ```

## Input and Output

- **Input Directory**: `examples/word_count/input/` contains sample text files for testing.
- **Output File**: `output.bin` is a binary file containing the aggregated word counts.

## Output Format

The `word_count` program generates a binary output file (`output.bin`) containing the results. The format of this file is:

- `token_length` (size_t)
- `token` (char array, length = token_length)
- `result_size` (size_t)
- `result_data` (opaque bytes, length = result_size)

In this example, the `result_data` is a `size_t` representing the total count for the word.

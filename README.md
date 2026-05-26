# MapReduce Library (libmr)

`libmr` is a lightweight C11 library for MapReduce operations with support for multi-process and multi-threaded execution. It provides a robust framework for processing large datasets by distributing work across a pipeline of specialized processes (Main, Mapper, Reducer) and utilizing multiple worker threads within each process.

## Features

- **Multi-process Pipeline:** Utilizes three distinct processes (Main, Mapper, Reducer) connected via pipes for efficient data flow.
- **Multi-threaded Execution:** Configure the number of mapper and reducer threads within their respective processes to optimize performance.
- **Flexible API:** Define custom mapper and reducer functions to suit your specific data processing needs.
- **Configurable Queues:** Control the internal queue capacity for coordinating between mappers and reducers.
- **Logging Support:** Synchronized logging across processes and threads using POSIX semaphores.
- **Directory Processing:** Automatically process all files within a specified directory.

## Getting Started

### Prerequisites

- A C11-compliant compiler (e.g., GCC or Clang).
- GNU Make.

### Building the Library

To build both debug and release versions:

```bash
make
```

To build only in debug mode:

```bash
make debug
```

To build only in release mode (optimized):

```bash
make release
```

The compiled library (`libmr.a`) will be located in `bin/debug/` or `bin/release/`.

### Running Tests

The project uses the [Unity](https://github.com/ThrowTheSwitch/Unity) test framework. To run the tests:

```bash
make test
```

### Examples

The project includes a Word Count example. To build and run it:

```bash
make examples
echo "hello world hello gemini hello" > input.txt
./examples/word_count/word_count input.txt output.mro
./examples/word_count/read_output output.mro
```

## API Overview

The core of `libmr` revolves around the following types and functions:

- `mr_t`: An opaque handle for a MapReduce instance.
- `mr_attr_t`: Configuration attributes for the framework.
- `mr_create()`: Initializes a new MapReduce instance.
- `mr_start()`: Begins the MapReduce execution.
- `mr_destroy()`: Releases resources associated with an instance.

### Mapper and Reducer Functions

You must provide your own mapper and reducer functions:

```c
typedef int (*mr_mapper_t)(const mr_file_line_t* line, mr_emit_pair_t emit,
                           void* emit_arg, void* user_arg);

typedef int (*mr_reducer_t)(const char* token, const mr_value_t* values,
                            size_t values_count, mr_emit_result_t emit,
                            void* emit_arg, void* user_arg);
```

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details (if applicable).

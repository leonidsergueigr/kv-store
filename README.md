# KV-Store

A lightweight key-value store written from scratch in C to explore memory management, data structures, and systems programming.

> **Status:** Work in progress — currently an in-memory key-value store with an interactive CLI.

## Features

- Key-value operations: `SET`, `GET`, `DELETE`, and `LIST`
- Interactive command-line interface
- Dynamic array storage with automatic capacity growth
- Dynamic memory management
- Basic input validation and error handling
- Modular architecture separating parsing and storage logic

## Getting Started

### Requirements

- C compiler (GCC or Clang)
- GNU Make

### Build

```bash
git clone https://github.com/leonidsergueigr/kv-store.git
cd kv-store
make
```

### Run

```bash
make run
```

### Example

```text
> SET username Leonid Gr
Key-value pair stored successfully.

> GET username
Value: Leonid Gr

> SET language C
Key-value pair stored successfully.

> LIST
username = Leonid Gr
language = C

> DELETE username
Key deleted successfully.

> EXIT
Exiting...
```

## Project Structure

```text
kv-store/
├── include/      # Public interfaces
├── src/          # Implementation and CLI
├── tests/        # Tests
├── Makefile      # Build configuration
└── .gitignore
```

## Roadmap

- [x] Basic key-value operations
- [x] Interactive CLI
- [x] Dynamic array-based storage
- [x] Dynamic memory allocation
- [ ] Comprehensive unit tests and memory checks
- [ ] Hash table implementation
- [ ] File persistence
- [ ] Thread safety and concurrency
- [ ] Performance benchmarks
- [ ] Optional TCP server interface

## Limitations

- Data is stored only in memory and is lost when the program exits.
- Lookup currently uses a linear search.
- Concurrent access is not yet supported.

## Learning Goals

This project is a hands-on exploration of C and systems programming, including pointers, memory ownership, data structures, modular design, file I/O, and concurrency.

The goal is to progressively build a more reliable and efficient storage engine while understanding the mechanisms behind each feature.

## Author

[Léonid Gr](https://github.com/leonidsergueigr)

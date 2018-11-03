
# Concurrent Level Hashing 

Concurrent level hashing supports multi-reader and multi-writer concurrency via simply using fine-grained locking.  
The code for concurrent level hashing is run in DRAM platform.

## How to run

1.  Do `make` to generate an executable file `clevel`
2.  Run `clevel` with the number of threads, e.g., `./clevel 4`
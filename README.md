
# Level Hashing

Level hashing is a write-optimized and high-performance hashing index scheme for persistent memory, with low-overhead consistency guarantee and cost-efficient resizing. Level hashing provides a sharing-based two-level hash table, which achieves a constant-scale search/insertion/deletion/update time complexity in the worst case and rarely incurs extra NVM writes. To guarantee the consistency with low overhead, level hashing leverages log-free consistency schemes for insertion, deletion, and resizing operations, and an opportunistic log-free scheme for update operation. To cost-efficiently resize this hash table, level hashing leverages an in-place resizing scheme that only needs to rehash 1/3 of buckets instead of the entire table, thus significantly reducing the number of rehashed buckets and improving the resizing performance.   
To learn more, please read our OSDI paper:

* Pengfei Zuo, Yu Hua, Jie Wu, “[Write-Optimized and High-Performance Hashing Index Scheme for Persistent Memory](https://www.usenix.org/conference/osdi18/presentation/zuo)”, in Proceedings of the 13th USENIX Symposium on Operating Systems Design and Implementation (OSDI), 2018.

## Directory Description

* **level_hashing:** The code for single-threaded level hashing, run in DRAM platform.
* **concurrent_level_hashing:** The code for concurrent level hashing, run in DRAM platform.
* **persistent_level_hashing:** The code for persistent level hashing, run in the simulated NVM platform, i.e., [Quartz](https://github.com/HewlettPackard/quartz).

## Contact

If you have any questions about level hashing, please feel free to contact me.   
Email: pfzuo@hust.edu.cn   
Homepage: https://pfzuo.github.io/  
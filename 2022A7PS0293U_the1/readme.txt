Take Home Assignment 1
Suryavir Kapur
2022A7PS0293U

Important Note:
Used a custom barrier implementation (from GitHub) because I couldn't use POSIX barrier on my PC (MacOS).
Will also work on Linux.
================================================================

Part A: Performance Testing Across Dimensions
--------------------------------------------
1. Used nested loops to iterate through all combinations:
   - Outer loop: iterates through matrix dimensions
   - Inner loop: iterates through thread counts

2. GFLOPS calculation:
   - Each matrix-vector multiplication requires 2 x m x n operations
   - GFLOPS = (ops_count / time_taken) / 10^9
   This gives us performance metrics for each configuration.


Part B: False Sharing Prevention
-------------------------------
1. New Padded Struct:
   struct aligned_double {
       double value;
       char padding[CACHE_LINE - sizeof(double)];
   };

2. What I did:
   - Defined CACHE_LINE as 64 bytes (common cache line size)
   - Each value is padded to fill a complete cache line
   - This ensures values used by different threads don't share cache lines

3. Implementation in Pth_mat_vect_no_false_sharing():
   - Uses y_aligned array instead of y
   - Each thread writes to cache-aligned locations


Part C: Barrier Synchronization
-----------------------------
Used a custom implementation (from GitHub) because I couldn't use POSIX barrier on my PC (MacOS).

1. Implementation in Pth_mat_vect_with_barrier():
   - Calls my_barrier_wait() after each row computation
   - All threads synchronize before moving to next row
   - Ensures coordinated progress through matrix

Analysis:
--------
- Base version: Standard parallel implementation
- No false sharing: Showed improved performance due to reduced
  cache coherence traffic
- Barrier version: Showed reduced performance due to synchronization
  overhead, but ensures strict ordering of operations

Output:
-------
dimensions: 8000000x8    threads: 1     gflops/sec: 0.458377
dimensions: 8000000x8    threads: 2     gflops/sec: 0.996405
dimensions: 8000000x8    threads: 4     gflops/sec: 1.846192
dimensions: 8000000x8    threads: 8     gflops/sec: 2.732001
dimensions: 8000x8000    threads: 1     gflops/sec: 0.526143
dimensions: 8000x8000    threads: 2     gflops/sec: 1.011266
dimensions: 8000x8000    threads: 4     gflops/sec: 1.571517
dimensions: 8000x8000    threads: 8     gflops/sec: 3.062566
dimensions: 8x8000000    threads: 1     gflops/sec: 0.408908
dimensions: 8x8000000    threads: 2     gflops/sec: 0.983276
dimensions: 8x8000000    threads: 4     gflops/sec: 1.781488
dimensions: 8x8000000    threads: 8     gflops/sec: 2.758858 
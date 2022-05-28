# C++ Buddy allocator

Benchmark results:

- Allocated `262144` 4kib large pages (1GiB worth of memory)
- Results lie between `3.8 - 4.1` seconds on my `Intel Core i5-4210U`, `12GB` ram.
- Benchmark results may vary based on your hardware and OSes availability.
- NOTE: Disable CPU frequency scaling for accurate benchmark results
- (I had it enabled since I don't want to reboot + multiple tasks are running in the background, so keep that in mind)

<br>

## Benchmark image:
<img src="benchmark.png">

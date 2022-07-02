# C++ Buddy allocator

Benchmark results:

- Allocated `262144` 4kib large pages (i.e. 1GiB worth of memory in 4kib chunks)
- Blazing fast allocations: The benchmark completed in under [0.1 seconds](benchmark.png) on my `Intel Core i5-4210U`, `12GB` ram running on Ubuntu Linux 20.04 LTS.
- Benchmark results may vary based on your hardware and OSes availability.
- NOTE: Disable CPU frequency scaling for accurate benchmark results
- (I had it disabled since I don't want to reboot + multiple tasks are running in the background, so keep that in mind)

<br>

## Benchmark image:
<img src="benchmark.png">

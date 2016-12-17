# Sum reduction

Implementation of multithreaded sum reduction, to serve as an example on the Advanced Computer Architectures course I lecture. Prints the sum to stderr and timing measurements to stout.

Runs 10 times for each number of threads between 1 and 128 that is either a multiple of 10 or a power of 2.

Example use:

```
mkdir csv
make
./sumreduction > csv/results.csv
```
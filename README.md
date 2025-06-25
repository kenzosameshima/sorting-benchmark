# sorting-benchmark
This project is a Supervised Practical Assignment (SPA) for the Data Structures course at UNIP. It implements a program that takes input data previously established by the instructor, stores the numbers in an array, and then copies and sorts them using multiple sorting algorithms in order to compare their efficiency. The main implementation is in C, with a small portion in Python used for selecting the input file and invoking the main.exe.

# Running the Code
To run the program, begin by compiling the source code. The following instructions assume you are using GCC on a Windows system (e.g., via MinGW or WSL).

Use the command below to compile the project:

```bash
gcc -I./include -o main main.c utils/utils.c sort/quick.c sort/selection.c sort/insertion.c sort/cocktail.c sort/heap.c sort/merge.c benchmark/benchmark.c
```
This will generate an executable file named main.


After compilation, run the Python script to execute the program:

```bash
python main.py
```

The script will prompt you to select an input file, which contains the numerical data that will be used to generate arrays for sorting. You will then be able to choose from a set of options to execute and benchmark the various sorting algorithms.

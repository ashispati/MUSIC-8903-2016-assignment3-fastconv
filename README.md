# Fast Convolution Library

This repository consists of the implementation of Fast Convolution done for assignment 3 of the class MUSI-8903

This repository contains the Fast Convolution library implemented for the MUSI8903 Audio Software Engineering assignment 3 and a demo program that uses this library to convolve an audio file with an impulse response (IR) file, writes the convolution output to a text file, and report the performance of the convolution operation. It also contains the test suite for the library called FastConv_Test, which implements the tests mentioned in the assignment problem statement.

The readme file contains information for the usage of the executable program.

#Usage Instructions:

The file MUSI8903Exec.cpp contains the source code for a command line program that takes as input the audio file path, the modulation frequency and the modulation amplitude.

The command line application has to be run as follow:

   \<MUSI8903Exec binary> \<Input file path> \<Impulse Response file path> \<FFT block size>
   
 - \<FFT block size>: The desired block size for the FFT-based convolution. Default set to 8192.

One notable detail is that the sampling rate of the input file and the IR should be the same for the convolution to be correct, so one mmust be careful while using this. A possible solution would be to implement a resampler.

The program outputs a text file, containing the convolved input and IR.

The file Test_FastConv.cpp in the TestExec build target contains all the tests mentioned in the assignment.

To run the tests:
   \<TestExec binary> FastConv_Test

Note that the above targets may also be run from within an IDE like Visual Studio or XCode.

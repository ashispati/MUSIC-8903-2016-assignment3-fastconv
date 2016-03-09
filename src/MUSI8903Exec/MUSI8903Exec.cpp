
#include <iostream>
#include <string>
#include <ctime>

#include "MUSI8903Config.h"

#include "AudioFileIf.h"
#include "FastConv.h"

using namespace std;

// local function declarations
void    showClInfo();

/////////////////////////////////////////////////////////////////////////////////
// main function
int main(int argc, char* argv[])
{
	std::string             sInputFilePath,                 //!< file paths
                            sIRFilePath,
                            sOutputFilePath;

	static const int        kBlockSize = 1024;

	clock_t                 time = 0;

	float                   **ppfInputAudioData = 0,
		**ppfIRData = 0,
		**ppfOutputAudioData = 0;

	CAudioFileIf            *phAudioFile = 0;
	std::fstream            hOutputFile;
	CAudioFileIf::FileSpec_t stFileSpec;

	long long				fftBlockSize = 8192,
		iLengthIR = 0,
		iLengthInput = 0;

	CFastConv				*pFastConv = 0;
	double					dLengthSeconds = 0;

	showClInfo();

	//////////////////////////////////////////////////////////////////////////////
	// parse command line arguments
	try
	{
		switch (argc)
		{
		case 1: cout << "Too few arguments. Enter Audio file path and IR file path." << endl;
			exit(0);
			break;
		case 2: cout << "Too few arguments. Enter IR file path also." << endl;
			exit(0);
		case 3: sInputFilePath = argv[1];
			sIRFilePath = argv[2];
			break;
		case 4: sInputFilePath = argv[1];
			sIRFilePath = argv[2];
			fftBlockSize = stof(argv[3]);
			break;
		default: cout << "Too many parameters. Check what you're entering." << endl;
			exit(0);
		}
	}
	catch (exception &exc)
	{
		cerr << "Invalid arguments passed. Please use the correct formatting for running the program." << endl;
	}

	//////////////////////////////////////////////////////////////////////////////
	// Read the input IR file
	CAudioFileIf::create(phAudioFile);
	phAudioFile->openFile(sIRFilePath, CAudioFileIf::kFileRead);
	if (!phAudioFile->isOpen())
	{
		cout << "IR file open error!";
		return -1;
	}
	phAudioFile->getFileSpec(stFileSpec);
	ppfIRData = new float*[1];
	phAudioFile->getLength(dLengthSeconds);
	iLengthIR = stFileSpec.fSampleRateInHz * dLengthSeconds;
	ppfIRData[0] = new float[iLengthIR];
	phAudioFile->readData(ppfIRData, iLengthIR);

	//////////////////////////////////////////////////////////////////////////////
	// Initialize Fast Convolution object

	CFastConv::create(pFastConv);
	pFastConv->init(ppfIRData[0],iLengthIR, fftBlockSize);
	phAudioFile->reset();

	//////////////////////////////////////////////////////////////////////////////
	// open the output text file
	/*
    hOutputFile.open(sOutputFilePath.c_str(), std::ios::out);
	if (!hOutputFile.is_open())
	{
		cout << "Text file open error!";
		return -1;
	}
    */
    
	//////////////////////////////////////////////////////////////////////////////
	// get audio data and process it
	phAudioFile->openFile(sInputFilePath, CAudioFileIf::kFileRead);
	if (!phAudioFile->isOpen())
	{
		cout << "IR file open error!";
		return -1;
	}
	phAudioFile->getFileSpec(stFileSpec);
	phAudioFile->getLength(dLengthSeconds);
	iLengthInput = dLengthSeconds * stFileSpec.fSampleRateInHz;

	ppfOutputAudioData = new float*[1];
	ppfOutputAudioData[0] = new float[kBlockSize];
    

	//////////////////////////////////////////////////////////////////////////////
	// allocate memory for input file
	ppfInputAudioData = new float*[1];
	ppfInputAudioData[0] = new float[kBlockSize];

	time = clock();

	int blockcounter = -1;
	while (!phAudioFile->isEof())
	{
		blockcounter++;
		long long iNumFrames = kBlockSize;
		phAudioFile->readData(ppfInputAudioData, iNumFrames);
		pFastConv->process(ppfInputAudioData[0], ppfOutputAudioData[0], iNumFrames);

        
		// Handle two separate convolution function calls with same block?? Or store input data in buffer and then process?
		/*for (int i = 0; i < iNumFrames; i++)
		{
			for (int c = 0; c < stFileSpec.iNumChannels; c++)
			{
				hOutputFile << ppfInputAudioData[c][i] << "\t";
			}
			hOutputFile << endl;
		}*/
	}
    int sizeOfTail = 0;
    pFastConv->getSizeOfTail(sizeOfTail);
    float **ppfReverbTail = new float*[1];
    ppfReverbTail[0] = new float[sizeOfTail];
    pFastConv->flushBuffer(ppfReverbTail[0]);
    

	cout << "Frequency Domain Convolution: \t" << (clock() - time)*1.F / CLOCKS_PER_SEC << " seconds." << endl;

	//////////////////////////////////////////////////////////////////////////////
	// clean-up
	CAudioFileIf::destroy(phAudioFile);
	//hOutputFile.close();

    delete[] ppfInputAudioData[0];
    delete[] ppfOutputAudioData[0];
    delete[] ppfReverbTail[0];
	delete[] ppfInputAudioData;
    delete[] ppfOutputAudioData;
    delete[] ppfReverbTail;
	ppfInputAudioData = 0;
    ppfOutputAudioData = 0;
    ppfReverbTail = 0;

	return 0;

}


void     showClInfo()
{
	cout << "GTCMT MUSI8903" << endl;
	cout << "(c) 2016 by Alexander Lerch" << endl;
	cout << endl;

	return;
}


#include "MUSI8903Config.h"

#ifdef WITH_TESTS
#include <cassert>
#include <cstdio>
#include <iostream>
#include <iomanip>

#include "UnitTest++.h"
#include "Vector.h"

#include "FastConv.h"

using namespace std;

SUITE(FastConv)
{
    struct FastConvData
    {
        FastConvData():
            m_pCFastConv(0),
            inputData(0),
            outputData(0),
            inputTmp(0),
            outputTmp(0),
            dataLength(8435),
            blockLength(13),
            lengthOfIr(11021),
            impulseResponse(0),
            convBlockLength(4096)
        {
            CFastConv::create(m_pCFastConv);
            
            inputData = new float [dataLength];
            outputData = new float [dataLength + lengthOfIr - 1];
            impulseResponse = new float [lengthOfIr];
            
            for (int i = 0; i < lengthOfIr; i++) {
                impulseResponse[i] = static_cast<float>(rand())/RAND_MAX;
                //impulseResponse[i] = i;
            }
            
            CVectorFloat::setZero(inputData, dataLength);
            CVectorFloat::setZero(outputData, dataLength);
            
        }

        ~FastConvData()
        {
            delete[] inputData;
            delete[] outputData;
            //delete[] inputTmp;
            //delete[] outputTmp;
            delete[] impulseResponse;
            CFastConv::destroy(m_pCFastConv);
        }

        void TestProcess()
        {
            int numFramesRemaining = dataLength;
            while (numFramesRemaining > 0)
            {
                int numFrames = std::min(numFramesRemaining, blockLength);
                inputTmp = &inputData[dataLength - numFramesRemaining];
                outputTmp = &outputData[dataLength - numFramesRemaining];
                m_pCFastConv->process(inputTmp, outputTmp, numFrames);
                numFramesRemaining -= numFrames;
            }
            int sizeOfTail = 0;
            m_pCFastConv->getSizeOfTail(sizeOfTail);
            CHECK_EQUAL(lengthOfIr-1, sizeOfTail);
            outputTmp = &outputData[dataLength];
            m_pCFastConv->flushBuffer(outputTmp);
        }
        
        void resetIOData()
        {
            for (int i = 0; i < dataLength; i++)
            {
                inputData[i] = 0;
            }
            
            for (int i = 0; i < dataLength + lengthOfIr - 1; i++)
            {
                outputData[i] = 0;
            }
        }
        
        CFastConv *m_pCFastConv;
        float* inputData;
        float* outputData;
        float* inputTmp;
        float* outputTmp;
        int dataLength;
        int blockLength;
        int lengthOfIr;
        float* impulseResponse;
        int convBlockLength;
    };

    
    TEST_FIXTURE(FastConvData, IrTest)
    {
        // initialise fast conv
        m_pCFastConv->init(impulseResponse, lengthOfIr, convBlockLength);
        
        // set input data
        for (int i = 0; i < dataLength; i++)
        {
            inputData[i] = 0;
        }
        int delay = 5;
        inputData[delay] = 1;
        
        TestProcess();
		
        for (int i = 0; i < dataLength + lengthOfIr - 1; i++)
        {
            if (i < delay)
            {
                CHECK_CLOSE(0, outputData[i], 1e-3F);
            }
            else if (i <= delay + lengthOfIr - 1)
            {
                CHECK_CLOSE(impulseResponse[i-delay], outputData[i], 1e-3F);
            }
            else if (i > delay + lengthOfIr - 1)
            {
                CHECK_CLOSE(0, outputData[i], 1e-3F);
            }
        }
        
        resetIOData();
    }
    
    TEST_FIXTURE(FastConvData, InputBlockLengthTest)
    {
        int blockSizes[] = {1, 13, 1023, 2048,1,17, 5000, 1897};
        int numBlockSizes = 8;
        
        for (int i = 0; i < numBlockSizes; i++)
        {
            blockLength = blockSizes[i];
            
            // initialise fast conv
            m_pCFastConv->init(impulseResponse, lengthOfIr);
            
            // set input data
            for (int i = 0; i < dataLength; i++)
            {
                inputData[i] = 0;
            }
            int delay = 5;
            inputData[delay] = 1;

            
            TestProcess();
           
            
            // check output
            for (int i = 0; i < dataLength + lengthOfIr - 1; i++)
            {
                if (i < delay)
                {
                    CHECK_CLOSE(0, outputData[i], 1e-3F);
                }
                else if (i <= delay + lengthOfIr - 1)
                {
                    CHECK_CLOSE(impulseResponse[i-delay], outputData[i], 1e-3F);
                }
                else if (i > delay + lengthOfIr - 1)
                {
                    CHECK_CLOSE(0, outputData[i], 1e-3F);
                }
            }
            
            resetIOData();
            
        }
    }
    
}

#endif //WITH_TESTS

#include <iostream>
#include <iomanip>

#include "Vector.h"
#include "Util.h"

#include "FastConv.h"
#include "Fft.h"

using namespace std;

CFastConv::CFastConv( void ):
    _is_initialized(false),
    _length_of_ir(0),
    _block_length(0),
    _impulse_response(0),
    _reverb_tail(0)
{

    reset();
}

CFastConv::~CFastConv( void ) {}

Error_t CFastConv::create(CFastConv*& pCFastConv)
{
    pCFastConv = new CFastConv();
    
    if (!pCFastConv)
        return kUnknownError;
    
    return kNoError;
}

Error_t CFastConv::destroy (CFastConv*& pCFastConv)
{
    if (!pCFastConv)
        return kUnknownError;
    
    pCFastConv->reset ();
    
    delete pCFastConv;
    pCFastConv = 0;
    
    return kNoError;
}

Error_t CFastConv::init(float *pfImpulseResponse, int iLengthOfIr, int iBlockLength /*= 8192*/)
{
    reset();
    
    _length_of_ir = iLengthOfIr;
    
    //checks if the the input block length is a power of 2, otherwise selects the next power of 2
    if (CUtil::isPowOf2(iBlockLength)) {
        _block_length = iBlockLength;
    }
    else {
        _block_length = CUtil::nextPowOf2(iBlockLength);
    }
    
    //set impulse response
    _impulse_response = new float[_length_of_ir];
    for (int i = 0; i < _length_of_ir; i++)
    {
        _impulse_response[i] = pfImpulseResponse[i];
    }
    
    _reverb_tail = new CRingBuffer<float>(_length_of_ir - 1);
    _is_initialized = true;
    return kNoError;
}

Error_t CFastConv::reset()
{
    delete[] _impulse_response;
    delete _reverb_tail;
    
    _length_of_ir = 0;
    _block_length = 0;
    _impulse_response = 0;
    _reverb_tail = 0;
    _is_initialized = false ;
    
    return kNoError;
}

Error_t CFastConv::process (float *pfInputBuffer, float *pfOutputBuffer, int iLengthOfBuffers )
{
    if (!_is_initialized) {
        return kNotInitializedError;
    }
	//processTimeDomainBlockedIR(pfInputBuffer, pfOutputBuffer, iLengthOfBuffers);
    //processTimeDomain(pfInputBuffer, pfOutputBuffer, iLengthOfBuffers);
    processFreqDomain(pfInputBuffer, pfOutputBuffer, iLengthOfBuffers);
    return kNoError;
}

Error_t CFastConv::flushBuffer(float*& pfReverbTail)
{
    if (!_is_initialized) {
        return kNotInitializedError;
    }
    
    for (int i = 0; i < _length_of_ir - 1; i++) {
        pfReverbTail[i] = _reverb_tail->get(i);
    }
    _reverb_tail->reset();
    return kNoError;
}

Error_t CFastConv::getSizeOfTail(int& iSizeOfTail) {
    if (!_is_initialized) {
        return kNotInitializedError;
    }
    iSizeOfTail = _length_of_ir - 1;
    return kNoError;
}

Error_t CFastConv::processTimeDomain(float *pfInputBuffer, float *pfOutputBuffer, int iLengthOfBuffers)
{
    //add back previous reverb tail
    for (int i = 0; i < std::min(_length_of_ir - 1, iLengthOfBuffers); i++)
    {
        pfOutputBuffer[i] = _reverb_tail->getPostInc();
        _reverb_tail->putPostInc(0);
    }
    
    //compute convolution for the process buffer length
    for (int i = 0; i < iLengthOfBuffers; i++)
    {
        for (int j = 0; j <= i; j++)
        {
            if (j < _length_of_ir)
            {
                pfOutputBuffer[i] += pfInputBuffer[i-j] * _impulse_response[j];
                
            }
            
        }
    }
    
    //update flush buffer
    for (int i = 0; i < _length_of_ir-1; i++)
    {
        float value = _reverb_tail->getPostInc();
        for (int j = 0; j < _length_of_ir - 1 - i; j++)
        {
            if (j <= iLengthOfBuffers - 1)
            {
                value += pfInputBuffer[iLengthOfBuffers - 1 - j] * _impulse_response[i + j + 1];
            }
        }
        _reverb_tail->putPostInc(value);
    }
    return kNoError;
}

Error_t CFastConv::processTimeDomainBlockedIR(float* pfInputBuffer, float* pfOutputBuffer, int iLengthOfBuffers)
{
    //Calculate number of blocks into which the IR and input buffer is to be divided
	int num_ir_blocks = ceil(_length_of_ir / static_cast<float>(_block_length));
	int num_input_blocks = ceil(iLengthOfBuffers / static_cast<float>(_block_length));

	//Temp buffer for storing intermediate convolutions
	float* temp_buffer = new float[2 * _block_length - 1];

	//Temp reverb tail
	float* temp_reverb = new float[iLengthOfBuffers + _length_of_ir - 1];
	memset(temp_reverb, 0, sizeof(float)*(iLengthOfBuffers + _length_of_ir - 1));

	//Add back previous reverb tail
	for (int i = 0; i < std::min(_length_of_ir - 1, iLengthOfBuffers); i++)
	{
		pfOutputBuffer[i] = _reverb_tail->getPostInc();
		_reverb_tail->putPostInc(0);
	}
	
	//Process all input blocks
	for (int i_input = 0; i_input < num_input_blocks; i_input++)
	{
		//Process all IR blocks
		for (int i = 0; i < num_ir_blocks; i++)
		{
			memset(temp_buffer, 0, sizeof(float)*(2 * _block_length - 1));
			for (int j = 0; j < 2 * _block_length - 1; j++)
			{
				for (int k = 0; k <= j; k++)
				{
					if (k < _block_length && (j - k) < _block_length)
					{
						//check for last block and if index exceeds the length of the IR
						float ir_val = (k + i*_block_length >= _length_of_ir) ? 0 : _impulse_response[k + i*_block_length];
						
						//check for last block and if index exceeds length of input buffer
						float input_val = (j - k + i_input*_block_length >= iLengthOfBuffers) ? 0 : pfInputBuffer[j - k + i_input*_block_length];
                
						temp_buffer[j] += input_val * ir_val;
					}
				}
			}
			//add from temp_buffer to outputbuffer and reverb_tail
			for (int j = 0, k = std::max(0, _block_length*i - iLengthOfBuffers); j < 2 * _block_length - 1 && k < _length_of_ir - 1; j++)
			{
				if (j + i*_block_length < iLengthOfBuffers)
					pfOutputBuffer[j + i*_block_length] += temp_buffer[j];
				else
				{
					temp_reverb[k] += temp_buffer[j];
					k++;
				}
			}
		}
	}
	//add temporary reverb buffer to member
	for (int i = 0; i < _length_of_ir - 1; i++)
	{
		float value = _reverb_tail->getPostInc();
		value += temp_reverb[i];
		_reverb_tail->putPostInc(value);
	}
	
	delete[] temp_buffer;
	delete[] temp_reverb;
	return kNoError;
}

Error_t CFastConv::processFreqDomain (float *pfInputBuffer, float *pfOutputBuffer, int iLengthOfBuffers) {
    
    //Calculate number of blocks into which the IR and input buffer is to be divided
    int num_ir_blocks = ceil(_length_of_ir / static_cast<float>(_block_length));
    int num_input_blocks = ceil(iLengthOfBuffers / static_cast<float>(_block_length));
    
    //Temp buffer for storing intermediate convolutions
    float* temp_buffer = new float[2 * _block_length];
    
    //Temp reverb tail
    float* temp_reverb = new float[iLengthOfBuffers + _length_of_ir - 1];
    memset(temp_reverb, 0, sizeof(float)*(iLengthOfBuffers + _length_of_ir - 1));
    
    //Zero pad impulse response and input buffer
    float* input_buffer = new float [num_input_blocks * _block_length];
    float* ir_buffer = new float [num_ir_blocks * _block_length];
    for (int i = 0; i < num_input_blocks * _block_length; i++) {
        if (i < iLengthOfBuffers) {
            input_buffer[i] = pfInputBuffer[i] * 2 * _block_length;
        }
        else {
            input_buffer[i] = 0;
        }
    }
    for (int i = 0; i < num_ir_blocks * _block_length; i++) {
        if (i < _length_of_ir) {
            ir_buffer[i] = _impulse_response[i];
        }
        else {
            ir_buffer[i] = 0;
        }
    }
    
    //Add back previous reverb tail to output
    for (int i = 0; i < std::min(_length_of_ir - 1, iLengthOfBuffers); i++)
    {
        pfOutputBuffer[i] = _reverb_tail->getPostInc();
        _reverb_tail->putPostInc(0);
    }
    
    //Process all input blocks
    for (int i_input = 0; i_input < num_input_blocks; i_input++)
    {
        //Process all IR blocks
        for (int i_ir = 0; i_ir < num_ir_blocks; i_ir++)
        {
            //Initialize temp variables
            memset(temp_buffer, 0, sizeof(float)*(2 * _block_length));
            CFft::complex_t* input_block_fft = new CFft::complex_t[2*_block_length];
            CFft::complex_t* ir_block_fft = new CFft::complex_t[2*_block_length];
            CFft* fft;
            CFft::create(fft);
            fft->init(_block_length,2,CFft::WindowFunction_t::kWindowNone, CFft::Windowing_t::kNoWindow);
           
            
            //Perform FFT based convolution
            float* input_block_tmp = &input_buffer[i_input * _block_length];
            fft->doFft(input_block_fft, input_block_tmp);
            float* ir_block_tmp = &ir_buffer[i_ir * _block_length];
            fft->doFft(ir_block_fft, ir_block_tmp);
            
            fft->mulCompSpectrum(input_block_fft, ir_block_fft);
            fft->doInvFft(temp_buffer, input_block_fft);

            //Free memory
            CFft::destroy(fft);
            
            //Add from temp_buffer to outputbuffer and reverb_tail
            for (int j = 0, k = std::max(0, _block_length*i_ir - iLengthOfBuffers); j < 2 * _block_length - 1 && k < _length_of_ir - 1; j++)
            {
                if (j + i_ir*_block_length < iLengthOfBuffers)
                    pfOutputBuffer[j + i_ir*_block_length] += temp_buffer[j];
                else
                {
                    temp_reverb[k] += temp_buffer[j];
                    k++;
                }
            }
        
            //Free memory
            delete[] input_block_fft;
            delete[] ir_block_fft;
        }
    }
    //Add temporary reverb tail buffer to member
    for (int i = 0; i < _length_of_ir - 1; i++)
    {
        float value = _reverb_tail->getPostInc();
        value += temp_reverb[i];
        _reverb_tail->putPostInc(value);
    }

    //Free memory
    delete[] temp_buffer;
    delete[] temp_reverb;
    delete[] input_buffer;
    delete[] ir_buffer;
    return kNoError;
}

#include <iostream>

#include "Vector.h"
#include "Util.h"

#include "FastConv.h"

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
    
    _block_length = iBlockLength;
    _length_of_ir = iLengthOfIr;
    
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
    delete[] _reverb_tail;
    
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
	processTimeDomainBlockedIR(pfInputBuffer, pfOutputBuffer, iLengthOfBuffers);
    //processTimeDomain(pfInputBuffer, pfOutputBuffer, iLengthOfBuffers);
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
        if (i == 50) {
            std::cout << pfOutputBuffer[i] << std::endl;
        }
        
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
	//Need to add function that zero pads the IR if it isn't an exact multiple of the block_length...or handle it some other way
	//Current implementation assumes multiple of block_size

	//Zero padding, won't really change the IR internally. Will only process the last block separately.
	int pad_input = iLengthOfBuffers % _block_length;
	int pad_ir = _length_of_ir % _block_length;

	int num_ir_blocks = ceil(_length_of_ir / _block_length);
	int num_input_blocks = ceil(iLengthOfBuffers / _block_length);

	//Temp buffer for storing intermediate convolutions
	float* temp_buffer = new float[iLengthOfBuffers + _block_length - 1];
	//float* temp_buffer = new float[2 * _block_length - 1];

	//Temp reverb tail
	float* temp_reverb = new float[iLengthOfBuffers + _length_of_ir - 1];
	memset(temp_reverb, 0, sizeof(float)*(iLengthOfBuffers + _length_of_ir - 1));

	//add back previous reverb tail
	for (int i = 0; i < std::min(_length_of_ir - 1, iLengthOfBuffers); i++)
	{
		pfOutputBuffer[i] = _reverb_tail->getPostInc();
		_reverb_tail->putPostInc(0);
	}
	
	//Process all IR blocks
	for (int i = 0; i < num_ir_blocks; i++)
	{
		memset(temp_buffer, 0, sizeof(float)*(iLengthOfBuffers + _block_length - 1));
		for (int j = 0; j < iLengthOfBuffers + _block_length -1; j++)
		{
			for (int k = 0; k <= j ; k++)
			{
				if(k<_block_length && (j-k) < iLengthOfBuffers)
					temp_buffer[j] += pfInputBuffer[j - k] * _impulse_response[k + i*_block_length];
			}
		}


		//add from temp_buffer to outputbuffer
		for (int j = 0; j < std::min(iLengthOfBuffers, iLengthOfBuffers - i*_block_length); j++)
		{
			pfOutputBuffer[j + i*_block_length] += temp_buffer[j];
		}
		
		//add the rest to the reverb_tail
		//Temp reverb tail improves time complexity, worsens space complexity though
		for (int j = std::max(0, iLengthOfBuffers - i*_block_length), k = std::max(0, i*_block_length - iLengthOfBuffers); (j < iLengthOfBuffers + _block_length - 1) && (k < _length_of_ir - 1); j++, k++)
		{
			temp_reverb[k] += temp_buffer[j];
			//cout << temp_reverb[k] << "\t" << temp_buffer[j] << endl;
		}
	}

	/*for (int i = 0; i < iLengthOfBuffers; i++)
	{
		cout << pfOutputBuffer[i] << "\t";
	}
	cout << endl;*/
	//add temporary reverb buffer to member
	for (int i = 0; i < _length_of_ir - 1; i++)
	{
		float value = _reverb_tail->getPostInc();
		value += temp_reverb[i];
		_reverb_tail->putPostInc(value);
		//cout << value << endl;
	}

	
	delete[] temp_buffer;
	delete[] temp_reverb;
	return kNoError;
}
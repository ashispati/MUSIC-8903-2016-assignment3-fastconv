
#include <iostream>

#include "Vector.h"
#include "Util.h"

#include "FastConv.h"

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
    delete _impulse_response;
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
    processTimeDomain(pfInputBuffer, pfOutputBuffer, iLengthOfBuffers);
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
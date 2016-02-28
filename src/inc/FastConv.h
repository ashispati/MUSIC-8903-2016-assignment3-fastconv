
#if !defined(__FastConv_HEADER_INCLUDED__)
#define __FastConv_HEADER_INCLUDED__

#pragma once

#include "ErrorDef.h"
#include "RingBuffer.h"


/*! \brief interface for fast convolution
*/
class CFastConv
{
public:

    CFastConv(void);
    virtual ~CFastConv(void);

    /*! creates a new fast convolution instance
     \param pCFastConv pointer to the new class
     \return Error_t
     */
    static Error_t create(CFastConv*& pCFastConv);
    
    /*! destroys a fast convolution instance
     \param pCFastConv pointer to the class to be destroyed
     \return Error_t
     */
    static Error_t destroy (CFastConv*& pCFastConv);
    
    /*! initializes the class with the impulse response and the block length
    \param pfImpulseResponse impulse response samples (mono only)
    \param iLengthOfIr length of impulse response
    \param iBlockLength processing block size
    \return Error_t
    */
    Error_t init (float *pfImpulseResponse, int iLengthOfIr, int iBlockLength = 8192);
    
    /*! resets all internal class members
    \return Error_t
    */
    Error_t reset ();

    /*! computes cost and path w/o back-tracking
    \param pfInputBuffer (mono)
    \param pfOutputBuffer (mono)
    \param iLengthOfBuffers can be anything from 1 sample to 10000000 samples
    \return Error_t
    */
    Error_t process (float *pfInputBuffer, float *pfOutputBuffer, int iLengthOfBuffers );
    
    /*! returns the reverb tail at the end, this should be called once after the last process call
    \param pfReverbTail (mono) should be a null pointer
     \return Error_t
    */
    Error_t flushBuffer(float*& pfReverbTail);
    
    /*! returns the size of the reverb tail
     \param iSizeOfTail
     \return Error_t
     */
    Error_t getSizeOfTail(int& iSizeOfTail);
    
 
private:
    bool _is_initialized;
    int _length_of_ir;
    int _block_length;
    float* _impulse_response;
    CRingBuffer<float>* _reverb_tail;
    
    /*! computes time domain convolution 
     \param pfInputBuffer (mono)
     \param pfOutputBuffer (mono)
     \param iLengthOfBuffers can be anything from 1 sample to 10000000 samples
     \return Error_t
     */
    Error_t processTimeDomain (float *pfInputBuffer, float *pfOutputBuffer, int iLengthOfBuffers);
};


#endif

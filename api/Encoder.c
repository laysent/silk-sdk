/***********************************************************************
Copyright (c) 2006-2012, Skype Limited. All rights reserved. 
Redistribution and use in source and binary forms, with or without 
modification, (subject to the limitations in the disclaimer below) 
are permitted provided that the following conditions are met:
- Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright 
notice, this list of conditions and the following disclaimer in the 
documentation and/or other materials provided with the distribution.
- Neither the name of Skype Limited, nor the names of specific 
contributors, may be used to endorse or promote products derived from 
this software without specific prior written permission.
NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED 
BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND 
CONTRIBUTORS ''AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF 
USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***********************************************************************/


/*****************************/
/* Silk encoder test program */
/*****************************/

#ifdef _WIN32
#define _CRT_SECURE_NO_DEPRECATE    1
#endif
#include <node_api.h>
#include <assert.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SKP_Silk_SDK_API.h"

/* Define codec specific settings */
#define ENCODER_MAX_BYTES_PER_FRAME     250 // Equals peak bitrate of 100 kbps 
#define ENCODER_MAX_INPUT_FRAMES        5
#define ENCODER_FRAME_LENGTH_MS         20
#define ENCODER_MAX_API_FS_KHZ          48

#ifdef _SYSTEM_IS_BIG_ENDIAN
/* Function to convert a little endian int16 to a */
/* big endian int16 or vica verca                 */
void encoder_swap_endian(
    SKP_int16       vec[],              /*  I/O array of */
    SKP_int         len                 /*  I   length      */
)
{
    SKP_int i;
    SKP_int16 tmp;
    SKP_uint8 *p1, *p2;

    for( i = 0; i < len; i++ ){
        tmp = vec[ i ];
        p1 = (SKP_uint8 *)&vec[ i ]; p2 = (SKP_uint8 *)&tmp;
        p1[ 0 ] = p2[ 1 ]; p1[ 1 ] = p2[ 0 ];
    }
}
#endif

#if (defined(_WIN32) || defined(_WINCE)) 
#include <windows.h>	/* timer */
#else    // Linux or Mac
#include <sys/time.h>
#endif

#ifdef _WIN32

unsigned long EncoderGetHighResolutionTime() /* O: time in usec*/
{
    /* Returns a time counter in microsec	*/
    /* the resolution is platform dependent */
    /* but is typically 1.62 us resolution  */
    LARGE_INTEGER lpPerformanceCount;
    LARGE_INTEGER lpFrequency;
    QueryPerformanceCounter(&lpPerformanceCount);
    QueryPerformanceFrequency(&lpFrequency);
    return (unsigned long)((1000000*(lpPerformanceCount.QuadPart)) / lpFrequency.QuadPart);
}
#else    // Linux or Mac
unsigned long EncoderGetHighResolutionTime() /* O: time in usec*/
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    return((tv.tv_sec*1000000)+(tv.tv_usec));
}
#endif // _WIN32

#define WRITE_ENCODE_OUTPUT(input, length) \
    output_temp_pointer = output; \
    output = malloc(size + (length)); \
    if (output_temp_pointer != NULL) { \
        memcpy(output, output_temp_pointer, size); \
        free(output_temp_pointer); \
    } \
    memcpy(&output[size], (input), (length)); \
    size += (length);

napi_value Encode(
    napi_env env,
    void* _inStream,
    size_t total,
    SKP_int32 API_fs_Hz,
    SKP_int32 max_internal_fs_Hz,
    SKP_int32 packetSize_ms,
    SKP_int32 targetRate_bps,
    SKP_int32 packetLoss_perc,
    SKP_int32 complexity_mode,
    SKP_int32 INBandFEC_enabled,
    SKP_int32 DTX_enabled,
    SKP_int32 quiet,
    SKP_int32 tencent,
    SKP_int32 tencent_amr
) {
    unsigned long tottime, starttime;
    double    filetime;
    size_t    counter, index = 0, size = 0;
    char *output = NULL, *output_temp_pointer, *inStream;
    SKP_int32 k, totPackets, totActPackets, ret;
    SKP_int16 nBytes;
    double    sumBytes, sumActBytes, avg_rate, act_rate, nrg;
    SKP_uint8 payload[ ENCODER_MAX_BYTES_PER_FRAME * ENCODER_MAX_INPUT_FRAMES ];
    SKP_int16 in[ ENCODER_FRAME_LENGTH_MS * ENCODER_MAX_API_FS_KHZ * ENCODER_MAX_INPUT_FRAMES ];
    SKP_int32 encSizeBytes;
    void      *psEnc;
    char      error_message[250];
#ifdef _SYSTEM_IS_BIG_ENDIAN
    SKP_int16 nBytes_LE;
#endif

    inStream = (char*)_inStream;

    /* default settings */
    SKP_int32 smplsSinceLastPacket;
    SKP_int32 frameSizeReadFromFile_ms = 20;

    SKP_SILK_SDK_EncControlStruct encControl; // Struct for input to encoder
    SKP_SILK_SDK_EncControlStruct encStatus;  // Struct for status of encoder

    /* If no max internal is specified, set to minimum of API fs and 24 kHz */
    if( max_internal_fs_Hz == 0 ) {
        max_internal_fs_Hz = 24000;
        if( API_fs_Hz < max_internal_fs_Hz ) {
            max_internal_fs_Hz = API_fs_Hz;
        }
    }

    /* Print options */
    if( !quiet ) {
        printf("********** Silk Encoder (Fixed Point) v %s ********************\n", SKP_Silk_SDK_get_version());
        printf("********** Compiled for %d bit cpu ******************************* \n", (int)sizeof(void*) * 8 );
        printf( "Input Stream Length:            %zu\n",     total );
        printf( "API sampling rate:              %d Hz\n",  API_fs_Hz );
        printf( "Maximum internal sampling rate: %d Hz\n",  max_internal_fs_Hz );
        printf( "Packet interval:                %d ms\n",  packetSize_ms );
        printf( "Inband FEC used:                %d\n",     INBandFEC_enabled );
        printf( "DTX used:                       %d\n",     DTX_enabled );
        printf( "Complexity:                     %d\n",     complexity_mode );
        printf( "Target bitrate:                 %d bps\n", targetRate_bps );
    }

    /* Add Silk header to stream */
    {
        static const char Tencent_amr_header[] = "#!AMR\n";
        static const char Tencent_header[] = "\x02";
        static const char Silk_header[] = "#!SILK_V3";

        if (tencent_amr) {
            WRITE_ENCODE_OUTPUT(Tencent_amr_header, sizeof(char) * strlen(Tencent_amr_header));
        }

        if (tencent) {
            WRITE_ENCODE_OUTPUT(Tencent_header, sizeof(char) * strlen(Tencent_header));
        }

        WRITE_ENCODE_OUTPUT(Silk_header, sizeof(char) * strlen(Silk_header));
    }

    /* Create Encoder */
    ret = SKP_Silk_SDK_Get_Encoder_Size( &encSizeBytes );
    if( ret ) {
        sprintf(error_message, "\nError: SKP_Silk_create_encoder returned %d\n", ret);
        napi_throw_error(env, NULL, error_message);
        return NULL;
    }

    psEnc = malloc( encSizeBytes );

    /* Reset Encoder */
    ret = SKP_Silk_SDK_InitEncoder( psEnc, &encStatus );
    if( ret ) {
        sprintf(error_message, "\nError: SKP_Silk_reset_encoder returned %d\n", ret);
        napi_throw_error(env, NULL, error_message);
        return NULL;
    }

    /* Set Encoder parameters */
    encControl.API_sampleRate        = API_fs_Hz;
    encControl.maxInternalSampleRate = max_internal_fs_Hz;
    encControl.packetSize            = ( packetSize_ms * API_fs_Hz ) / 1000;
    encControl.packetLossPercentage  = packetLoss_perc;
    encControl.useInBandFEC          = INBandFEC_enabled;
    encControl.useDTX                = DTX_enabled;
    encControl.complexity            = complexity_mode;
    encControl.bitRate               = ( targetRate_bps > 0 ? targetRate_bps : 0 );

    if( API_fs_Hz > ENCODER_MAX_API_FS_KHZ * 1000 || API_fs_Hz < 0 ) {
        sprintf(error_message, "\nError: API sampling rate = %d out of range, valid range 8000 - 48000 \n \n", API_fs_Hz);
        napi_throw_error(env, NULL, error_message);
        return NULL;
    }

    tottime              = 0;
    totPackets           = 0;
    totActPackets        = 0;
    smplsSinceLastPacket = 0;
    sumBytes             = 0.0;
    sumActBytes          = 0.0;
    smplsSinceLastPacket = 0;
    
    while( 1 ) {
        /* Read input from file */
        counter = ( frameSizeReadFromFile_ms * API_fs_Hz ) / 1000;
        if ((index + counter * sizeof(SKP_int16)) > total) break;
        memcpy(in, &inStream[index], counter * sizeof(SKP_int16));
        index += counter * sizeof(SKP_int16);
#ifdef _SYSTEM_IS_BIG_ENDIAN
        encoder_swap_endian( in, counter );
#endif

        /* max payload size */
        nBytes = ENCODER_MAX_BYTES_PER_FRAME * ENCODER_MAX_INPUT_FRAMES;

        starttime = EncoderGetHighResolutionTime();

        /* Silk Encoder */
        ret = SKP_Silk_SDK_Encode( psEnc, &encControl, in, (SKP_int16)counter, payload, &nBytes );
        if( ret && quiet == 0 ) {
            printf( "\nSKP_Silk_Encode returned %d", ret );
        }

        tottime += EncoderGetHighResolutionTime() - starttime;

        /* Get packet size */
        packetSize_ms = ( SKP_int )( ( 1000 * ( SKP_int32 )encControl.packetSize ) / encControl.API_sampleRate );

        smplsSinceLastPacket += ( SKP_int )counter;
        
        if( ( ( 1000 * smplsSinceLastPacket ) / API_fs_Hz ) == packetSize_ms ) {
            /* Sends a dummy zero size packet in case of DTX period  */
            /* to make it work with the decoder test program.        */
            /* In practice should be handled by RTP sequence numbers */
            totPackets++;
            sumBytes  += nBytes;
            nrg = 0.0;
            for( k = 0; k < ( SKP_int )counter; k++ ) {
                nrg += in[ k ] * (double)in[ k ];
            }
            if( ( nrg / ( SKP_int )counter ) > 1e3 ) {
                sumActBytes += nBytes;
                totActPackets++;
            }

            /* Write payload size */
#ifdef _SYSTEM_IS_BIG_ENDIAN
            nBytes_LE = nBytes;
            encoder_swap_endian( &nBytes_LE, 1 );
            WRITE_ENCODE_OUTPUT(&nBytes_LE, sizeof(SKP_int16) * 1);
#else
            WRITE_ENCODE_OUTPUT(&nBytes, sizeof(SKP_int16) * 1);
#endif

            /* Write payload */
            WRITE_ENCODE_OUTPUT(payload, sizeof(SKP_uint8) * nBytes);

            smplsSinceLastPacket = 0;
        
            if( !quiet ) {
                fprintf( stderr, "\rPackets encoded:                %d", totPackets );
            }
        }
    }

    /* Write dummy because it can not end with 0 bytes */
    nBytes = -1;

    /* Write payload size */
    if (!tencent) {
        WRITE_ENCODE_OUTPUT(&nBytes, sizeof(SKP_int16) * 1);
    }

    /* Free Encoder */
    free( psEnc );

    filetime  = totPackets * 1e-3 * packetSize_ms;
    avg_rate  = 8.0 / packetSize_ms * sumBytes       / totPackets;
    act_rate  = 8.0 / packetSize_ms * sumActBytes    / totActPackets;
    if( !quiet ) {
        printf( "\nFile length:                    %.3f s", filetime );
        printf( "\nTime for encoding:              %.3f s (%.3f%% of realtime)", 1e-6 * tottime, 1e-4 * tottime / filetime );
        printf( "\nAverage bitrate:                %.3f kbps", avg_rate  );
        printf( "\nActive bitrate:                 %.3f kbps", act_rate  );
        printf( "\n\n" );
    }

    napi_status status;
    napi_value result;
    status = napi_create_buffer_copy(env, size, output, NULL, &result);
    assert(status == napi_ok);
    return result;
}

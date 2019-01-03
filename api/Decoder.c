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
/* Silk decoder test program */
/*****************************/

#ifdef _WIN32
#define _CRT_SECURE_NO_DEPRECATE    1
#endif
#include <node_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SKP_Silk_API.h"
#include "SKP_Silk_SDK_API.h"
#include "SKP_Silk_SigProc_FIX.h"

/* Define codec specific settings should be moved to h file */
#define DECODER_MAX_BYTES_PER_FRAME     1024
#define DECODER_MAX_INPUT_FRAMES        5
#define DECODER_MAX_FRAME_LENGTH        480
#define DECODER_FRAME_LENGTH_MS         20
#define DECODER_MAX_API_FS_KHZ          48
#define DECODER_MAX_LBRR_DELAY          2

#ifdef _SYSTEM_IS_BIG_ENDIAN
/* Function to convert a little endian int16 to a */
/* big endian int16 or vica verca                 */
void decoder_swap_endian(
    SKP_int16       vec[],
    SKP_int         len
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

unsigned long DecoderGetHighResolutionTime() /* O: time in usec*/
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
unsigned long DecoderGetHighResolutionTime() /* O: time in usec*/
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    return((tv.tv_sec*1000000)+(tv.tv_usec));
}
#endif // _WIN32

/* Seed for the random number generator, which is used for simulating packet loss */
static SKP_int32 rand_seed = 1;

void Decode(napi_env env, char* bitInFileName, char* speechOutFileName, SKP_int32 quiet, SKP_float loss_prob, SKP_int32 API_Fs_Hz)
{
    unsigned long tottime, starttime;
    double    filetime;
    size_t    counter;
    SKP_int32 totPackets, i, k;
    SKP_int16 ret, len, tot_len;
    SKP_int16 nBytes;
    SKP_uint8 payload[    DECODER_MAX_BYTES_PER_FRAME * DECODER_MAX_INPUT_FRAMES * ( DECODER_MAX_LBRR_DELAY + 1 ) ];
    SKP_uint8 *payloadEnd = NULL, *payloadToDec = NULL;
    SKP_uint8 FECpayload[ DECODER_MAX_BYTES_PER_FRAME * DECODER_MAX_INPUT_FRAMES ], *payloadPtr;
    SKP_int16 nBytesFEC;
    SKP_int16 nBytesPerPacket[ DECODER_MAX_LBRR_DELAY + 1 ], totBytes;
    SKP_int16 out[ ( ( DECODER_FRAME_LENGTH_MS * DECODER_MAX_API_FS_KHZ ) << 1 ) * DECODER_MAX_INPUT_FRAMES ], *outPtr;
    FILE      *bitInFile, *speechOutFile;
    SKP_int32 packetSize_ms=0;
    SKP_int32 decSizeBytes;
    char      error_message[250];
    void      *psDec;
    SKP_int32 frames, lost;
    SKP_SILK_SDK_DecControlStruct DecControl;

    if( !quiet ) {
        printf("********** Silk Decoder (Fixed Point) v %s ********************\n", SKP_Silk_SDK_get_version());
        printf("********** Compiled for %d bit cpu *******************************\n", (int)sizeof(void*) * 8 );
        printf( "Input:                    %s\n", bitInFileName );
        printf( "Output:                   %s\n", speechOutFileName );
    }

    /* Open files */
    bitInFile = fopen( bitInFileName, "rb" );
    if( bitInFile == NULL ) {
        sprintf(error_message, "Error: could not open input file %s\n", bitInFileName);
        napi_throw_error(env, NULL, error_message);
        return;
    } 

    /* Check Silk header */
    {
        char header_buf[50];
        char magic_number[50];
        int maybe_silk = 0;

        /* Tencent variant of Silk codec, check if first byte is 0x02 */
        fread(header_buf, sizeof(char), 1, bitInFile);

        /* Silk header is one of the following: #!SILK_V3 or .#!SILK_V3 or #!AMR\n.#!SILK_V3, where . = 0x02 */
        if (header_buf[0] == 0x02) {

            strcpy(magic_number, "#!SILK_V3");
            maybe_silk = 1;

        } else if (header_buf[0] == '#') {
            
            fread(header_buf, sizeof(char), 2, bitInFile);
            header_buf[2] = '\0'; /* Terminate null character */

            if (strcmp(header_buf, "!A") == 0) {
                strcpy(magic_number, "MR\n\x02#!SILK_V3");
                maybe_silk = 1;
            } else if (strcmp(header_buf, "!S") == 0) {
                strcpy(magic_number, "ILK_V3");
                maybe_silk = 1;
            }
        }

        if (maybe_silk != 1) {
            napi_throw_error(env, NULL, "Error: Wrong Header\n");
            return;
        }

        counter = fread(header_buf, sizeof(char), strlen(magic_number), bitInFile);
        header_buf[strlen(magic_number)] = '\0'; /* Terminate with a null character */
        if (strcmp(header_buf, magic_number) != 0) { 
            /* Non-equal strings */
            sprintf(error_message, "Error: Wrong Header %s\n", header_buf);
            napi_throw_error(env, NULL, error_message);
            return;
        }
    }

    speechOutFile = fopen( speechOutFileName, "wb" );
    if( speechOutFile == NULL ) {
        sprintf(error_message, "Error: could not open output file %s\n", speechOutFileName);
        napi_throw_error(env, NULL, error_message);
        return;
    }

    /* Set the samplingrate that is requested for the output */
    if( API_Fs_Hz == 0 ) {
        DecControl.API_sampleRate = 24000;
    } else {
        DecControl.API_sampleRate = API_Fs_Hz;
    }

    /* Initialize to one frame per packet, for proper concealment before first packet arrives */
    DecControl.framesPerPacket = 1;

    /* Create decoder */
    ret = SKP_Silk_SDK_Get_Decoder_Size( &decSizeBytes );
    if( ret ) {
        printf( "\nSKP_Silk_SDK_Get_Decoder_Size returned %d", ret );
    }
    psDec = malloc( decSizeBytes );

    /* Reset decoder */
    ret = SKP_Silk_SDK_InitDecoder( psDec );
    if( ret ) {
        printf( "\nSKP_Silk_InitDecoder returned %d", ret );
    }

    totPackets = 0;
    tottime    = 0;
    payloadEnd = payload;

    /* Simulate the jitter buffer holding MAX_FEC_DELAY packets */
    for( i = 0; i < DECODER_MAX_LBRR_DELAY; i++ ) {
        /* Read payload size */
        counter = fread( &nBytes, sizeof( SKP_int16 ), 1, bitInFile );
#ifdef _SYSTEM_IS_BIG_ENDIAN
        decoder_swap_endian( &nBytes, 1 );
#endif
        /* Read payload */
        counter = fread( payloadEnd, sizeof( SKP_uint8 ), nBytes, bitInFile );

        if( ( SKP_int16 )counter < nBytes ) {
            break;
        }
        nBytesPerPacket[ i ] = nBytes;
        payloadEnd          += nBytes;
        totPackets++;
    }

    while( 1 ) {
        /* Read payload size */
        counter = fread( &nBytes, sizeof( SKP_int16 ), 1, bitInFile );
#ifdef _SYSTEM_IS_BIG_ENDIAN
        decoder_swap_endian( &nBytes, 1 );
#endif
        if( nBytes < 0 || counter < 1 ) {
            break;
        }
        
        /* Read payload */
        counter = fread( payloadEnd, sizeof( SKP_uint8 ), nBytes, bitInFile );
        if( ( SKP_int16 )counter < nBytes ) {
            break;
        }

        /* Simulate losses */
        rand_seed = SKP_RAND( rand_seed );
        if( ( ( ( float )( ( rand_seed >> 16 ) + ( 1 << 15 ) ) ) / 65535.0f >= ( loss_prob / 100.0f ) ) && ( counter > 0 ) ) {
            nBytesPerPacket[ DECODER_MAX_LBRR_DELAY ] = nBytes;
            payloadEnd                       += nBytes;
        } else {
            nBytesPerPacket[ DECODER_MAX_LBRR_DELAY ] = 0;
        }

        if( nBytesPerPacket[ 0 ] == 0 ) {
            /* Indicate lost packet */
            lost = 1;

            /* Packet loss. Search after FEC in next packets. Should be done in the jitter buffer */
            payloadPtr = payload;
            for( i = 0; i < DECODER_MAX_LBRR_DELAY; i++ ) {
                if( nBytesPerPacket[ i + 1 ] > 0 ) {
                    starttime = DecoderGetHighResolutionTime();
                    SKP_Silk_SDK_search_for_LBRR( payloadPtr, nBytesPerPacket[ i + 1 ], ( i + 1 ), FECpayload, &nBytesFEC );
                    tottime += DecoderGetHighResolutionTime() - starttime;
                    if( nBytesFEC > 0 ) {
                        payloadToDec = FECpayload;
                        nBytes = nBytesFEC;
                        lost = 0;
                        break;
                    }
                }
                payloadPtr += nBytesPerPacket[ i + 1 ];
            }
        } else {
            lost = 0;
            nBytes = nBytesPerPacket[ 0 ];
            payloadToDec = payload;
        }

        /* Silk decoder */
        outPtr = out;
        tot_len = 0;
        starttime = DecoderGetHighResolutionTime();

        if( lost == 0 ) {
            /* No Loss: Decode all frames in the packet */
            frames = 0;
            do {
                /* Decode 20 ms */
                ret = SKP_Silk_SDK_Decode( psDec, &DecControl, 0, payloadToDec, nBytes, outPtr, &len );
                if( ret ) {
                    printf( "\nSKP_Silk_SDK_Decode returned %d", ret );
                }

                frames++;
                outPtr  += len;
                tot_len += len;
                if( frames > DECODER_MAX_INPUT_FRAMES ) {
                    /* Hack for corrupt stream that could generate too many frames */
                    outPtr  = out;
                    tot_len = 0;
                    frames  = 0;
                }
                /* Until last 20 ms frame of packet has been decoded */
            } while( DecControl.moreInternalDecoderFrames ); 
        } else {    
            /* Loss: Decode enough frames to cover one packet duration */
            for( i = 0; i < DecControl.framesPerPacket; i++ ) {
                /* Generate 20 ms */
                ret = SKP_Silk_SDK_Decode( psDec, &DecControl, 1, payloadToDec, nBytes, outPtr, &len );
                if( ret ) {
                    printf( "\nSKP_Silk_Decode returned %d", ret );
                }
                outPtr  += len;
                tot_len += len;
            }
        }

        packetSize_ms = tot_len / ( DecControl.API_sampleRate / 1000 );
        tottime += DecoderGetHighResolutionTime() - starttime;
        totPackets++;

        /* Write output to file */
#ifdef _SYSTEM_IS_BIG_ENDIAN   
        decoder_swap_endian( out, tot_len );
#endif
        fwrite( out, sizeof( SKP_int16 ), tot_len, speechOutFile );

        /* Update buffer */
        totBytes = 0;
        for( i = 0; i < DECODER_MAX_LBRR_DELAY; i++ ) {
            totBytes += nBytesPerPacket[ i + 1 ];
        }
        SKP_memmove( payload, &payload[ nBytesPerPacket[ 0 ] ], totBytes * sizeof( SKP_uint8 ) );
        payloadEnd -= nBytesPerPacket[ 0 ];
        SKP_memmove( nBytesPerPacket, &nBytesPerPacket[ 1 ], DECODER_MAX_LBRR_DELAY * sizeof( SKP_int16 ) );

        if( !quiet ) {
            fprintf( stderr, "\rPackets decoded:          %d", totPackets );
        }
    }

    /* Empty the recieve buffer */
    for( k = 0; k < DECODER_MAX_LBRR_DELAY; k++ ) {
        if( nBytesPerPacket[ 0 ] == 0 ) {
            /* Indicate lost packet */
            lost = 1;

            /* Packet loss. Search after FEC in next packets. Should be done in the jitter buffer */
            payloadPtr = payload;
            for( i = 0; i < DECODER_MAX_LBRR_DELAY; i++ ) {
                if( nBytesPerPacket[ i + 1 ] > 0 ) {
                    starttime = DecoderGetHighResolutionTime();
                    SKP_Silk_SDK_search_for_LBRR( payloadPtr, nBytesPerPacket[ i + 1 ], ( i + 1 ), FECpayload, &nBytesFEC );
                    tottime += DecoderGetHighResolutionTime() - starttime;
                    if( nBytesFEC > 0 ) {
                        payloadToDec = FECpayload;
                        nBytes = nBytesFEC;
                        lost = 0;
                        break;
                    }
                }
                payloadPtr += nBytesPerPacket[ i + 1 ];
            }
        } else {
            lost = 0;
            nBytes = nBytesPerPacket[ 0 ];
            payloadToDec = payload;
        }

        /* Silk decoder */
        outPtr  = out;
        tot_len = 0;
        starttime = DecoderGetHighResolutionTime();

        if( lost == 0 ) {
            /* No loss: Decode all frames in the packet */
            frames = 0;
            do {
                /* Decode 20 ms */
                ret = SKP_Silk_SDK_Decode( psDec, &DecControl, 0, payloadToDec, nBytes, outPtr, &len );
                if( ret ) {
                    printf( "\nSKP_Silk_SDK_Decode returned %d", ret );
                }

                frames++;
                outPtr  += len;
                tot_len += len;
                if( frames > DECODER_MAX_INPUT_FRAMES ) {
                    /* Hack for corrupt stream that could generate too many frames */
                    outPtr  = out;
                    tot_len = 0;
                    frames  = 0;
                }
            /* Until last 20 ms frame of packet has been decoded */
            } while( DecControl.moreInternalDecoderFrames );
        } else {    
            /* Loss: Decode enough frames to cover one packet duration */

            /* Generate 20 ms */
            for( i = 0; i < DecControl.framesPerPacket; i++ ) {
                ret = SKP_Silk_SDK_Decode( psDec, &DecControl, 1, payloadToDec, nBytes, outPtr, &len );
                if( ret ) {
                    printf( "\nSKP_Silk_Decode returned %d", ret );
                }
                outPtr  += len;
                tot_len += len;
            }
        }

        packetSize_ms = tot_len / ( DecControl.API_sampleRate / 1000 );
        tottime += DecoderGetHighResolutionTime() - starttime;
        totPackets++;

        /* Write output to file */
#ifdef _SYSTEM_IS_BIG_ENDIAN   
        decoder_swap_endian( out, tot_len );
#endif
        fwrite( out, sizeof( SKP_int16 ), tot_len, speechOutFile );

        /* Update Buffer */
        totBytes = 0;
        for( i = 0; i < DECODER_MAX_LBRR_DELAY; i++ ) {
            totBytes += nBytesPerPacket[ i + 1 ];
        }
        SKP_memmove( payload, &payload[ nBytesPerPacket[ 0 ] ], totBytes * sizeof( SKP_uint8 ) );
        payloadEnd -= nBytesPerPacket[ 0 ];
        SKP_memmove( nBytesPerPacket, &nBytesPerPacket[ 1 ], DECODER_MAX_LBRR_DELAY * sizeof( SKP_int16 ) );

        if( !quiet ) {
            fprintf( stderr, "\rPackets decoded:          %d", totPackets );
        }
    }

    if( !quiet ) {
        printf( "\nDecoding Finished \n" );
    }

    /* Free decoder */
    free( psDec );

    /* Close files */
    fclose( speechOutFile );
    fclose( bitInFile );

    filetime = totPackets * 1e-3 * packetSize_ms;
    if( !quiet ) {
        printf("\nFile length:              %.3f s", filetime);
        printf("\nTime for decoding:        %.3f s (%.3f%% of realtime)", 1e-6 * tottime, 1e-4 * tottime / filetime);
        printf("\n\n");
    }
    return;
}

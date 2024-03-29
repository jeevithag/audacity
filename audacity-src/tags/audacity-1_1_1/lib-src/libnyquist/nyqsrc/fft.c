/* fft.c -- implement snd_fft */

#include <math.h>
#include <stdio.h>
#ifndef mips
#include "stdlib.h"
#endif
#include "xlisp.h"
#include "sound.h"
#include "falloc.h"
#include "fft.h"
#include "fftn.h"

/* NOTE: this code does not properly handle start times that do not
 * correspond to the time of the first actual sample
 */


/* snd_fetch_array -- fetch a lisp array of samples */
/*
 * storage layout: the extra field points to extra state that we'll use
 * extra[0] -> length of extra storage
 * extra[1] -> CNT (number of samples in current block)
 * extra[2] -> INDEX (current sample index in current block)
 * extra[3] -> FILLCNT (how many samples in buffer)
 * extra[4] -> TERMCNT (how many samples until termination)
 * extra[5 .. 5+len-1] -> samples (stored as floats)
 * extra[5+len .. 5+3*len-1] -> real and imag. arrays for fft
 * extra[5+3*len ... 5+4*len-1] -> window coefficients
 * 
 * Termination details:
 *   Return NIL when the sound terminates.
 *   Termination is defined as the point where all original
 * signal samples have been shifted out of the samples buffer
 * so that all that's left are zeros from beyond the termination
 * point.
 *   Implementation: when termination is discovered, set TERMCNT
 * to the number of samples to be shifted out. TERMCNT is initially
 * -1 as a flag that we haven't seen the termination yet. 
 * Each time samples are shifted, decrement TERMCNT by the shift amount.
 * When TERMCNT goes to zero, return NULL.
 */

#define CNT extra[1]
#define INDEX extra[2]
#define FILLCNT extra[3]
#define TERMCNT extra[4]
#define OFFSET 5
#define SAMPLES list->block->samples

/* DEBUGGING PRINT FUNCTION:
    void printfloats(char *caption, float *data, int len)
    {
        int i;
        printf("%s: ", caption);
        for (i = 0; i < len; i++) {
            printf("%d:%g ", i, data[i]);
        }
        printf("\n");
    }
*/

void n_samples_from_sound(sound_type s, long n, float *table)
{
    long blocklen;
    sample_type scale_factor = s->scale;
    s = sound_copy(s);
    while (n > 0) {
        sample_block_type sampblock = sound_get_next(s, &blocklen);
        long togo = min(blocklen, n);
        long i;
        sample_block_values_type sbufp = sampblock->samples;
        for (i = 0; i < togo; i++) {
            *table++ = (float) (*sbufp++ * scale_factor);
        }
        n -= togo;
    }
    sound_unref(s);
}


LVAL snd_fft(sound_type s, long len, long step, LVAL winval)
{
    long i, maxlen, skip, fillptr;
    float *samples;
    float *temp_fft;
    float *window;
    LVAL result;
    
    if (len < 1) xlfail("len < 1");

    if (!s->extra) { /* this is the first call, so fix up s */
        sound_type w = NULL;
        if (winval) {
            if (soundp(winval)) {
                w = getsound(winval);
            } else {
                xlerror("expected a sound", winval);
            }
        }
        /* note: any storage required by fft must be allocated here in a 
         * contiguous block of memory who's size is given by the first long
         * in the block. Here, there are 4 more longs after the size, and 
         * then room for 4*len floats (assumes that floats and longs take 
         * equal space).
         *
         * The reason for 4*len floats is to provide space for:
         *    the samples to be transformed (len)
         *    the complex FFT result (2*len)
         *    the window coefficients (len)
         *
         * The reason for this storage restriction is that when a sound is 
         * freed, the block of memory pointed to by extra is also freed. 
         * There is no function call that might free a more complex 
         * structure (this could be added in sound.c, however, if it's 
         * really necessary).
         */
        s->extra = (long *) malloc(sizeof(long) * (4 * len + OFFSET));
        s->extra[0] = sizeof(long) * (4 * len + OFFSET);
        s->CNT = s->INDEX = s->FILLCNT = 0;
        s->TERMCNT = -1;
        maxlen = len;
        window = (float *) &(s->extra[OFFSET + 3 * len]);
        /* fill the window from w */
        if (!w) {
            for (i = 0; i < len; i++) *window++ = 1.0F;
        } else {
            n_samples_from_sound(w, len, window);
        }
    } else {
        maxlen = ((s->extra[0] / sizeof(long)) - OFFSET) / 4;
        if (maxlen != len) xlfail("len changed from initial value");
    }
    samples = (float *) &(s->extra[OFFSET]);
    temp_fft = samples + len;
    window = temp_fft + 2 * len;
    /* step 1: refill buffer with samples */
    fillptr = s->FILLCNT;
    while (fillptr < maxlen) {
        if (s->INDEX == s->CNT) {
            sound_get_next(s, &(s->CNT));
            if (s->SAMPLES == zero_block->samples) {
                if (s->TERMCNT < 0) s->TERMCNT = fillptr;
            }
            s->INDEX = 0;
        }
        samples[fillptr++] = s->SAMPLES[s->INDEX++] * s->scale;
    }
    s->FILLCNT = fillptr;

    /* it is important to test here AFTER filling the buffer, because
     * if fillptr WAS 0 when we hit the zero_block, then filling the 
     * buffer will set TERMCNT to 0.
     */
    if (s->TERMCNT == 0) return NULL;
    
    /* logical stop time is ignored by this code -- to fix this,
     * you would need a way to return the logical stop time to 
     * the caller.
     */

    /* step 2: construct an array and return it */
    xlsave1(result);
    result = newvector(len);

    /* first len floats will be real part, second len floats imaginary
     * copy buffer to temp_fft with windowing
     */
    for (i = 0; i < len; i++) {
        temp_fft[i] = samples[i] * *window++;
        temp_fft[i + len] = 0.0F;
    }
    /* perform the fft: */
    fftnf(1, (const int *) &len, temp_fft, temp_fft + len, 1, -1.0);
    setelement(result, 0, cvflonum(temp_fft[0]));
    for (i = 2; i < len; i += 2) {
        setelement(result, i - 1, cvflonum(temp_fft[i / 2] * 2));
        setelement(result, i, cvflonum(temp_fft[len + (i / 2)] * -2));
    }
    if (len % 2 == 0)
        setelement(result, len - 1, cvflonum(temp_fft[len / 2]));

    /* step 3: shift samples by step */
    if (step < 0) xlfail("step < 0");
    s->FILLCNT -= step;
    if (s->FILLCNT < 0) s->FILLCNT = 0;
    for (i = 0; i < s->FILLCNT; i++) {
        samples[i] = samples[i + step];
    }

    if (s->TERMCNT >= 0) {
        s->TERMCNT -= step;
        if (s->TERMCNT < 0) s->TERMCNT = 0;
    }

    /* step 4: advance in sound to next sample we need
     *   (only does work if step > size of buffer)
     */
    skip = step - maxlen;
    while (skip > 0) {
        long remaining = s->CNT - s->INDEX;
        if (remaining >= skip) {
            s->INDEX += skip;
            skip = 0;
        } else {
            skip -= remaining;
            sound_get_next(s, &(s->CNT));
            s->INDEX = 0;
        }
    }
    
    /* restore the stack */
    xlpop();
    return result;
} /* snd_fetch_array */

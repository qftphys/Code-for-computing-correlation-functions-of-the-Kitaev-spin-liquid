/* initseq_vec.c
 * 
 * Finds the Geyer statstics of the input vector (or each column of matrix)
 * This is a vectorized version of initseq.
 * This is a MEX file for MATLAB
*/

    /* Conversions table From R extentions
     *
     * Real -> mxIsDouble (do a check, cast shouldn't be needed)
     * SEXP -> int or mx*
     * Protect -> Nothing. Memory can be moved around by R functions, but 
     *                      there isn't an analogous thing in mex.
     * R_alloc -> mxCreate* , mxMalloc* ,... or something
     * allocVector -> ''
     * LENGTH -> mxGetN , mxGetM
     * REALSXP -> mxREAL
     */

#include "mex.h"

/* The gateway function */
void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[])
{
    /*getting values and pointers
		some careful checks */
    if( !mxIsDouble(prhs[0]) || 
         mxIsComplex(prhs[0])) {
        mexErrMsgIdAndTxt("initseq:inputs","x must be type double.");
    }
    
	int xM = (int) mxGetM(prhs[0]); /* rows */
	int xN = (int) mxGetN(prhs[0]); /* columns */
    
    double *x = mxGetPr(prhs[0]);
    int len = xM;
    double *xreal = mxGetPr( mxCreateDoubleMatrix(len, 1, mxREAL) );
    
    
    double *gamma_zero_vec;
    double *var_pos_vec;
    double *var_dec_vec;
    
    if (nlhs > 1)
    {    
        plhs[3] =  mxCreateDoubleMatrix(1, xN, mxREAL);
        gamma_zero_vec = mxGetPr( plhs[3] );
    
        plhs[2] =  mxCreateDoubleMatrix(1, xN, mxREAL);
        var_pos_vec = mxGetPr( plhs[2] );

        plhs[1] =  mxCreateDoubleMatrix(1, xN, mxREAL);
        var_dec_vec = mxGetPr( plhs[1] );        
    }
    
    double *var_con_vec;
    plhs[0] =  mxCreateDoubleMatrix(1, xN, mxREAL);
    var_con_vec = mxGetPr( plhs[0] );
    
    /* actual code */
    int l;
    for (l=0; l<xN; ++l) {
    
        double *buff = (double *)mxGetPr( mxCreateDoubleMatrix(len/2, 1, mxREAL) );    
        int i;
        double gamma_zero = 0.0; 

        /*Find the mean*/
        double mean = 0.0;
        for (i = 0; i<len; ++i) {
            xreal[i] = x[l*len+i];
            mean += xreal[i];
        }
        mean /= len;

        /* Subtract the mean */
        for (i = 0; i<len; ++i)
            xreal[i] -= mean;

        /* Actual algorithm */
        for (i = 0; i < len / 2; ++i) {

            int lag1 = 2 * i;
            double gam1 = 0.0;
            for (int j = 0; j + lag1 < len; ++j)
                gam1 += (xreal)[j] * (xreal)[j + lag1];
            gam1 /= len;

            if (i == 0)
                gamma_zero = gam1;

            int lag2 = lag1 + 1;
            double gam2 = 0.0;
            for (int j = 0; j + lag2 < len; ++j)
                gam2 += (xreal)[j] * (xreal)[j + lag2];
            gam2 /= len;

            buff[i] = gam1 + gam2;
            if (buff[i] < 0.0) {
                buff[i] = 0.0;
                ++i;
                break;
            }
        }

        double var_pos = 0.0;
        for (int j = 0; j < i; ++j)
            var_pos += buff[j];

        for (int j = 1; j < i; ++j)
            if (buff[j] > buff[j - 1])
                buff[j] = buff[j - 1];

        double var_dec = 0.0;    
        for (int j = 0; j < i; ++j)
            var_dec += buff[j];

        for (int j = i - 1; j > 0; --j)
            buff[j] -= buff[j - 1];

        /* Pool Adjacent Violators Algorithm (PAVA) */
        double *puff = (double *)mxGetPr( mxCreateDoubleMatrix(i, 1, mxREAL) );
        int *nuff = (int *)mxGetPr( mxCreateDoubleMatrix(i, 1, mxREAL) );
        int nstep = 0;
        for (int j = 1; j < i; ++j) {
            puff[nstep] = buff[j];
            nuff[nstep] = 1;
            ++nstep;
            while(nstep > 1 && puff[nstep - 1] / nuff[nstep - 1]
                < puff[nstep - 2] / nuff[nstep - 2]) {
                puff[nstep - 2] += puff[nstep - 1];
                nuff[nstep - 2] += nuff[nstep - 1];
                --nstep;
            }
        }

        for (int jstep = 0, j = 1; jstep < nstep; ++jstep) {
            double muff = puff[jstep] / nuff[jstep];
            for (int k = 0; k < nuff[jstep]; ++j, ++k)
                buff[j] = buff[j - 1] + muff;
        }

        double var_con = 0.0;  
        for (int j = 0; j < i; ++j)
            var_con += buff[j];

        var_pos *= 2.0;
        var_dec *= 2.0;
        var_con *= 2.0;
        var_pos -= gamma_zero;
        var_dec -= gamma_zero;
        var_con -= gamma_zero;
        
        /* unload data to vector answers */
        if (nlhs > 1)
        {
            gamma_zero_vec[l] = gamma_zero;
            var_pos_vec[l] = var_pos;
            var_dec_vec[l] = var_dec;
        }
        var_con_vec[l] = var_con;
    }
    
    return;
}
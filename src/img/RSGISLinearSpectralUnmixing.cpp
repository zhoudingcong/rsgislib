/*
 *  RSGISLinearSpectralUnmixing.cpp
 *  RSGIS_LIB
 *
 *  Created by Pete Bunting on 02/02/2012.
 *  Copyright 2012 RSGISLib.
 * 
 *  RSGISLib is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  RSGISLib is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RSGISLib.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "RSGISLinearSpectralUnmixing.h"


namespace rsgis{namespace img{

    RSGISCalcLinearSpectralUnmixing::RSGISCalcLinearSpectralUnmixing(std::string gdalFormat, GDALDataType gdalDataType, float gain, float offset)
    {
        this->gdalFormat = gdalFormat;
        this->gdalDataType = gdalDataType;
        this->gain = gain;
        this->offset = offset;
    }
    
    void RSGISCalcLinearSpectralUnmixing::performUnconstainedLinearSpectralUnmixing(GDALDataset **datasets, int numDatasets, std::string outputImage, std::string endmembersFilePath)throw(RSGISImageCalcException)
    {
        try
        {
            unsigned int numOfImageBands = 0;
            for(int i = 0; i < numDatasets; ++i)
            {
                numOfImageBands += datasets[i]->GetRasterCount();
            }            
            
            rsgis::math::RSGISMatrices matrixUtils;
            gsl_matrix *endmembers = matrixUtils.readGSLMatrixFromTxt(endmembersFilePath);
            matrixUtils.printGSLMatrix(endmembers);
            std::cout << std::endl;
            
            if(endmembers->size1 != numOfImageBands)
            {
                gsl_matrix_free(endmembers);
                throw RSGISImageCalcException("The number of image bands and wavelengths within the endmemebers should match.");
            }
            
            if(endmembers->size2 >= endmembers->size1)
            {
                gsl_matrix_free(endmembers);
                throw RSGISImageCalcException("The number of endmember samples should be less than the number of input image bands.");
            }
            
            gsl_matrix *V = gsl_matrix_alloc (endmembers->size2, endmembers->size2);
            gsl_vector *S = gsl_vector_alloc(endmembers->size2);
            gsl_vector *work = gsl_vector_alloc(endmembers->size2);
            int status = gsl_linalg_SV_decomp(endmembers, V, S, work);
            if(status != 0)
            {
                throw RSGISImageCalcException(gsl_strerror(status));
            }
            
            gsl_vector *b = gsl_vector_alloc(endmembers->size1);
            gsl_vector *x = gsl_vector_alloc(endmembers->size2);
            
            RSGISUnconstrainedLinearSpectralUnmixing *calcUnconstrained = new RSGISUnconstrainedLinearSpectralUnmixing(endmembers->size2, endmembers, V, S, work, b, x, this->gain, this->offset);
            RSGISCalcImage calcImage(calcUnconstrained);
            calcImage.calcImage(datasets, numDatasets, outputImage, false, NULL, gdalFormat, gdalDataType);
            
            gsl_matrix_free(endmembers);
            gsl_matrix_free(V);
            gsl_vector_free(S);
            gsl_vector_free(work);
            gsl_vector_free(b);
            gsl_vector_free(x);
            
        }
        catch(RSGISException &e)
        {
            throw RSGISImageCalcException(e.what());
        }
    }
    
    void RSGISCalcLinearSpectralUnmixing::performPartConstainedLinearSpectralUnmixing(GDALDataset **datasets, int numDatasets, std::string outputImage, std::string endmembersFilePath, float weight) throw(RSGISImageCalcException)
    {
        
        try
        {
            unsigned int numOfImageBands = 0;
            for(int i = 0; i < numDatasets; ++i)
            {
                numOfImageBands += datasets[i]->GetRasterCount();
            }            
            
            rsgis::math::RSGISMatrices matrixUtils;
            gsl_matrix *endmembersIn = matrixUtils.readGSLMatrixFromTxt(endmembersFilePath);
            matrixUtils.printGSLMatrix(endmembersIn);
            std::cout << std::endl;
            
            if(endmembersIn->size1 != numOfImageBands)
            {
                gsl_matrix_free(endmembersIn);
                throw RSGISImageCalcException("The number of image bands and wavelengths within the endmemebers should match.");
            }
            
            if(endmembersIn->size2 >= endmembersIn->size1)
            {
                gsl_matrix_free(endmembersIn);
                throw RSGISImageCalcException("The number of endmember samples should be less than the number of input image bands.");
            }
            
            
            gsl_matrix *endmembers = gsl_matrix_alloc (endmembersIn->size1+1, endmembersIn->size2);
            for(unsigned int i = 0; i < endmembersIn->size1; ++i)
            {
                for(unsigned int j = 0; j < endmembersIn->size2; ++j)
                {
                    gsl_matrix_set(endmembers, i, j, gsl_matrix_get(endmembersIn, i, j));
                }
            }
            for(unsigned int j = 0; j < endmembersIn->size2; ++j)
            {
                gsl_matrix_set(endmembers, endmembersIn->size1, j, weight);
            }
            
            
            gsl_matrix *V = gsl_matrix_alloc (endmembers->size2, endmembers->size2);
            gsl_vector *S = gsl_vector_alloc(endmembers->size2);
            gsl_vector *work = gsl_vector_alloc(endmembers->size2);
            int status = gsl_linalg_SV_decomp(endmembers, V, S, work);
            if(status != 0)
            {
                throw RSGISImageCalcException(gsl_strerror(status));
            }
            
            gsl_vector *b = gsl_vector_alloc(endmembers->size1);
            gsl_vector *x = gsl_vector_alloc(endmembers->size2);
            
            RSGISPartConstrainedLinearSpectralUnmixing *calcPartConstrained = new RSGISPartConstrainedLinearSpectralUnmixing(endmembers->size2, weight, endmembers, V, S, work, b, x, this->gain, this->offset);
            RSGISCalcImage calcImage(calcPartConstrained);
            calcImage.calcImage(datasets, numDatasets, outputImage, false, NULL, gdalFormat, gdalDataType);
            
            gsl_matrix_free(endmembersIn);
            gsl_matrix_free(endmembers);
            gsl_matrix_free(V);
            gsl_vector_free(S);
            gsl_vector_free(work);
            gsl_vector_free(b);
            gsl_vector_free(x);
            
        }
        catch(RSGISException &e)
        {
            throw RSGISImageCalcException(e.what());
        }
    }
    
    void RSGISCalcLinearSpectralUnmixing::performConstainedNNLinearSpectralUnmixing(GDALDataset **datasets, int numDatasets, std::string outputImage, std::string endmembersFilePath, float weight) throw(RSGISImageCalcException)
    {
        try
        {
            unsigned int numOfImageBands = 0;
            for(int i = 0; i < numDatasets; ++i)
            {
                numOfImageBands += datasets[i]->GetRasterCount();
            }            
            
            rsgis::math::RSGISVectors vecUtils;
            rsgis::math::RSGISMatrices matrixUtils;
            gsl_matrix *endmembersIn = matrixUtils.readGSLMatrixFromTxt(endmembersFilePath);
            matrixUtils.printGSLMatrix(endmembersIn);
            std::cout << std::endl;
            
            if(endmembersIn->size1 != numOfImageBands)
            {
                throw RSGISImageCalcException("The number of image bands and wavelengths within the endmemebers should match.");
            }
            
            if(endmembersIn->size2 >= endmembersIn->size1)
            {
                gsl_matrix_free(endmembersIn);
                throw RSGISImageCalcException("The number of endmember samples should be less than the number of input image bands.");
            }
            
            /*
            gsl_matrix *endmembers = gsl_matrix_alloc (endmembersIn->size1+1, endmembersIn->size2);
            for(unsigned int i = 0; i < endmembersIn->size1; ++i)
            {
                for(unsigned int j = 0; j < endmembersIn->size2; ++j)
                {
                    gsl_matrix_set(endmembers, i, j, gsl_matrix_get(endmembersIn, i, j));
                }
            }
            
            for(unsigned int j = 0; j < endmembersIn->size2; ++j)
            {
                gsl_matrix_set(endmembers, endmembersIn->size1, j, weight);
            }
            matrixUtils.printGSLMatrix(endmembers);
            std::cout << std::endl;
            
            
            gsl_matrix *V = gsl_matrix_alloc (endmembers->size2, endmembers->size2);
            gsl_vector *S = gsl_vector_alloc(endmembers->size2);
            gsl_vector *work = gsl_vector_alloc(endmembers->size2);
            int outVal = gsl_linalg_SV_decomp(endmembers, V, S, work);
            std::cout << "outVal = " << outVal << std::endl;
            matrixUtils.printGSLMatrix(endmembers);
            
            gsl_vector *b = gsl_vector_alloc(endmembers->size1);
            
            float e1 = 0.2;
            float e2 = 0.7;
            float e3 = 0.1;
            gsl_vector_set(b, 0, ((49.34*e1)+(78.95*e2)+(13.43*e3)));
            gsl_vector_set(b, 1, ((32.67*e1)+(111.47*e2)+(10.39*e3)));
            gsl_vector_set(b, 2, ((579.42*e1)+(206.47*e2)+(33.65*e3)));
            gsl_vector_set(b, 3, ((206.63*e1)+(213.85*e2)+(18.61*e3)));
            gsl_vector_set(b, 4, weight);
            
            std::cout << "B = \n";
            vecUtils.printGSLVector(b);
            
            
            gsl_vector *x = gsl_vector_alloc(endmembers->size2);
            
            outVal = gsl_linalg_SV_solve(endmembers, V, S, b, x);
            std::cout << "outVal = " << outVal << std::endl;
             
            vecUtils.printGSLVector(x);
            */
                        
            int m = endmembersIn->size1+1;
            int n = endmembersIn->size2;
            
            std::cout << "m = " << m << std::endl;
            std::cout << "n = " << n << std::endl;
            
            int mda = m;
            
            double* a = new double[m*n];
            double *b = new double[m];
            double *x = new double[n];
            double rNorm = 0;
            double *w = new double[n];
            double *zz = new double[m];
            int *index = new int[n];
            int mode = 0;
            
            for(unsigned int i = 0; i < endmembersIn->size1; ++i)
            {
                for(unsigned int j = 0; j < endmembersIn->size2; ++j)
                {
                    a[(i*endmembersIn->size2)+j] = gsl_matrix_get(endmembersIn, i, j);
                }
            }
            for(unsigned int j = 0; j < endmembersIn->size2; ++j)
            {
                a[(endmembersIn->size1*endmembersIn->size2)+j] = weight;
            }
            
            for(unsigned int i = 0; i < m; ++i)
            {
                for(unsigned int j = 0; j < n; ++j)
                {
                    if( j == 0 )
                    {
                        std::cout << a[(i*n)+j];
                    }
                    else
                    {
                        std::cout << "," << a[(i*n)+j];
                    }
                }
                std::cout << "\n";
            }
            
            float e1 = 0.5;
            float e2 = 0.3;
            float e3 = 0.2;
            b[0] = ((49.34*e1)+(78.95*e2)+(13.43*e3));
            b[1] = ((32.67*e1)+(111.47*e2)+(10.39*e3));
            b[2] = ((579.42*e1)+(206.47*e2)+(33.65*e3));
            b[3] = ((206.63*e1)+(213.85*e2)+(18.61*e3));
            b[4] = weight;
            
            std::cout << "B: [" << b[0] << "," << b[1] << "," << b[2] << "," << b[3] << "," << b[4] << "]\n";
            
            rsgis::math::RSGISNNLS nnls;
            
            //double* a, int* mda, int* m, int* n, double* b, double* x, double* rnorm, double* w, double* zz, int* index, int* mode
            
            int outVal = nnls.nnls_c(a, &mda, &m, &n, b, x, &rNorm, w, zz, index, &mode); 
            std::cout << "outVal = " << outVal << std::endl;
            std::cout << "rNorm = " << rNorm << std::endl;
            
            std::cout << "E: [" << e1 << "," << e2 << "," << e3 << "]\n";
            std::cout << "X: [" << x[0] << "," << x[1] << "," << x[2] << "]\n";
                        
        }
        catch(RSGISException &e)
        {
            throw RSGISImageCalcException(e.what());
        } 
    }
    
    
    void RSGISCalcLinearSpectralUnmixing::performExhaustiveConstrainedSpectralUnmixing(GDALDataset **datasets, int numDatasets, std::string outputImage, std::string endmembersFilePath, float stepResolution)throw(RSGISImageCalcException)
    {
        try
        {
            unsigned int numOfImageBands = 0;
            for(int i = 0; i < numDatasets; ++i)
            {
                numOfImageBands += datasets[i]->GetRasterCount();
            }            
            
            rsgis::math::RSGISMatrices matrixUtils;
            gsl_matrix *endmembersRaw = matrixUtils.readGSLMatrixFromTxt(endmembersFilePath);
            matrixUtils.printGSLMatrix(endmembersRaw);
            std::cout << std::endl;
            
            if(endmembersRaw->size1 != numOfImageBands)
            {
                throw RSGISImageCalcException("The number of image bands and wavelengths within the endmemebers should match.");
            }
            
            if(endmembersRaw->size2 >= endmembersRaw->size1)
            {
                gsl_matrix_free(endmembersRaw);
                throw RSGISImageCalcException("The number of endmember samples should be less than the number of input image bands.");
            }
            
            if((endmembersRaw->size2 != 3) & (endmembersRaw->size2 != 2))
            {
                throw RSGISImageCalcException("Unmixing is only implemented for 2 or 3 endmembers.");
            }
            
            gsl_matrix *endmembersNorm = matrixUtils.normalisedColumnsMatrix(endmembersRaw);
            matrixUtils.printGSLMatrix(endmembersNorm);
            std::cout << std::endl;
            
            RSGISExhaustiveLinearSpectralUnmixing *calcExhaustive = new RSGISExhaustiveLinearSpectralUnmixing(endmembersNorm->size2+1, endmembersNorm, stepResolution, this->gain, this->offset);
            RSGISCalcImage calcImage(calcExhaustive);
            calcImage.calcImage(datasets, numDatasets, outputImage, false, NULL, gdalFormat, gdalDataType);
            
            delete calcExhaustive;
            gsl_matrix_free(endmembersRaw);
            gsl_matrix_free(endmembersNorm);
        }
        catch(RSGISException &e)
        {
            throw RSGISImageCalcException(e.what());
        }
    }
    
    RSGISCalcLinearSpectralUnmixing::~RSGISCalcLinearSpectralUnmixing()
    {
        
    }
    
    
    RSGISUnconstrainedLinearSpectralUnmixing::RSGISUnconstrainedLinearSpectralUnmixing(int numberOutBands, gsl_matrix *endmembers, gsl_matrix *V, gsl_vector *S, gsl_vector *work, gsl_vector *b, gsl_vector *x, float gain, float offset):RSGISCalcImageValue(numberOutBands)
    {
        this->endmembers = endmembers;
        this->V = V;
        this->S = S;
        this->work = work;
        this->b = b;
        this->x = x;
        this->gain = gain;
        this->offset = offset;
    }
        
    void RSGISUnconstrainedLinearSpectralUnmixing::calcImageValue(float *bandValues, int numBands, double *output) throw(RSGISImageCalcException)
    {
        try 
        {
            if(b->size != numBands)
            {
                throw RSGISImageCalcException("The size vector of for the input data is not equal to the number image bands.");
            }
            
            if(x->size != this->numOutBands)
            {
                throw RSGISImageCalcException("The size of the output vector is not the same of the number of output image bands.");
            }
            
            for(int i = 0; i < numBands; ++i)
            {
                gsl_vector_set(b, i, bandValues[i]);
            }
            
            int status = gsl_linalg_SV_solve(endmembers, V, S, b, x);
            if(status != 0)
            {
                throw RSGISImageCalcException(gsl_strerror(status));
            }
            
            for(int i = 0; i < x->size; ++i)
            {
                output[i] = offset + (gsl_vector_get(x, i)*gain);
            }
        }
        catch (RSGISImageCalcException &e) 
        {
            throw e;
        }
    }
    
    RSGISUnconstrainedLinearSpectralUnmixing::~RSGISUnconstrainedLinearSpectralUnmixing()
    {
        
    }
    
    
    RSGISPartConstrainedLinearSpectralUnmixing::RSGISPartConstrainedLinearSpectralUnmixing(int numberOutBands, float weight, gsl_matrix *endmembers, gsl_matrix *V, gsl_vector *S, gsl_vector *work, gsl_vector *b, gsl_vector *x, float gain, float offset):RSGISCalcImageValue(numberOutBands)
    {
        this->weight = weight;
        this->endmembers = endmembers;
        this->V = V;
        this->S = S;
        this->work = work;
        this->b = b;
        this->x = x;
        this->gain = gain;
        this->offset = offset;
    }
    
    void RSGISPartConstrainedLinearSpectralUnmixing::calcImageValue(float *bandValues, int numBands, double *output) throw(RSGISImageCalcException)
    {
        try 
        {
            if((b->size-1) != numBands)
            {
                throw RSGISImageCalcException("The size vector of for the input data is not equal to the number image bands.");
            }
            
            if(x->size != this->numOutBands)
            {
                throw RSGISImageCalcException("The size of the output vector is not the same of the number of output image bands.");
            }
            
            for(int i = 0; i < numBands; ++i)
            {
                gsl_vector_set(b, i, bandValues[i]);
            }
            gsl_vector_set(b, numBands, weight);
            
            int status = gsl_linalg_SV_solve(endmembers, V, S, b, x);
            if(status != 0)
            {
                throw RSGISImageCalcException(gsl_strerror(status));
            }
            
            for(int i = 0; i < x->size; ++i)
            {
                output[i] = offset + (gsl_vector_get(x, i)*gain);
            }
        }
        catch (RSGISImageCalcException &e) 
        {
            throw e;
        }
    }
    
    RSGISPartConstrainedLinearSpectralUnmixing::~RSGISPartConstrainedLinearSpectralUnmixing()
    {
        
    }
    
    
    RSGISExhaustiveLinearSpectralUnmixing::RSGISExhaustiveLinearSpectralUnmixing(int numberOutBands, gsl_matrix *endmembers, float stepRes, float gain, float offset):RSGISCalcImageValue(numberOutBands)
    {
        this->endmembers = endmembers;
        this->stepRes = stepRes;
        this->numOfEndMembers = endmembers->size2;
        this->gain = gain;
        this->offset = offset;
    }
    
    void RSGISExhaustiveLinearSpectralUnmixing::calcImageValue(float *bandValues, int numBands, double *output) throw(RSGISImageCalcException)
    {
        // All values have to be greater than zero.
        
        // Find the linear combination which is closest to 1 but less than 1. 
        
        // Also find the linear combination which is closest to 1 but greater than 1.
        
        // Provide option to except greater than 1 if closer result than less than 1 and under some threshold.
        
        unsigned int numOfSteps = (1/this->stepRes)+1;
        
        float threshold = 1 + this->stepRes;
        
        float *normBandVals = new float[numBands];
        
        double sqSum = 0;
        for(int i = 0; i < numBands; ++i)
        {
            sqSum += (bandValues[i]*bandValues[i]);
        }
        
        float normVal = sqrt(sqSum);
        
        if(this->numOfEndMembers == 2)
        {
            float em1Val = 0;
            float em2Val = 0;
            
            bool first = true;
            float distVal = 0;
            float minError = 0;
            float minEM1Val = 0;
            float minEM2Val = 0;
            
            if( normVal > 0)
            {
                for(int i = 0; i < numBands; ++i)
                {
                    normBandVals[i] = bandValues[i]/normVal;
                }
                
                for(unsigned int i = 0; i < numOfSteps; ++i)
                {
                    em2Val = 0;
                    for(unsigned int j = 0; j < numOfSteps; ++j)
                    {
                        if((em1Val+em2Val) < threshold)
                        {
                            distVal = this->calcDistance2MeasuredSpectra(em1Val, em2Val, normBandVals, numBands);
                            //std::cout << "[" << em1Val << "," << em2Val << "] = " << (em1Val+em2Val) << " Deviation = " << distVal << std::endl;
                            
                            if(first)
                            {
                                minError = distVal;
                                minEM1Val = em1Val;
                                minEM2Val = em2Val;
                                first = false;
                            }
                            else if(distVal < minError)
                            {
                                minError = distVal;
                                minEM1Val = em1Val;
                                minEM2Val = em2Val;
                            }
                        }
                        em2Val += this->stepRes;
                    }
                    em1Val += this->stepRes;
                }
                //std::cout << std::endl;
            }
            
            if(!first)
            {
                //std::cout << "Min Error = " << minError << std::endl;
                output[0] = offset + (minEM1Val * gain);
                output[1] = offset + (minEM2Val * gain);
                output[2] = offset + (minError * gain);
            }
            else
            {
                output[0] = 0;
                output[1] = 0;
                output[2] = 0;
            }
        }
        else if(this->numOfEndMembers == 3)
        {
            float em1Val = 0;
            float em2Val = 0;
            float em3Val = 0;
            
            bool first = true;
            float distVal = 0;
            float minError = 0;
            float minEM1Val = 0;
            float minEM2Val = 0;
            float minEM3Val = 0;
                        
            if( normVal > 0)
            {
                for(int i = 0; i < numBands; ++i)
                {
                    normBandVals[i] = bandValues[i]/normVal;
                }
                
                for(unsigned int i = 0; i < numOfSteps; ++i)
                {
                    em2Val = 0;
                    for(unsigned int j = 0; j < numOfSteps; ++j)
                    {
                        em3Val = 0;
                        for(unsigned int k = 0; k < numOfSteps; ++k)
                        {
                            if((em1Val+em2Val+em3Val) < threshold)
                            {
                                distVal = this->calcDistance2MeasuredSpectra(em1Val, em2Val, em3Val, normBandVals, numBands);
                                //std::cout << "[" << em1Val << "," << em2Val << "," << em3Val << "] = " << (em1Val+em2Val+em3Val) << " Deviation = " << distVal << std::endl;
                                
                                if(first)
                                {
                                    minError = distVal;
                                    minEM1Val = em1Val;
                                    minEM2Val = em2Val;
                                    minEM3Val = em3Val;
                                    first = false;
                                }
                                else if(distVal < minError)
                                {
                                    minError = distVal;
                                    minEM1Val = em1Val;
                                    minEM2Val = em2Val;
                                    minEM3Val = em3Val; 
                                }
                            }
                            em3Val += this->stepRes;
                        }
                        em2Val += this->stepRes;
                    }
                    em1Val += this->stepRes;
                }
                //std::cout << std::endl;
            }
            
            if(!first)
            {
                //std::cout << "Min Error = " << minError << std::endl;
                output[0] = offset + (minEM1Val * gain);
                output[1] = offset + (minEM2Val * gain);
                output[2] = offset + (minEM3Val * gain);
                output[3] = offset + (minError * gain);
            }
            else
            {
                output[0] = 0;
                output[1] = 0;
                output[2] = 0;
                output[3] = 0;
            }
        }
        else
        {
            throw RSGISImageCalcException("Unmixing is only implemented for 3 endmembers.");
        }
                
        delete[] normBandVals;
    }
    
    float RSGISExhaustiveLinearSpectralUnmixing::calcDistance2MeasuredSpectra(float em1Val, float em2Val, float em3Val, float *normSpectra, unsigned int numBands) throw(RSGISImageCalcException)
    {
        float *genSpectra = new float[numBands];
        for(unsigned int i = 0; i < numBands; ++i)
        {
            genSpectra[i] = (gsl_matrix_get(endmembers, i, 0) * em1Val) + (gsl_matrix_get(endmembers, i, 1) * em2Val) + (gsl_matrix_get(endmembers, i, 2) * em3Val);
            //std::cout << i << ": " <<  genSpectra[i] << std::endl;
        }
        
        float errorVal = 0;
        for(unsigned int i = 0; i < numBands; ++i)
        {
            errorVal += ((genSpectra[i] - normSpectra[i])*(genSpectra[i] - normSpectra[i]));
        }
        
        //std::cout << "errorVal = " << errorVal << std::endl;
        
        delete[] genSpectra;
        
        errorVal = sqrt(errorVal/numBands);
        
        return errorVal;
    }
    
    float RSGISExhaustiveLinearSpectralUnmixing::calcDistance2MeasuredSpectra(float em1Val, float em2Val, float *normSpectra, unsigned int numBands) throw(RSGISImageCalcException)
    {
        float *genSpectra = new float[numBands];
        for(unsigned int i = 0; i < numBands; ++i)
        {
            genSpectra[i] = (gsl_matrix_get(endmembers, i, 0) * em1Val) + (gsl_matrix_get(endmembers, i, 1) * em2Val);
            //std::cout << i << ": " <<  genSpectra[i] << std::endl;
        }
        
        float errorVal = 0;
        for(unsigned int i = 0; i < numBands; ++i)
        {
            errorVal += ((genSpectra[i] - normSpectra[i])*(genSpectra[i] - normSpectra[i]));
        }
        
        //std::cout << "errorVal = " << errorVal << std::endl;
        
        delete[] genSpectra;
        
        errorVal = sqrt(errorVal/numBands);
        
        return errorVal;
    }
    
    RSGISExhaustiveLinearSpectralUnmixing::~RSGISExhaustiveLinearSpectralUnmixing()
    {
        
    }
    
    
}}

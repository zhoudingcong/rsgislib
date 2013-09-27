/*
 *  RSGISCmdImageCalc.h
 *
 *
 *  Created by Pete Bunting on 29/04/2013.
 *  Copyright 2013 RSGISLib.
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

#ifndef RSGISCmdImageCalc_H
#define RSGISCmdImageCalc_H

#include <iostream>
#include <string>
#include <vector>

#include "common/RSGISCommons.h"
#include "RSGISCmdException.h"
#include "RSGISCmdCommon.h"

namespace rsgis{ namespace cmds {

    struct VariableStruct
    {
        std::string image;
        std::string name;
        int bandNum;
    };

    enum RSGISInitClustererMethods
    {
        rsgis_init_random,
        rsgis_init_diagonal_full,
        rsgis_init_diagonal_stddev,
        rsgis_init_diagonal_full_attach,
        rsgis_init_diagonal_stddev_attach,
        rsgis_init_kpp
    };

    /** Function to run the band maths tools */
    void executeBandMaths(VariableStruct *variables, unsigned int numVars, std::string outputImage, std::string mathsExpression, std::string gdalFormat, RSGISLibDataType outDataType)throw(RSGISCmdException);
    /** Function to run the image maths tools */
    void executeImageMaths(std::string inputImage, std::string outputImage, std::string mathsExpression, std::string imageFormat, RSGISLibDataType outDataType)throw(RSGISCmdException);
    /** Function to run the KMeans tool */
    void executeKMeansClustering(std::string inputImage, std::string outputMatrixFile, unsigned int numClusters, unsigned int maxNumIterations, unsigned int subSample, bool ignoreZeros, float degreeOfChange, RSGISInitClustererMethods initClusterMethod)throw(RSGISCmdException);
    /** Function to run the KMeans tool */
    void executeISODataClustering(std::string inputImage, std::string outputMatrixFile, unsigned int numClusters, unsigned int maxNumIterations, unsigned int subSample, bool ignoreZeros, float degreeOfChange, RSGISInitClustererMethods initClusterMethod, float minDistBetweenClusters, unsigned int minNumFeatures, float maxStdDev, unsigned int minNumClusters, unsigned int startIteration, unsigned int endIteration)throw(RSGISCmdException);
    /** Function to run mahalanobis distance Window Filter */
    void executeMahalanobisDistFilter(std::string inputImage, std::string outputImage, unsigned int winSize, std::string gdalFormat, RSGISLibDataType outDataType)throw(RSGISCmdException);
    /** Function to run mahalanobis distance Image to Window Filter */
    void executeMahalanobisDist2ImgFilter(std::string inputImage, std::string outputImage, unsigned int winSize, std::string gdalFormat, RSGISLibDataType outDataType)throw(RSGISCmdException);
    /** Function to run image calculate distance command */
    void executeImageCalcDistance(std::string inputImage, std::string outputImage, std::string gdalFormat)throw(RSGISCmdException);
    /** Function to calculate summary statistics for a column of pixels */
    void executeImagePixelColumnSummary(std::string inputImage, std::string outputImage, rsgis::cmds::RSGISCmdStatsSummary summaryStats, std::string gdalFormat, RSGISLibDataType outDataType, float noDataValue, bool useNoDataValue)throw(RSGISCmdException);
    /** Function to perform a linear regression on each column of pixels */
    void executeImagePixelLinearFit(std::string inputImage, std::string outputImage, std::string gdalFormat, std::string bandValues, float noDataValue, bool useNoDataValue)throw(RSGISCmdException);
    /** Function to perform image normalisation */
    void executeNormalisation(std::vector<std::string> inputImages, std::vector<std::string> outputImages, bool calcInMinMax, double inMin, double inMax, double outMin, double outMax)throw(RSGISCmdException);
    /** Function to calculate the correlation between 2 images */
    void executeCorrelation(std::string inputImageA, std::string inputImageB, std::string outputMatrix) throw(RSGISCmdException);
    /** Function to calculate the covariance between 2 images */
    void executeCovariance(std::string inputImageA, std::string inputImageB, std::string inputMatrixA, std::string inputMatrixB, bool shouldCalcMean, std::string outputMatrix)throw(RSGISCmdException);
    /** Function to calculate the mean vector of an image */
    void executeMeanVector(std::string inputImage, std::string outputMatrix)throw(RSGISCmdException);
    /** Function to perform principal components analysis of an image */
    void executePCA(std::string eigenvectors, std::string inputImage, std::string outputImage, int numComponents)throw(RSGISCmdException);
    /** Function to generate a standardised image using the mean vector provided */
    void executeStandardise(std::string meanvectorStr, std::string inputImage, std::string outputImage)throw(RSGISCmdException);
    /** Function to replace values less then given, using a threshold */
    void executeReplaceValuesLessThan(std::string inputImage, std::string outputImage, double threshold, double value)throw(RSGISCmdException);
    /** Function to convert the image spectra to unit area */
    void executeUnitArea(std::string inputImage, std::string outputImage, std::string inMatrixfile)throw(RSGISCmdException);
    /** Function to calculate the speed of movement (mean, min and max) */
    void executeMovementSpeed(std::vector<std::string> inputImages, std::vector<unsigned int> imageBands, std::vector<float> imageTimes, float upper, float lower, std::string outputImage)throw(RSGISCmdException);
    /** Function that counts the number of values with a given range for each column*/
    void executeCountValsInCols(std::string inputImage, float upper, float lower, std::string outputImage)throw(RSGISCmdException);
    /** Function to calculate the root mean squared error between 2 images */
    void executeCalculateRMSE(std::string inputImageA, int inputBandA, std::string inputImageB, int inputBandB)throw(RSGISCmdException);
    /** Function to apply 2 var function to image */
    void executeApply2VarFunction(std::string inputImage, void *twoVarFunction, std::string outputImage)throw(RSGISCmdException);
    /** Function to apply 3 var function to image */
    void executeApply3VarFunction(std::string inputImage, void *threeVarFunction, std::string outputImage)throw(RSGISCmdException);
    /** Function to calculate the distance to the nearest geometry for each pixel in an image */
    void executeDist2Geoms(std::string inputVector, float imgResolution, std::string outputImage)throw(RSGISCmdException);
    /** Function to calculate statistics for individual image bands */
    void executeImageBandStats(std::string inputImage, std::string outputFile, bool ignoreZeros)throw(RSGISCmdException);
    /** Function to calculate the statistics for the whole image across all bands */
    void executeImageStats(std::string inputImage, std::string outputFile, bool ignoreZeros)throw(RSGISCmdException);
    /** Function to undertake an unconstrained linear spectral unmixing of the input image for a set of endmembers */
    void executeUnconLinearSpecUnmix(std::string inputImage, std::string imageFormat, RSGISLibDataType outDataType, float lsumGain, float lsumOffset, std::string outputFile, std::string endmembersFile)throw(RSGISCmdException);
    /** Function to undertake an exhaustive constrained linear spectral unmixing of the input image for a set of endmembers */
    void executeExhconLinearSpecUnmix(std::string inputImage, std::string imageFormat, RSGISLibDataType outDataType, float lsumGain, float lsumOffset, std::string outputFile, std::string endmembersFile, float stepResolution)throw(RSGISCmdException);
    /** Function to undertake a partially constrained linear spectral unmixing of the input image for a set of endmembers where the sum of the unmixing will be approximately 1*/
    void executeConSum1LinearSpecUnmix(std::string inputImage, std::string imageFormat, RSGISLibDataType outDataType, float lsumGain, float lsumOffset, float lsumWeight, std::string outputFile, std::string endmembersFile)throw(RSGISCmdException);
    /** Function to undertake a constrained linear spectral unmixing of the input image for a set of endmembers where the sum of the unmixing will be approximately 1 and non-negative */
    void executeNnConSum1LinearSpecUnmix(std::string inputImage, std::string imageFormat, RSGISLibDataType outDataType, float lsumGain, float lsumOffset, float lsumWeight, std::string outputFile, std::string endmembersFile)throw(RSGISCmdException);
    /** Function to test whether all bands are equal to the same value */
    void executeAllBandsEqualTo(std::string inputImage, float imgValue, float outputTrueVal, float outputFalseVal, std::string outputImage, std::string imageFormat, RSGISLibDataType outDataType)throw(RSGISCmdException);
    /** Function to generate a histogram for the region of the mask selected */
    void executeHistogram(std::string inputImage, std::string imageMask, std::string outputFile, unsigned int imgBand, float imgValue, double binWidth, bool calcInMinMax, double inMin, double inMax)throw(RSGISCmdException);
    /** Function to generate a histogram and return it */
    unsigned int* executeGetHistogram(std::string inputImage, unsigned int imgBand, double binWidth, unsigned int *nBins, bool calcInMinMax, double *inMin, double *inMax)throw(RSGISCmdException);
    /** Function to calculate image band percentiles */
    void executeBandPercentile(std::string inputImage, float percentile, float noDataValue, bool noDataValueSpecified, std::string outputFile)throw(RSGISCmdException);
    /** Function to calculate the distance to the nearest geometry for every pixel in an image */
    void executeImageDist2Geoms(std::string inputImage, std::string inputVector, std::string imageFormat, std::string outputImage)throw(RSGISCmdException);
}}


#endif


/*
 *  RSGISCmdImageCalc.cpp
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

#include "RSGISCmdImageCalc.h"
#include "RSGISCmdParent.h"

#include "common/RSGISImageException.h"

#include "img/RSGISBandMath.h"
#include "img/RSGISImageMaths.h"
#include "img/RSGISImageCalcException.h"
#include "img/RSGISCalcImageValue.h"
#include "img/RSGISCalcImage.h"
#include "img/RSGISImageClustering.h"
#include "img/RSGISImageWindowStats.h"
#include "img/RSGISImageStatistics.h"
#include "img/RSGISCalcCovariance.h"
#include "img/RSGISCalcEditImage.h"
#include "img/RSGISCalcDist2Geom.h"
#include "img/RSGISCalcCorrelationCoefficient.h"
#include "img/RSGISMeanVector.h"
#include "img/RSGISCalcImageMatrix.h"
#include "img/RSGISFitFunction2Pxls.h"
#include "img/RSGISImageNormalisation.h"
#include "img/RSGISStandardiseImage.h"
#include "img/RSGISApplyEigenvectors.h"
#include "img/RSGISReplaceValuesLessThanGivenValue.h"
#include "img/RSGISConvertSpectralToUnitArea.h"
#include "img/RSGISCalculateImageMovementSpeed.h"
#include "img/RSGISCountValsAboveThresInCol.h"
#include "img/RSGISCalcRMSE.h"
#include "img/RSGISApplyFunction.h"
#include "img/RSGISLinearSpectralUnmixing.h"


#include "math/RSGISVectors.h"
#include "math/RSGISMatrices.h"

#include "utils/RSGISTextUtils.h"
#include "utils/RSGISFileUtils.h"

#include "vec/RSGISVectorUtils.h"
#include "vec/RSGISGetOGRGeometries.h"
#include "vec/RSGISProcessVector.h"

#include "geom/RSGISGeometry.h"

#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_blas.h>

#include "exe/rsgis/RSGISExeImageCalculation.h"

#include "muParser.h"

namespace rsgis{ namespace cmds {
    
    void executeBandMaths(VariableStruct *variables, unsigned int numVars, std::string outputImage, std::string mathsExpression, std::string gdalFormat, RSGISLibDataType outDataType)throw(RSGISCmdException)
    {
        std::string *outBandName = new std::string[1];
        outBandName[0] = mathsExpression;
        
        GDALAllRegister();
        GDALDataset **datasets = NULL;
        rsgis::img::RSGISBandMath *bandmaths = NULL;
        rsgis::img::RSGISCalcImage *calcImage = NULL;
        mu::Parser *muParser = new mu::Parser();
        
        try
        {
            rsgis::img::VariableBands **processVaribles = new rsgis::img::VariableBands*[numVars];
            datasets = new GDALDataset*[numVars];
            
            int numRasterBands = 0;
            int totalNumRasterBands = 0;
            
            for(int i = 0; i < numVars; ++i)
            {
                std::cout << variables[i].image << std::endl;
                datasets[i] = (GDALDataset *) GDALOpen(variables[i].image.c_str(), GA_ReadOnly);
                if(datasets[i] == NULL)
                {
                    std::string message = std::string("Could not open image ") + variables[i].image;
                    throw rsgis::RSGISImageException(message.c_str());
                }
                
                numRasterBands = datasets[i]->GetRasterCount();
                
                if((variables[i].bandNum < 0) | (variables[i].bandNum > numRasterBands))
                {
                    throw rsgis::RSGISImageException("You have specified a band which is not within the image");
                }
                
                processVaribles[i] = new rsgis::img::VariableBands();
                processVaribles[i]->name = variables[i].name;
                processVaribles[i]->band = totalNumRasterBands + (variables[i].bandNum - 1);
                
                totalNumRasterBands += numRasterBands;
            }
            
            mu::value_type *inVals = new mu::value_type[numVars];
            for(int i = 0; i < numVars; ++i)
            {
                inVals[i] = 0;
                muParser->DefineVar(_T(processVaribles[i]->name.c_str()), &inVals[i]);
            }
            
            muParser->SetExpr(mathsExpression.c_str());
            
            bandmaths = new rsgis::img::RSGISBandMath(1, processVaribles, numVars, muParser);
            
            calcImage = new rsgis::img::RSGISCalcImage(bandmaths, "", true);
            calcImage->calcImage(datasets, numVars, outputImage, true, outBandName, gdalFormat, RSGIS_to_GDAL_Type(outDataType));
            
            for(int i = 0; i < numVars; ++i)
            {
                GDALClose(datasets[i]);
                delete processVaribles[i];
            }
            delete[] datasets;
            delete[] processVaribles;
            
            delete[] inVals;
            
            delete muParser;
            delete bandmaths;
            delete calcImage;
            delete[] outBandName;
            //GDALDestroyDriverManager();
        }
        catch(rsgis::RSGISImageException &e)
        {
            throw RSGISCmdException(e.what());
        }
        catch(rsgis::RSGISException &e)
        {
            throw RSGISCmdException(e.what());
        }
        catch (mu::ParserError &e)
        {
            std::string message = std::string("ERROR: ") + std::string(e.GetMsg()) + std::string(":\t \'") + std::string(e.GetExpr()) + std::string("\'");
            throw RSGISCmdException(message);
        }
        catch (std::exception &e)
        {
            throw RSGISCmdException(e.what());
        }
    }
    
    void executeImageMaths(std::string inputImage, std::string outputImage, std::string mathsExpression, std::string imageFormat, RSGISLibDataType outDataType)throw(RSGISCmdException)
    {
        GDALAllRegister();
        GDALDataset **datasets = NULL;
        rsgis::img::RSGISImageMaths *imageMaths = NULL;
        rsgis::img::RSGISCalcImage *calcImage = NULL;
        mu::Parser *muParser = new mu::Parser();
        
        try
        {
            datasets = new GDALDataset*[1];
            
            datasets[0] = (GDALDataset *) GDALOpen(inputImage.c_str(), GA_ReadOnly);
            if(datasets[0] == NULL)
            {
                std::string message = std::string("Could not open image ") + inputImage;
                throw rsgis::RSGISImageException(message.c_str());
            }
            
            int numRasterBands = datasets[0]->GetRasterCount();
            
            mu::value_type inVal;
            muParser->DefineVar(_T("b1"), &inVal);
            muParser->SetExpr(mathsExpression.c_str());
            
            imageMaths = new rsgis::img::RSGISImageMaths(numRasterBands, muParser);
            
            calcImage = new rsgis::img::RSGISCalcImage(imageMaths, "", true);
            calcImage->calcImage(datasets, 1, outputImage, false, NULL, imageFormat, RSGIS_to_GDAL_Type(outDataType));
            
            GDALClose(datasets[0]);
            delete[] datasets;
            
            delete muParser;
            delete imageMaths;
            delete calcImage;
            //GDALDestroyDriverManager();
        }
        catch(rsgis::RSGISImageException &e)
        {
            throw RSGISCmdException(e.what());
        }
        catch(rsgis::RSGISException &e)
        {
            throw RSGISCmdException(e.what());
        }
        catch (mu::ParserError &e)
        {
            std::string message = std::string("ERROR: ") + std::string(e.GetMsg()) + std::string(":\t \'") + std::string(e.GetExpr()) + std::string("\'");
            throw RSGISCmdException(message);
        }
        catch(std::exception &e)
        {
            throw RSGISCmdException(e.what());
        }
    }
    
    void executeKMeansClustering(std::string inputImage, std::string outputMatrixFile, unsigned int numClusters, unsigned int maxNumIterations, unsigned int subSample, bool ignoreZeros, float degreeOfChange, RSGISInitClustererMethods initClusterMethod)throw(RSGISCmdException)
    {
        try
        {
            GDALAllRegister();
            GDALDataset *dataset = (GDALDataset *) GDALOpenShared(inputImage.c_str(), GA_ReadOnly);
            if(dataset == NULL)
            {
                std::string message = std::string("Could not open image ") + inputImage;
                throw rsgis::RSGISImageException(message.c_str());
            }
            
            rsgis::math::InitClustererMethods initMethod = rsgis::math::init_diagonal_full_attach;
            
            switch( initClusterMethod )
            {
                case rsgis::cmds::rsgis_init_random:
                    initMethod = rsgis::math::init_random;
                    break;
                case rsgis::cmds::rsgis_init_diagonal_full:
                    initMethod = rsgis::math::init_diagonal_full;
                    break;
                case rsgis::cmds::rsgis_init_diagonal_stddev:
                    initMethod = rsgis::math::init_diagonal_stddev;
                    break;
                case rsgis::cmds::rsgis_init_diagonal_full_attach:
                    initMethod = rsgis::math::init_diagonal_full_attach;
                    break;
                case rsgis::cmds::rsgis_init_diagonal_stddev_attach:
                    initMethod = rsgis::math::init_diagonal_stddev_attach;
                    break;
                case rsgis::cmds::rsgis_init_kpp:
                    initMethod = rsgis::math::init_kpp;
                    break;
                default:
                    initMethod = rsgis::math::init_diagonal_full_attach;
                    break;
            }
            
            rsgis::img::RSGISImageClustering imgClustering;
            imgClustering.findKMeansCentres(dataset, outputMatrixFile, numClusters, maxNumIterations, subSample, ignoreZeros, degreeOfChange, initMethod);
            
            GDALClose(dataset);
            GDALDestroyDriverManager();
        }
        catch(rsgis::RSGISException &e)
        {
            throw RSGISCmdException(e.what());
        }
        catch(std::exception &e)
        {
            throw RSGISCmdException(e.what());
        }
    }

    void executeISODataClustering(std::string inputImage, std::string outputMatrixFile, unsigned int numClusters, unsigned int maxNumIterations, unsigned int subSample, bool ignoreZeros, float degreeOfChange, RSGISInitClustererMethods initClusterMethod, float minDistBetweenClusters, unsigned int minNumFeatures, float maxStdDev, unsigned int minNumClusters, unsigned int startIteration, unsigned int endIteration)throw(RSGISCmdException)
    {
        try
        {
            GDALAllRegister();
            GDALDataset *dataset = (GDALDataset *) GDALOpenShared(inputImage.c_str(), GA_ReadOnly);
            if(dataset == NULL)
            {
                std::string message = std::string("Could not open image ") + inputImage;
                throw rsgis::RSGISImageException(message.c_str());
            }
            
            rsgis::math::InitClustererMethods initMethod = rsgis::math::init_diagonal_full_attach;
            
            switch( initClusterMethod )
            {
                case rsgis::cmds::rsgis_init_random:
                    initMethod = rsgis::math::init_random;
                    break;
                case rsgis::cmds::rsgis_init_diagonal_full:
                    initMethod = rsgis::math::init_diagonal_full;
                    break;
                case rsgis::cmds::rsgis_init_diagonal_stddev:
                    initMethod = rsgis::math::init_diagonal_stddev;
                    break;
                case rsgis::cmds::rsgis_init_diagonal_full_attach:
                    initMethod = rsgis::math::init_diagonal_full_attach;
                    break;
                case rsgis::cmds::rsgis_init_diagonal_stddev_attach:
                    initMethod = rsgis::math::init_diagonal_stddev_attach;
                    break;
                case rsgis::cmds::rsgis_init_kpp:
                    initMethod = rsgis::math::init_kpp;
                    break;
                default:
                    initMethod = rsgis::math::init_diagonal_full_attach;
                    break;
            }
            
            rsgis::img::RSGISImageClustering imgClustering;
            imgClustering.findISODataCentres(dataset, outputMatrixFile, numClusters, maxNumIterations, subSample, ignoreZeros, degreeOfChange, initMethod, minDistBetweenClusters, minNumFeatures, maxStdDev, minNumClusters, startIteration, endIteration);
            
            GDALClose(dataset);
            GDALDestroyDriverManager();
        }
        catch(rsgis::RSGISException &e)
        {
            throw RSGISCmdException(e.what());
        }
        catch(std::exception &e)
        {
            throw RSGISCmdException(e.what());
        }
    }
    
    void executeMahalanobisDistFilter(std::string inputImage, std::string outputImage, unsigned int winSize, std::string gdalFormat, RSGISLibDataType outDataType)throw(RSGISCmdException)
    {
        try
        {
            GDALAllRegister();
            GDALDataset **datasets = new GDALDataset*[1];
            
            datasets[0] = (GDALDataset *) GDALOpen(inputImage.c_str(), GA_ReadOnly);
            if(datasets[0] == NULL)
            {
                std::string message = std::string("Could not open image ") + inputImage;
                throw rsgis::RSGISImageException(message.c_str());
            }
            
            rsgis::img::RSGISCalcImage *calcImage = NULL;
            
            rsgis::img::RSGISCalcImgPxlNeighboursDist *calcDistWindow = new rsgis::img::RSGISCalcImgPxlNeighboursDist();
            
            calcImage = new rsgis::img::RSGISCalcImage(calcDistWindow, "", true);
            calcImage->calcImageWindowData(datasets, 1, outputImage, winSize, gdalFormat, RSGIS_to_GDAL_Type(outDataType));
            delete calcDistWindow;
            delete calcImage;
            
            
            
            GDALClose(datasets[0]);
            delete[] datasets;
            GDALDestroyDriverManager();
        }
        catch(rsgis::RSGISException &e)
        {
            throw RSGISCmdException(e.what());
        }
        catch(std::exception &e)
        {
            throw RSGISCmdException(e.what());
        }
    }
    
    void executeMahalanobisDist2ImgFilter(std::string inputImage, std::string outputImage, unsigned int winSize, std::string gdalFormat, RSGISLibDataType outDataType)throw(RSGISCmdException)
    {
        try
        {
            GDALAllRegister();
            GDALDataset **datasets = new GDALDataset*[1];
            
            datasets[0] = (GDALDataset *) GDALOpen(inputImage.c_str(), GA_ReadOnly);
            if(datasets[0] == NULL)
            {
                std::string message = std::string("Could not open image ") + inputImage;
                throw rsgis::RSGISImageException(message.c_str());
            }
            
            rsgis::img::RSGISCalcImage *calcImage = NULL;
            
            // Initial Mahalanobis distance Calculation
            int numRasterBands = datasets[0]->GetRasterCount();
            rsgis::img::ImageStats **imgStats = new rsgis::img::ImageStats*[numRasterBands];
            for(int i = 0; i < numRasterBands; ++i)
            {
                imgStats[i] = new rsgis::img::ImageStats();
                imgStats[i]->max = 0;
                imgStats[i]->min = 0;
                imgStats[i]->mean = 0;
                imgStats[i]->sum = 0;
                imgStats[i]->stddev = 0;
            }
            rsgis::img::RSGISImageStatistics calcStats;
            calcStats.calcImageStatistics(datasets, 1, imgStats, numRasterBands, true, true);
            
            rsgis::math::RSGISVectors vecUtils;
            rsgis::math::RSGISMatrices matrixUtils;
            rsgis::math::Vector *varMeans = vecUtils.createVector(numRasterBands);
            for(int i = 0; i < numRasterBands; ++i)
            {
                varMeans->vector[i] = imgStats[i]->mean;
                delete imgStats[i];
            }
            delete[] imgStats;
            
            rsgis::math::Matrix *covarianceMatrix = matrixUtils.createMatrix(numRasterBands, numRasterBands);
            matrixUtils.setValues(covarianceMatrix, 0.0);
            
            rsgis::img::RSGISCreateCovarianceMatrix createCovarMatrix = rsgis::img::RSGISCreateCovarianceMatrix(varMeans, covarianceMatrix);
            calcImage = new rsgis::img::RSGISCalcImage(&createCovarMatrix, "", true);
            calcImage->calcImage(datasets, 1);
            delete calcImage;
            
            std::cout << "Covariance Matrix:\n";
            matrixUtils.printMatrix(covarianceMatrix);
            
            size_t numVals = covarianceMatrix->m;
            
            gsl_matrix *coVarGSL = matrixUtils.convertRSGIS2GSLMatrix(covarianceMatrix);
            
            gsl_matrix *invCovarianceMatrix = gsl_matrix_alloc(covarianceMatrix->m, covarianceMatrix->n);
            gsl_permutation *p = gsl_permutation_alloc(covarianceMatrix->m);
            int signum = 0;
            gsl_linalg_LU_decomp(coVarGSL, p, &signum);
            gsl_linalg_LU_invert (coVarGSL, p, invCovarianceMatrix);
            gsl_permutation_free(p);
            gsl_matrix_free(coVarGSL);
            
            std::cout << "Inverse Covariance Matrix:\n";
            matrixUtils.printGSLMatrix(invCovarianceMatrix);
            
            gsl_vector *dVals = gsl_vector_alloc(numVals);
            gsl_vector *outVec = gsl_vector_alloc(numVals);
            
            rsgis::img::RSGISCalcImgPxl2WindowDist *calcDistWindow = new rsgis::img::RSGISCalcImgPxl2WindowDist(invCovarianceMatrix, varMeans, dVals, outVec);

            calcImage = new rsgis::img::RSGISCalcImage(calcDistWindow, "", true);
            calcImage->calcImageWindowData(datasets, 1, outputImage, winSize, gdalFormat, RSGIS_to_GDAL_Type(outDataType));
            delete calcDistWindow;
            delete calcImage;
            gsl_vector_free(dVals);
            gsl_vector_free(outVec);
            gsl_matrix_free(invCovarianceMatrix);
            matrixUtils.freeMatrix(covarianceMatrix);
            vecUtils.freeVector(varMeans);
            
            GDALClose(datasets[0]);
            delete[] datasets;
            GDALDestroyDriverManager();
        }
        catch(rsgis::RSGISException &e)
        {
            throw RSGISCmdException(e.what());
        }
        catch(std::exception &e)
        {
            throw RSGISCmdException(e.what());
        }
    }
    
    void executeImageCalcDistance(std::string inputImage, std::string outputImage, std::string gdalFormat)throw(RSGISCmdException)
    {
        try
        {
            GDALAllRegister();
            GDALDataset *imgDataset = (GDALDataset *) GDALOpenShared(inputImage.c_str(), GA_ReadOnly);
            if(imgDataset == NULL)
            {
                std::string message = std::string("Could not open image ") + inputImage;
                throw rsgis::RSGISImageException(message.c_str());
            }
            
            // Create blank image
            rsgis::img::RSGISImageUtils imageUtils;
            GDALDataset *outImage = imageUtils.createCopy(imgDataset, 1, outputImage, gdalFormat, GDT_Float32);
            imageUtils.copyFloatGDALDataset(imgDataset, outImage);
            
            double *transform = new double[6];
            outImage->GetGeoTransform(transform);
            
            rsgis::img::RSGISCalcDistViaIterativeGrowth *calcDist = new rsgis::img::RSGISCalcDistViaIterativeGrowth(transform[1]);
            rsgis::img::RSGISCalcEditImage *calcEditImage = new rsgis::img::RSGISCalcEditImage(calcDist);
            
            bool change = true;
            unsigned int dist = 1;
            while(change)
            {
                std::cout << "Distance " << dist << ":\t" << std::flush;
                calcDist->resetChange();
                calcEditImage->calcImageWindowData(outImage, 3, -2);
                change = calcDist->changeOccurred();
                calcDist->incrementCounter();
                dist += 1;
            }
            
            // Translate to distance...
            std::cout << "Multiple by pixel size\n";
            calcEditImage->calcImageUseOut(outImage);
            
            // Clean up memory.
            GDALClose(outImage);
            GDALClose(imgDataset);
            delete[] transform;
        }
        catch(rsgis::RSGISException &e)
        {
            throw RSGISCmdException(e.what());
        }
        catch(std::exception &e)
        {
            throw RSGISCmdException(e.what());
        }
    }
    
    void executeImagePixelColumnSummary(std::string inputImage, std::string outputImage, rsgis::cmds::RSGISCmdStatsSummary summaryStats, std::string gdalFormat, RSGISLibDataType outDataType, float noDataValue, bool useNoDataValue)throw(RSGISCmdException)
    {
        try
        {
            GDALAllRegister();
            GDALDataset *imgDataset = (GDALDataset *) GDALOpenShared(inputImage.c_str(), GA_ReadOnly);
            if(imgDataset == NULL)
            {
                std::string message = std::string("Could not open image ") + inputImage;
                throw rsgis::RSGISImageException(message.c_str());
            }
            
            rsgis::math::RSGISMathsUtils mathUtils;
            rsgis::math::RSGISStatsSummary *mathSummaryStats = new rsgis::math::RSGISStatsSummary();
            mathUtils.initStatsSummary(mathSummaryStats);
            
            mathSummaryStats->calcMin = summaryStats.calcMin;
            mathSummaryStats->calcMax = summaryStats.calcMax;
            mathSummaryStats->calcMean = summaryStats.calcMean;
            mathSummaryStats->calcStdDev = summaryStats.calcStdDev;
            mathSummaryStats->calcSum = summaryStats.calcSum;
            mathSummaryStats->calcMedian = summaryStats.calcMedian;
            
            unsigned int numOutBands = 0;
            if(mathSummaryStats->calcMin)
            {
                ++numOutBands;
            }
            if(mathSummaryStats->calcMax)
            {
                ++numOutBands;
            }
            if(mathSummaryStats->calcMean)
            {
                ++numOutBands;
            }
            if(mathSummaryStats->calcMedian)
            {
                ++numOutBands;
            }
            if(mathSummaryStats->calcSum)
            {
                ++numOutBands;
            }
            if(mathSummaryStats->calcStdDev)
            {
                ++numOutBands;
            }
            
            if(numOutBands == 0)
            {
                throw RSGISException("No summaries where specified and therefore there would be no output image bands.");
            }
            
            std::string *bandNames = new std::string[numOutBands];
            int nameIdx = 0;
            if(mathSummaryStats->calcMin)
            {
                bandNames[nameIdx++] = "Min";
            }
            if(mathSummaryStats->calcMax)
            {
                bandNames[nameIdx++] = "Max";
            }
            if(mathSummaryStats->calcMean)
            {
                bandNames[nameIdx++] = "Mean";
            }
            if(mathSummaryStats->calcMedian)
            {
                bandNames[nameIdx++] = "Median";
            }
            if(mathSummaryStats->calcSum)
            {
                bandNames[nameIdx++] = "Sum";
            }
            if(mathSummaryStats->calcStdDev)
            {
                bandNames[nameIdx++] = "StdDev";
            }
            
            rsgis::img::RSGISImagePixelSummaries *pxlSummary = new rsgis::img::RSGISImagePixelSummaries(numOutBands, mathSummaryStats, noDataValue, useNoDataValue);
            
            rsgis::img::RSGISCalcImage calcImage = rsgis::img::RSGISCalcImage(pxlSummary, "", true);
            calcImage.calcImage(&imgDataset, 1, outputImage, true, bandNames, gdalFormat, RSGIS_to_GDAL_Type(outDataType));
            
            delete[] bandNames;
            delete pxlSummary;
            GDALClose(imgDataset);
        }
        catch(rsgis::RSGISException &e)
        {
            throw RSGISCmdException(e.what());
        }
        catch(std::exception &e)
        {
            throw RSGISCmdException(e.what());
        }
    }
    
    void executeImagePixelLinearFit(std::string inputImage, std::string outputImage, std::string gdalFormat, std::string bandValues, float noDataValue, bool useNoDataValue)throw(RSGISCmdException)
    {
        try
        {
            GDALAllRegister();
            GDALDataset *imgDataset = (GDALDataset *) GDALOpenShared(inputImage.c_str(), GA_ReadOnly);
            if(imgDataset == NULL)
            {
                std::string message = std::string("Could not open image ") + inputImage;
                throw rsgis::RSGISImageException(message.c_str());
            }
            
            rsgis::utils::RSGISTextUtils textUtils;
            std::vector<std::string> strValues = textUtils.readFileToStringVector(bandValues);
            std::vector<float> bandXValues;
            for(std::vector<std::string>::iterator iterStrVals = strValues.begin(); iterStrVals != strValues.end(); ++iterStrVals)
            {
                if((*iterStrVals) != "")
                {
                    try
                    {
                        bandXValues.push_back(textUtils.strtofloat(*iterStrVals));
                    }
                    catch (rsgis::RSGISException &e)
                    {
                        // ignore.
                        std::cout << "Warning \'" << *iterStrVals << "\' could not be converted to an float.\n";
                    }
                }
            }
            
            if(bandXValues.size() != imgDataset->GetRasterCount())
            {
                std::cout << "bandXValues.size() = " << bandXValues.size() << std::endl;
                std::cout << "imgDataset->GetRasterCount() = " << imgDataset->GetRasterCount() << std::endl;
                GDALClose(imgDataset);
                throw RSGISException("The number of image bands and x values are not the same.");
            }
            
            std::string *bandNames = new std::string[3];
            bandNames[0] = "Intercept";
            bandNames[1] = "Slope";
            bandNames[2] = "SumSq";
            
            rsgis::img::RSGISLinearFit2Column *linearFit = new rsgis::img::RSGISLinearFit2Column(bandXValues, noDataValue, useNoDataValue);
            
            rsgis::img::RSGISCalcImage calcImage = rsgis::img::RSGISCalcImage(linearFit, "", true);
            calcImage.calcImage(&imgDataset, 1, outputImage, true, bandNames, gdalFormat, GDT_Float32);
            
            delete[] bandNames;
            delete linearFit;
            
            
            GDALClose(imgDataset);
        }
        catch(rsgis::RSGISException &e)
        {
            throw RSGISCmdException(e.what());
        }
        catch(std::exception &e)
        {
            throw RSGISCmdException(e.what());
        }
    }
    
    void executeNormalisation(std::vector<std::string> inputImages, std::vector<std::string> outputImages, bool calcInMinMax, double inMin, double inMax, double outMin, double outMax)throw(RSGISCmdException)
    {
        GDALAllRegister();
        GDALDataset *dataset = NULL;
        rsgis::img::RSGISImageNormalisation *normImage = NULL;
        double *imageMaxBands = NULL;
        double *imageMinBands = NULL;
        double *outMinBands = NULL;
        double *outMaxBands = NULL;
        
        int rasterCount = 0;
        
        std::vector<std::string>::iterator inImage;
        std::vector<std::string>::iterator outImage;
        
        try
        {
            normImage = new rsgis::img::RSGISImageNormalisation();
            
            for(inImage = inputImages.begin(), outImage = outputImages.begin(); inImage != inputImages.end(); ++inImage, ++outImage)
            {
                std::cout << *inImage << std::endl;
                dataset = (GDALDataset *) GDALOpen((*inImage).c_str(), GA_ReadOnly);
                if(dataset == NULL)
                {
                    std::string message = std::string("Could not open image ") + *inImage;
                    throw rsgis::RSGISImageException(message.c_str());
                }
                
                rasterCount = dataset->GetRasterCount();
                
                imageMaxBands = new double[rasterCount];
                imageMinBands = new double[rasterCount];
                outMinBands = new double[rasterCount];
                outMaxBands = new double[rasterCount];
                
                for(int j = 0; j < rasterCount; j++)
                {
                    if(calcInMinMax)
                    {
                        imageMaxBands[j] = 0;
                        imageMinBands[j] = 0;
                    }
                    else
                    {
                        imageMaxBands[j] = inMax;
                        imageMinBands[j] = inMin;
                    }
                    outMaxBands[j] = outMax;
                    outMinBands[j] = outMin;
                }
                
                normImage->normaliseImage(dataset, imageMaxBands, imageMinBands, outMaxBands, outMinBands, calcInMinMax, *outImage);
                
                GDALClose(dataset);
                delete[] imageMinBands;
                delete[] imageMaxBands;
                delete[] outMinBands;
                delete[] outMaxBands;
            }	
        }
        catch(rsgis::RSGISException e)
        {
            throw RSGISCmdException(e.what());
        }
        
        if(normImage != NULL)
        {
            delete normImage;
        }			

    }
    
    void executeCorrelation(std::string inputImageA, std::string inputImageB, std::string outputMatrix) throw(RSGISCmdException)
    {
        
        GDALAllRegister();
        GDALDataset **datasetsA = NULL;
        GDALDataset **datasetsB = NULL;
        
        rsgis::math::RSGISMatrices matrixUtils;
        
        rsgis::img::RSGISCalcImageSingle *calcImgSingle = NULL;
        rsgis::img::RSGISCalcCC *calcCC = NULL;
        rsgis::img::RSGISCalcImageMatrix *calcImgMatrix = NULL;
        
        rsgis::math::Matrix *correlationMatrix = NULL;
        
        try
        {
            datasetsA = new GDALDataset*[1];
            std::cout << inputImageA << std::endl;
            datasetsA[0] = (GDALDataset *) GDALOpenShared(inputImageA.c_str(), GA_ReadOnly);
            if(datasetsA[0] == NULL)
            {
                std::string message = std::string("Could not open image ") + inputImageA;
                throw rsgis::RSGISImageException(message.c_str());
            }
            
            
            datasetsB = new GDALDataset*[1];
            std::cout << inputImageB << std::endl;
            datasetsB[0] = (GDALDataset *) GDALOpenShared(inputImageB.c_str(), GA_ReadOnly);
            if(datasetsB[0] == NULL)
            {
                std::string message = std::string("Could not open image ") + inputImageB;
                throw rsgis::RSGISImageException(message.c_str());
            }
            
            calcCC = new rsgis::img::RSGISCalcCC(1);
            calcImgSingle = new rsgis::img::RSGISCalcImageSingle(calcCC);
            calcImgMatrix = new rsgis::img::RSGISCalcImageMatrix(calcImgSingle);
            correlationMatrix = calcImgMatrix->calcImageMatrix(datasetsA, datasetsB, 1);
            matrixUtils.saveMatrix2txt(correlationMatrix, outputMatrix);
            
            delete calcCC;
            delete calcImgMatrix;
            delete calcImgSingle;
            
            if(datasetsA[0] != NULL) {
                GDALClose(datasetsA[0]);
            }
            if(datasetsB[0] != NULL) {
                GDALClose(datasetsB[0]);
            }
            
            delete [] datasetsA;
            delete [] datasetsB;
        }
        catch(rsgis::RSGISException e)
        {
            throw RSGISCmdException(e.what());
        }
    }
    
    void executeCovariance(std::string inputImageA, std::string inputImageB, std::string inputMatrixA, std::string inputMatrixB, bool shouldCalcMean, std::string outputMatrix)throw(RSGISCmdException)
    {
        GDALAllRegister();
        GDALDataset **datasetsA = NULL;
        GDALDataset **datasetsB = NULL;
        
        rsgis::math::RSGISMatrices matrixUtils;
        
        rsgis::img::RSGISCalcImageSingle *calcImgSingle = NULL;
        rsgis::img::RSGISCalcCovariance *calcCovar = NULL;
        rsgis::img::RSGISCalcImageMatrix *calcImgMatrix = NULL;
        
        rsgis::img::RSGISCalcImageSingle *calcImgSingleMean = NULL;
        rsgis::img::RSGISCalcMeanVectorIndividual *calcMean = NULL;
        rsgis::img::RSGISCalcImageMatrix *calcImgMatrixMean = NULL;
        
        rsgis::math::Matrix *meanAMatrix = NULL;
        rsgis::math::Matrix *meanBMatrix = NULL;
        rsgis::math::Matrix *covarianceMatrix = NULL;
        
        try
        {
            datasetsA = new GDALDataset*[1];
            std::cout << inputImageA << std::endl;
            datasetsA[0] = (GDALDataset *) GDALOpenShared(inputImageA.c_str(), GA_ReadOnly);
            if(datasetsA[0] == NULL)
            {
                std::string message = std::string("Could not open image ") + inputImageA;
                throw rsgis::RSGISImageException(message.c_str());
            }
            
            datasetsB = new GDALDataset*[1];
            std::cout << inputImageB << std::endl;
            datasetsB[0] = (GDALDataset *) GDALOpenShared(inputImageB.c_str(), GA_ReadOnly);
            if(datasetsB[0] == NULL)
            {
                std::string message = std::string("Could not open image ") + inputImageB;
                throw rsgis::RSGISImageException(message.c_str());
            }
            
            
            if(shouldCalcMean)
            {
                std::cout << "Mean vectors will be calculated\n";
                calcMean = new rsgis::img::RSGISCalcMeanVectorIndividual(1);
                calcImgSingleMean = new rsgis::img::RSGISCalcImageSingle(calcMean);
                calcImgMatrixMean = new rsgis::img::RSGISCalcImageMatrix(calcImgSingle);
                meanAMatrix = calcImgMatrixMean->calcImageVector(datasetsA, 1);
                meanBMatrix = calcImgMatrixMean->calcImageVector(datasetsB, 1);
                std::cout << "Mean Vectors have been calculated\n";
            }
            else
            {
                meanAMatrix = matrixUtils.readMatrixFromTxt(inputMatrixA);
                meanBMatrix = matrixUtils.readMatrixFromTxt(inputMatrixB);
            }
            
            calcCovar = new rsgis::img::RSGISCalcCovariance(1, meanAMatrix, meanBMatrix);
            calcImgSingle = new rsgis::img::RSGISCalcImageSingle(calcCovar);
            calcImgMatrix = new rsgis::img::RSGISCalcImageMatrix(calcImgSingle);
            covarianceMatrix = calcImgMatrix->calcImageMatrix(datasetsA, datasetsB, 1);
            matrixUtils.saveMatrix2txt(covarianceMatrix, outputMatrix);
        }
        catch(rsgis::RSGISException e)
        {
            throw e;
        }
        
        if(calcImgSingle != NULL)
        {
            delete calcImgSingle;
        }
        if(calcCovar != NULL)
        {
            delete calcCovar;
        }
        if(calcImgMatrix != NULL)
        {
            delete calcImgMatrix;
        }
        
        if(calcImgSingleMean != NULL)
        {
            delete calcImgSingleMean;
        }
        if(calcMean != NULL)
        {
            delete calcMean;
        }
        if(calcImgMatrixMean != NULL)
        {
            delete calcImgMatrixMean;
        }
        
        if(meanAMatrix != NULL)
        {
            matrixUtils.freeMatrix(meanAMatrix);
        }
        if(meanBMatrix != NULL)
        {
            matrixUtils.freeMatrix(meanBMatrix);
        }
        if(covarianceMatrix != NULL)
        {
            matrixUtils.freeMatrix(covarianceMatrix);
        }
        
        if(datasetsA[0] != NULL) {
            GDALClose(datasetsA[0]);
        }
        if(datasetsB[0] != NULL) {
            GDALClose(datasetsB[0]);
        }
        
        delete [] datasetsA;
        delete [] datasetsB;

    }
    
    void executeMeanVector(std::string inputImage, std::string outputMatrix)throw(RSGISCmdException)
    {
        GDALAllRegister();
        GDALDataset **datasets = NULL;
        
        rsgis::img::RSGISCalcImageSingle *calcImgSingle = NULL;
        rsgis::img::RSGISCalcMeanVectorIndividual *calcMean = NULL;
        rsgis::img::RSGISCalcImageMatrix *calcImgMatrix = NULL;
        
        rsgis::math::RSGISMatrices matrixUtils;
        rsgis::math::Matrix *meanVectorMatrix = NULL;
        
        try
        {
            datasets = new GDALDataset*[1];
            std::cout << inputImage << std::endl;
            datasets[0] = (GDALDataset *) GDALOpenShared(inputImage.c_str(), GA_ReadOnly);
            if(datasets[0] == NULL)
            {
                std::string message = std::string("Could not open image ") + inputImage;
                throw RSGISCmdException(message.c_str());
            }
            
            calcMean = new rsgis::img::RSGISCalcMeanVectorIndividual(1);
            calcImgSingle = new rsgis::img::RSGISCalcImageSingle(calcMean);
            calcImgMatrix = new rsgis::img::RSGISCalcImageMatrix(calcImgSingle);
            meanVectorMatrix = calcImgMatrix->calcImageVector(datasets, 1);
            matrixUtils.saveMatrix2txt(meanVectorMatrix, outputMatrix);
        }
        catch(rsgis::RSGISException e)
        {
            throw RSGISCmdException(e.what());
        }
        
        if(calcImgSingle != NULL)
        {
            delete calcImgSingle;
        }
        if(calcMean != NULL)
        {
            delete calcMean;
        }
        if(calcImgMatrix != NULL)
        {
            delete calcImgMatrix;
        }
        
        if(meanVectorMatrix != NULL)
        {
            matrixUtils.freeMatrix(meanVectorMatrix);
        }
        
        if(datasets[0] != NULL) {
            GDALClose(datasets[0]);
        }
        
        delete [] datasets;

    }
    
    void executePCA(std::string eigenvectors, std::string inputImage, std::string outputImage, int numComponents)throw(RSGISCmdException)
    {
        GDALAllRegister();
        GDALDataset **datasets = NULL;
        
        rsgis::math::RSGISMatrices matrixUtils;
        rsgis::img::RSGISCalcImage *calcImage = NULL;
        rsgis::img::RSGISApplyEigenvectors *applyPCA = NULL;
        rsgis::math::Matrix *eigenvectorsMatrix = NULL;
        
        try
        {
            std::cout << "Reading in from file " << eigenvectors << std::endl;
            eigenvectorsMatrix = matrixUtils.readMatrixFromTxt(eigenvectors);
            std::cout << "Finished reading in matrix\n";
            
            datasets = new GDALDataset*[1];
            std::cout << "Reading in image " << inputImage << std::endl;
            datasets[0] = (GDALDataset *) GDALOpenShared(inputImage.c_str(), GA_ReadOnly);
            if(datasets[0] == NULL)
            {
                std::string message = std::string("Could not open image ") + inputImage;
                throw rsgis::RSGISImageException(message.c_str());
            }
            
            applyPCA = new rsgis::img::RSGISApplyEigenvectors(numComponents, eigenvectorsMatrix);
            calcImage = new rsgis::img::RSGISCalcImage(applyPCA, "", true);
            calcImage->calcImage(datasets, 1, outputImage);
            
            if(datasets[0] != NULL) {
                GDALClose(datasets[0]);
            }
            
        }
        catch(rsgis::RSGISException e)
        {
            throw e;
        }
        
        if(calcImage != NULL)
        {
            delete calcImage;
        }
        if(applyPCA != NULL)
        {
            delete applyPCA;
        }
        
        if(eigenvectorsMatrix != NULL)
        {
            matrixUtils.freeMatrix(eigenvectorsMatrix);
        }
    
        delete [] datasets;
    }
    
    void executeStandardise(std::string meanvectorStr, std::string inputImage, std::string outputImage)throw(RSGISCmdException)
    {
        GDALAllRegister();
        GDALDataset **datasets = NULL;
        
        rsgis::math::RSGISMatrices matrixUtils;
        rsgis::img::RSGISCalcImage *calcImage = NULL;
        rsgis::img::RSGISStandardiseImage *stdImg = NULL;
        rsgis::math::Matrix *meanVectorMatrix = NULL;
        
        try
        {
            std::cout << "Reading in from file " << meanvectorStr << std::endl;
            meanVectorMatrix = matrixUtils.readMatrixFromTxt(meanvectorStr);
            std::cout << "Finished reading in matrix\n";
            
            datasets = new GDALDataset*[1];
            std::cout << "Reading in image " << inputImage << std::endl;
            datasets[0] = (GDALDataset *) GDALOpenShared(inputImage.c_str(), GA_ReadOnly);
            if(datasets[0] == NULL)
            {
                std::string message = std::string("Could not open image ") + inputImage;
                throw rsgis::RSGISImageException(message.c_str());
            }
            
            stdImg = new rsgis::img::RSGISStandardiseImage(datasets[0]->GetRasterCount(), meanVectorMatrix);
            calcImage = new rsgis::img::RSGISCalcImage(stdImg, "", true);
            calcImage->calcImage(datasets, 1, outputImage);
            
            delete calcImage;
            delete stdImg;
            matrixUtils.freeMatrix(meanVectorMatrix);
            GDALClose(datasets[0]);
            delete [] datasets;
            
        }
        catch(rsgis::RSGISException e)
        {
            throw e;
        }
    }
    
    void executeReplaceValuesLessThan(std::string inputImage, std::string outputImage, double threshold, double value)throw(RSGISCmdException)
    {
        GDALAllRegister();
        GDALDataset **datasets = NULL;
        rsgis::img::RSGISCalcImageValue *calcImageValue = NULL;
        rsgis::img::RSGISCalcImage *calcImage = NULL;
        
        try
        {
            datasets = new GDALDataset*[1];
            
            datasets[0] = (GDALDataset *) GDALOpen(inputImage.c_str(), GA_ReadOnly);
            if(datasets[0] == NULL)
            {
                std::string message = std::string("Could not open image ") + inputImage;
                throw rsgis::RSGISImageException(message.c_str());
            }
            
            int numImgBands = datasets[0]->GetRasterCount();
            
            
            calcImageValue = new rsgis::img::RSGISReplaceValuesLessThanGivenValue(numImgBands, threshold, value);
            
            calcImage = new rsgis::img::RSGISCalcImage(calcImageValue, "", true);
            calcImage->calcImage(datasets, 1, outputImage);
            
            
            GDALClose(datasets[0]);
            
            delete calcImageValue;
            delete calcImage;
            delete [] datasets;
        }
        catch(rsgis::RSGISException &e)
        {
            throw RSGISCmdException(e.what());
        }

    }
    
    void executeUnitArea(std::string inputImage, std::string outputImage, std::string inMatrixfile)throw(RSGISCmdException)
    {
        GDALAllRegister();
        GDALDataset **datasets = NULL;
        rsgis::img::RSGISCalcImageValue *calcImageValue = NULL;
        rsgis::img::RSGISCalcImage *calcImage = NULL;
        rsgis::math::RSGISMatrices matrixUtils;
        
        try
        {
            datasets = new GDALDataset*[1];
            
            datasets[0] = (GDALDataset *) GDALOpen(inputImage.c_str(), GA_ReadOnly);
            if(datasets[0] == NULL)
            {
                std::string message = std::string("Could not open image ") + inputImage;
                throw rsgis::RSGISImageException(message.c_str());
            }
            
            int numImgBands = datasets[0]->GetRasterCount();
            
            rsgis::math::Matrix *bandsValuesMatrix = matrixUtils.readMatrixFromTxt(inMatrixfile);
            
            if(bandsValuesMatrix->n != numImgBands)
            {
                GDALClose(datasets[0]);
                matrixUtils.freeMatrix(bandsValuesMatrix);
                
                throw rsgis::RSGISException("The bandvalues matrix needs to have the same number of rows as the input image has bands");
            }
            
            if(bandsValuesMatrix->m != 2)
            {
                GDALClose(datasets[0]);
                matrixUtils.freeMatrix(bandsValuesMatrix);
                delete [] datasets;
                
                throw rsgis::RSGISException("The bandvalues matrix needs to have 2 columns (Wavelength, Width)");
            }
            
            calcImageValue = new rsgis::img::RSGISConvertSpectralToUnitArea(numImgBands, bandsValuesMatrix);
            
            calcImage = new rsgis::img::RSGISCalcImage(calcImageValue, "", true);
            calcImage->calcImage(datasets, 1, outputImage);
            
            
            GDALClose(datasets[0]);
            
            matrixUtils.freeMatrix(bandsValuesMatrix);
            
            delete calcImageValue;
            delete calcImage;
            delete [] datasets;
        }
        catch(rsgis::RSGISException &e)
        {
            throw RSGISCmdException(e.what());
        }

    }
    
    void executeMovementSpeed(std::vector<std::string> inputImages, std::vector<unsigned int> imageBands, std::vector<float> imageTimes, float upper, float lower, std::string outputImage)throw(RSGISCmdException)
    {
        GDALAllRegister();
        GDALDataset **datasets = NULL;
        rsgis::img::RSGISCalcImage *calcImage = NULL;
        rsgis::img::RSGISCalculateImageMovementSpeed *calcImageValue = NULL;

        unsigned int numImages = inputImages.size();
        
        // check that we have bands and times for each image
        if(imageBands.size() != numImages || imageTimes.size() != numImages) {
            throw RSGISCmdException("Bands and Times were not supplied for all images");
        }
        
        try
        {
            datasets = new GDALDataset*[numImages];
            
            unsigned int numRasterBands = 0;
            unsigned int totalNumRasterBands = 0;
            unsigned int *imgBandsInStack = new unsigned int[numImages];
            
            for(int i = 0; i < numImages; ++i)
            {
                datasets[i] = (GDALDataset *) GDALOpen(inputImages[i].c_str(), GA_ReadOnly);
                if(datasets[i] == NULL)
                {
                    std::string message = std::string("Could not open image ") + inputImages[i];
                    throw rsgis::RSGISImageException(message.c_str());
                }
                
                numRasterBands = datasets[i]->GetRasterCount();
                imgBandsInStack[i] = totalNumRasterBands + imageBands[i];
                
                std::cout << "Opened Image: " << inputImages[i] << " will be using band " << imgBandsInStack[i] << " in stack." << std::endl;
                
                if(imageBands[i] > (numRasterBands-1))
                {
                    throw rsgis::RSGISImageException("You have specified a band which is not within the image");
                }
                
                totalNumRasterBands += numRasterBands;
            }
            
            int numOutputBands = ((numImages-1)*2) + 3;
            
            rsgis::math::RSGISMathsUtils mathUtils;
            std::string *outBandNames = new std::string[numOutputBands];
            outBandNames[0] = std::string("Mean Movement Speed");
            outBandNames[1] = std::string("Min Movement Speed");
            outBandNames[2] = std::string("Max Movement Speed");
            
            int idx = 0;
            for(int i = 0; i < numImages-1; ++i)
            {
                idx = (i * 2) + 3;
                outBandNames[idx] = std::string("Images ") + mathUtils.inttostring(i+1) + std::string("-") + mathUtils.inttostring(i+2) + std::string(" Displacement");
                outBandNames[idx+1] = std::string("Images ") + mathUtils.inttostring(i+1) + std::string("-") + mathUtils.inttostring(i+2) + std::string(" Movement");
            }
            
            std::cout << "Number of Output Image bands = " << numOutputBands << std::endl;
            
            float *timesArrayPointer = &imageTimes[0];  // WARNING WARNING - this may be bonkers, on the other hand, it may be a nice shorthand for passing a vector as array
            
            calcImageValue = new rsgis::img::RSGISCalculateImageMovementSpeed(numOutputBands, numImages, imgBandsInStack, timesArrayPointer, upper, lower);
            calcImage = new rsgis::img::RSGISCalcImage(calcImageValue, "", true);
            calcImage->calcImage(datasets, numImages, outputImage, true, outBandNames);
            
            for(int i = 0; i < numImages; ++i)
            {
                GDALClose(datasets[i]);
            }
            
            delete[] datasets;
            delete[] outBandNames;
            
            delete calcImage;
            delete calcImageValue;
            delete[] imgBandsInStack;

        }
        catch(rsgis::RSGISException e)
        {
            throw RSGISCmdException(e.what());
        }

    }
    
    void executeCountValsInCols(std::string inputImage, float upper, float lower, std::string outputImage)throw(RSGISCmdException)
    {
        GDALAllRegister();
        GDALDataset **datasets = NULL;
        rsgis::img::RSGISCalcImage *calcImage = NULL;
        rsgis::img::RSGISCountValsAboveThresInCol *calcImageValue = NULL;
        
        try
        {
            datasets = new GDALDataset*[1];
            datasets[0] = (GDALDataset *) GDALOpen(inputImage.c_str(), GA_ReadOnly);
            if(datasets[0] == NULL)
            {
                std::string message = std::string("Could not open image ") + inputImage;
                throw rsgis::RSGISImageException(message.c_str());
            }
            
            calcImageValue = new rsgis::img::RSGISCountValsAboveThresInCol(1, upper, lower);
            calcImage = new rsgis::img::RSGISCalcImage(calcImageValue, "", true);
            calcImage->calcImage(datasets, 1, outputImage);
            
            GDALClose(datasets[0]);
            delete[] datasets;
            
            delete calcImage;
            delete calcImageValue;
        }
        catch(rsgis::RSGISException e)
        {
            throw RSGISCmdException(e.what());
        }
    }
    
    void executeCalculateRMSE(std::string inputImageA, int inputBandA, std::string inputImageB, int inputBandB)throw(RSGISCmdException)
    {
        GDALAllRegister();
        GDALDataset **datasetsA = NULL;
        GDALDataset **datasetsB = NULL;
        
        rsgis::img::RSGISCalcImageSingle *calcImgSingle = NULL;
        rsgis::img::RSGISCalcRMSE *calculateRSME = NULL;
        double *outRMSE;
        outRMSE = new double[1];
        
        std::cout << "Calculating RMSE between: " << inputImageA << " (Band " << inputBandA + 1 << ") and " << inputImageB << " (Band " << inputBandB + 1 << ")" << std::endl;
        
        try {
            
            datasetsA = new GDALDataset*[1];
            std::cout << inputImageA << std::endl;
            datasetsA[0] = (GDALDataset *) GDALOpenShared(inputImageA.c_str(), GA_ReadOnly);
            
            if(datasetsA[0] == NULL) {
                std::string message = std::string("Could not open image ") + inputImageA;
                throw rsgis::RSGISImageException(message.c_str());
            }
            
            datasetsB = new GDALDataset*[1];
            std::cout << inputImageB << std::endl;
            datasetsB[0] = (GDALDataset *) GDALOpenShared(inputImageB.c_str(), GA_ReadOnly);
            
            if(datasetsB[0] == NULL) {
                std::string message = std::string("Could not open image ") + inputImageB;
                throw rsgis::RSGISImageException(message.c_str());
            }
            
            calculateRSME = new rsgis::img::RSGISCalcRMSE(1);
            calcImgSingle = new rsgis::img::RSGISCalcImageSingle(calculateRSME);
            calcImgSingle->calcImage(datasetsA, datasetsB, 1, outRMSE, inputBandA, inputBandB);
            
            std::cout << "RMSE = " << outRMSE[0] << std::endl;
            
            delete calculateRSME;
            delete calcImgSingle;
            delete[] outRMSE;
            
            if(datasetsA != NULL) {
                GDALClose(datasetsA [0]);
                delete[] datasetsA;
            }
            
            if(datasetsB != NULL) {
                GDALClose(datasetsB[0]);
                delete[] datasetsB;
            }
            
        } catch(rsgis::RSGISException e) {
            throw RSGISCmdException(e.what());
        }
    }
    
    void executeApply2VarFunction(std::string inputImage, void *twoVarFunction, std::string outputImage)throw(RSGISCmdException)
    {
        GDALAllRegister();
        GDALDataset **datasets = NULL;
        try
        {
            datasets = new GDALDataset*[1];
            std::cout << inputImage << std::endl;
            datasets[0] = (GDALDataset *) GDALOpenShared(inputImage.c_str(), GA_ReadOnly);
            if(datasets[0] == NULL)
            {
                std::string message = std::string("Could not open image ") + inputImage;
                throw rsgis::RSGISImageException(message.c_str());
            }
        }
        catch(rsgis::RSGISException& e)
        {
            throw e;
        }
        rsgis::img::RSGISCalcImage *calcImg = NULL;
        rsgis::img::RSGISApplyTwoVarFunction *applyFunction = NULL;
        
        try
        {
            std::cout << "Applying two variable function" <<std::endl;
            rsgis::math::RSGISMathTwoVariableFunction *twoVarFuncPointer = (rsgis::math::RSGISMathTwoVariableFunction*) twoVarFunction;
            applyFunction = new rsgis::img::RSGISApplyTwoVarFunction(1, twoVarFuncPointer);
            calcImg = new rsgis::img::RSGISCalcImage(applyFunction, "", true);
            calcImg->calcImage(datasets, 1, outputImage);
        }
        catch(rsgis::RSGISException& e)
        {
            throw e;
        }
        if(datasets != NULL)
        {
            GDALClose(datasets[0]);
            delete[] datasets;
        }
        
        delete calcImg;
        delete applyFunction;
    }
    
    void executeApply3VarFunction(std::string inputImage, void *threeVarFunction, std::string outputImage)throw(RSGISCmdException)
    {
        GDALAllRegister();
        GDALDataset **datasets = NULL;
        try
        {
            datasets = new GDALDataset*[1];
            std::cout << inputImage << std::endl;
            datasets[0] = (GDALDataset *) GDALOpenShared(inputImage.c_str(), GA_ReadOnly);
            if(datasets[0] == NULL)
            {
                std::string message = std::string("Could not open image ") + inputImage;
                throw rsgis::RSGISImageException(message.c_str());
            }
        }
        catch(rsgis::RSGISException& e)
        {
            throw e;
        }
        rsgis::img::RSGISCalcImage *calcImg = NULL;
        rsgis::img::RSGISApplyThreeVarFunction *applyFunction = NULL;
        try
        {
            std::cout << "Applying three variable function" <<std::endl;
            rsgis::math::RSGISMathThreeVariableFunction *threeVarFuncPointer = (rsgis::math::RSGISMathThreeVariableFunction*) threeVarFunction;
            applyFunction = new rsgis::img::RSGISApplyThreeVarFunction(1, threeVarFuncPointer);
            calcImg = new rsgis::img::RSGISCalcImage(applyFunction, "", true);
            calcImg->calcImage(datasets, 1, outputImage);
        }
        catch(rsgis::RSGISException& e)
        {
            throw RSGISCmdException(e.what());
        }
        if(datasets != NULL)
        {
            GDALClose(datasets[0]);
            delete[] datasets;
        }
        
        delete calcImg;
        delete applyFunction;

    }
    
    void executeDist2Geoms(std::string inputVector, float imgResolution, std::string outputImage)throw(RSGISCmdException)
    {
        OGRRegisterAll();
        
        rsgis::utils::RSGISFileUtils fileUtils;
        rsgis::vec::RSGISVectorUtils vecUtils;
        
        std::string SHPFileInLayer = vecUtils.getLayerName(inputVector);
        
        OGRSpatialReference* inputSpatialRef = NULL;
        OGRDataSource *inputSHPDS = NULL;
        OGRLayer *inputSHPLayer = NULL;
        try
        {
            
            /////////////////////////////////////
            //
            // Open Input Shapfile.
            //
            /////////////////////////////////////
            inputSHPDS = OGRSFDriverRegistrar::Open(inputVector.c_str(), FALSE);
            if(inputSHPDS == NULL)
            {
                std::string message = std::string("Could not open vector file ") + inputVector;
                throw rsgis::RSGISFileException(message.c_str());
            }
            inputSHPLayer = inputSHPDS->GetLayerByName(SHPFileInLayer.c_str());
            if(inputSHPLayer == NULL)
            {
                std::string message = std::string("Could not open vector layer ") + SHPFileInLayer;
                throw rsgis::RSGISFileException(message.c_str());
            }
            inputSpatialRef = inputSHPLayer->GetSpatialRef();
            char **wktString = new char*[1];    // TODO: check deletion
            inputSpatialRef->exportToWkt(wktString);
            std::string projection = std::string(wktString[0]);
            OGRFree(wktString);
            OGREnvelope ogrExtent;
            inputSHPLayer->GetExtent(&ogrExtent);
            geos::geom::Envelope extent = geos::geom::Envelope(ogrExtent.MinX, ogrExtent.MaxX, ogrExtent.MinY, ogrExtent.MaxY);
            
            // Get Geometries into memory
            std::vector<OGRGeometry*> *ogrGeoms = new std::vector<OGRGeometry*>();
            rsgis::vec::RSGISGetOGRGeometries *getOGRGeoms = new rsgis::vec::RSGISGetOGRGeometries(ogrGeoms);
            rsgis::vec::RSGISProcessVector processVector = rsgis::vec::RSGISProcessVector(getOGRGeoms);
            processVector.processVectorsNoOutput(inputSHPLayer, false);
            delete getOGRGeoms;
            
            // Create Geometry Collection
            OGRGeometryCollection *geomCollectionOrig = new OGRGeometryCollection();
            for(std::vector<OGRGeometry*>::iterator iterGeoms = ogrGeoms->begin(); iterGeoms != ogrGeoms->end(); ++iterGeoms)
            {
                geomCollectionOrig->addGeometryDirectly(*iterGeoms);
            }
            
            OGRGeometryCollection *geomCollectionLines = new OGRGeometryCollection();
            rsgis::geom::RSGISGeometry geomUtils;
            geomUtils.convertGeometryCollection2Lines(geomCollectionOrig, geomCollectionLines);
            
            // Create blank image
            rsgis::img::RSGISImageUtils imageUtils;
            GDALDataset *outImage = imageUtils.createBlankImage(outputImage, extent, imgResolution, 1, projection, 0);
            
            rsgis::img::RSGISCalcDist2Geom *dist2GeomCalcValue = new rsgis::img::RSGISCalcDist2Geom(1, geomCollectionLines, geomCollectionOrig);
            rsgis::img::RSGISCalcEditImage *calcEditImage = new rsgis::img::RSGISCalcEditImage(dist2GeomCalcValue);
            calcEditImage->calcImage(outImage);
            
            // Clean up memory.
            delete geomCollectionOrig;
            delete geomCollectionLines;
            delete ogrGeoms;
            OGRDataSource::DestroyDataSource(inputSHPDS);
            OGRCleanupAll();
            GDALClose(outImage);
        }
        catch (rsgis::RSGISException e) 
        {
            throw RSGISCmdException(e.what());
        }
    }
    
    void executeImageBandStats(std::string inputImage, std::string outputFile, bool ignoreZeros)throw(RSGISCmdException)
    {
        GDALAllRegister();
        GDALDataset **datasets = NULL;
        
        rsgis::img::RSGISImageStatistics calcImgStats;
        
        try
        {
            datasets = new GDALDataset*[1];
            datasets[0] = (GDALDataset *) GDALOpenShared(inputImage.c_str(), GA_ReadOnly);
            if(datasets[0] == NULL)
            {
                std::string message = std::string("Could not open image ") + inputImage;
                throw rsgis::RSGISImageException(message.c_str());
            }
            
            int numImageBands = datasets[0]->GetRasterCount();
            rsgis::img::ImageStats **stats = new rsgis::img::ImageStats*[numImageBands];
            for(int i = 0; i < numImageBands; ++i)
            {
                stats[i] = new rsgis::img::ImageStats;
                stats[i]->min = 0;
                stats[i]->max = 0;
                stats[i]->mean = 0;
                stats[i]->sum = 0;
                stats[i]->stddev = 0;
            }
            
            calcImgStats.calcImageStatistics(datasets, 1, stats, numImageBands, true, ignoreZeros);
            
            std::ofstream outTxtFile;
            outTxtFile.open(outputFile.c_str());
            outTxtFile.precision(15);
            outTxtFile << "Min,Max,Mean,StdDev,Sum\n";
            for(int i = 0; i < numImageBands; ++i)
            {
                outTxtFile << stats[i]->min << "," << stats[i]->max << "," << stats[i]->mean << "," << stats[i]->stddev << "," << stats[i]->sum << std::endl;
            }
            outTxtFile.flush();
            outTxtFile.close();
            
            GDALClose(datasets[0]);
            delete[] datasets;
        }
        catch(rsgis::RSGISException e)
        {
            throw RSGISCmdException(e.what());
        }

    }
    
    void executeImageStats(std::string inputImage, std::string outputFile, bool ignoreZeros)throw(RSGISCmdException)
    {
        GDALAllRegister();
        GDALDataset **datasets = NULL;
        
        rsgis::img::RSGISImageStatistics calcImgStats;
        
        try
        {
            datasets = new GDALDataset*[1];
            datasets[0] = (GDALDataset *) GDALOpenShared(inputImage.c_str(), GA_ReadOnly);
            if(datasets[0] == NULL)
            {
                std::string message = std::string("Could not open image ") + inputImage;
                throw rsgis::RSGISImageException(message.c_str());
            }
            
            rsgis::img::ImageStats *stats = new rsgis::img::ImageStats();
            stats->min = 0;
            stats->max = 0;
            stats->mean = 0;
            stats->sum = 0;
            stats->stddev = 0;
            
            calcImgStats.calcImageStatistics(datasets, 1, stats, true, ignoreZeros);
            
            std::ofstream outTxtFile;
            outTxtFile.open(outputFile.c_str());
            outTxtFile.precision(15);
            outTxtFile << "Min,Max,Mean,StdDev,Sum\n";
            outTxtFile << stats->min << "," << stats->max << "," << stats->mean << "," << stats->stddev << "," << stats->sum << std::endl;
            outTxtFile.flush();
            outTxtFile.close();
            
            GDALClose(datasets[0]);
            delete[] datasets;
            
        }
        catch(rsgis::RSGISException e)
        {
            throw RSGISCmdException(e.what());
        }

    }
    
    void executeUnconLinearSpecUnmix(std::string inputImage, std::string imageFormat, RSGISLibDataType outDataType, float lsumGain, float lsumOffset, std::string outputFile, std::string endmembersFile)throw(RSGISCmdException)
    {
        GDALAllRegister();
        GDALDataset **datasets = NULL;
        
        try
        {
            datasets = new GDALDataset*[1];
            datasets[0] = (GDALDataset *) GDALOpenShared(inputImage.c_str(), GA_ReadOnly);
            if(datasets[0] == NULL)
            {
                std::string message = std::string("Could not open image ") + inputImage;
                    throw rsgis::RSGISImageException(message.c_str());
            }
            
            rsgis::img::RSGISCalcLinearSpectralUnmixing calcSpecUnmix(imageFormat, RSGIS_to_GDAL_Type(outDataType), lsumGain, lsumOffset);
            calcSpecUnmix.performUnconstainedLinearSpectralUnmixing(datasets, 1, outputFile, endmembersFile);
            
            GDALClose(datasets[0]);
            delete[] datasets;
        }
        catch(rsgis::RSGISException e)
        {
            throw RSGISCmdException(e.what());
        }

    }
    
    void executeExhconLinearSpecUnmix(std::string inputImage, std::string imageFormat, RSGISLibDataType outDataType, float lsumGain, float lsumOffset, std::string outputFile, std::string endmembersFile, float stepResolution)throw(RSGISCmdException)
    {
        GDALAllRegister();
        GDALDataset **datasets = NULL;
        
        try
        {
            datasets = new GDALDataset*[1];
            datasets[0] = (GDALDataset *) GDALOpenShared(inputImage.c_str(), GA_ReadOnly);
            if(datasets[0] == NULL)
            {
                std::string message = std::string("Could not open image ") + inputImage;
                throw rsgis::RSGISImageException(message.c_str());
            }
            
            rsgis::img::RSGISCalcLinearSpectralUnmixing calcSpecUnmix(imageFormat, RSGIS_to_GDAL_Type(outDataType), lsumGain, lsumOffset);
            calcSpecUnmix.performExhaustiveConstrainedSpectralUnmixing(datasets, 1, outputFile, endmembersFile, stepResolution);
            
            GDALClose(datasets[0]);
            delete[] datasets;
        }
        catch(rsgis::RSGISException e)
        {
            throw RSGISCmdException(e.what());
        }

    }
    
    void executeConSum1LinearSpecUnmix(std::string inputImage, std::string imageFormat, RSGISLibDataType outDataType, float lsumGain, float lsumOffset, float lsumWeight, std::string outputFile, std::string endmembersFile)throw(RSGISCmdException)
    {
        GDALAllRegister();
        GDALDataset **datasets = NULL;
        
        try
        {
            datasets = new GDALDataset*[1];
            datasets[0] = (GDALDataset *) GDALOpenShared(inputImage.c_str(), GA_ReadOnly);
            if(datasets[0] == NULL)
            {
                std::string message = std::string("Could not open image ") + inputImage;
                throw rsgis::RSGISImageException(message.c_str());
            }
            
            rsgis::img::RSGISCalcLinearSpectralUnmixing calcSpecUnmix(imageFormat, RSGIS_to_GDAL_Type(outDataType), lsumGain, lsumOffset);
            calcSpecUnmix.performPartConstainedLinearSpectralUnmixing(datasets, 1, outputFile, endmembersFile, lsumWeight);
            
            GDALClose(datasets[0]);
            delete[] datasets;
        }
        catch(rsgis::RSGISException e)
        {
            throw RSGISCmdException(e.what());
        }
    }
    
    void executeNnConSum1LinearSpecUnmix(std::string inputImage, std::string imageFormat, RSGISLibDataType outDataType, float lsumGain, float lsumOffset, float lsumWeight, std::string outputFile, std::string endmembersFile)throw(RSGISCmdException)
    {
        GDALAllRegister();
        GDALDataset **datasets = NULL;
        
        try
        {
            datasets = new GDALDataset*[1];
            datasets[0] = (GDALDataset *) GDALOpenShared(inputImage.c_str(), GA_ReadOnly);
            if(datasets[0] == NULL)
            {
                std::string message = std::string("Could not open image ") + inputImage;
                throw rsgis::RSGISImageException(message.c_str());
            }
        
            
            rsgis::img::RSGISCalcLinearSpectralUnmixing calcSpecUnmix(imageFormat, RSGIS_to_GDAL_Type(outDataType), lsumGain, lsumOffset);
            calcSpecUnmix.performConstainedNNLinearSpectralUnmixing(datasets, 1, outputFile, endmembersFile, lsumWeight);
            
            GDALClose(datasets[0]);
            delete[] datasets;
        }
        catch(rsgis::RSGISException e)
        {
            throw RSGISCmdException(e.what());
        }

    }
    
    void executeAllBandsEqualTo(std::string inputImage, float imgValue, float outputTrueVal, float outputFalseVal, std::string outputImage, std::string imageFormat, RSGISLibDataType outDataType)throw(RSGISCmdException)
    {
        try
        {
            GDALAllRegister();
            GDALDataset **datasets = new GDALDataset*[1];
            datasets[0] = (GDALDataset *) GDALOpenShared(inputImage.c_str(), GA_ReadOnly);
            if(datasets[0] == NULL)
            {
                std::string message = std::string("Could not open image ") + inputImage;
                throw rsgis::RSGISImageException(message.c_str());
            }
            
            rsgis::img::RSGISAllBandsEqualTo *calcImageValue = new rsgis::img::RSGISAllBandsEqualTo(1, imgValue, outputTrueVal, outputFalseVal);
            rsgis::img::RSGISCalcImage calcImage = rsgis::img::RSGISCalcImage(calcImageValue, "", true);
            calcImage.calcImage(datasets, 1, outputImage, false, NULL, imageFormat, RSGIS_to_GDAL_Type(outDataType));
            
            GDALClose(datasets[0]);
            delete[] datasets;
            delete calcImageValue;
        }
        catch(rsgis::RSGISException e)
        {
            throw RSGISCmdException(e.what());
        }

    }
    
    void executeHistogram(std::string inputImage, std::string imageMask, std::string outputFile, unsigned int imgBand, float imgValue, double binWidth, bool calcInMinMax, double inMin, double inMax)throw(RSGISCmdException)
    {
        try {
            GDALAllRegister();
            GDALDataset **datasets = new GDALDataset*[2];
            
            datasets[0] = (GDALDataset *) GDALOpenShared(imageMask.c_str(), GA_ReadOnly);
            if(datasets[0] == NULL)
            {
                std::string message = std::string("Could not open image ") + imageMask;
                throw rsgis::RSGISImageException(message.c_str());
            }
            
            datasets[1] = (GDALDataset *) GDALOpenShared(inputImage.c_str(), GA_ReadOnly);
            if(datasets[1] == NULL)
            {
                std::string message = std::string("Could not open image ") + inputImage;
                throw rsgis::RSGISImageException(message.c_str());
            }
            
            if(calcInMinMax)
            {
                unsigned int numImgBands = datasets[1]->GetRasterCount();
                rsgis::img::ImageStats **imgStats = new rsgis::img::ImageStats*[numImgBands];
                for(unsigned int i = 0; i < numImgBands; ++i)
                {
                    imgStats[i] = new rsgis::img::ImageStats();
                    imgStats[i]->max = 0;
                    imgStats[i]->min = 0;
                    imgStats[i]->mean = 0;
                    imgStats[i]->stddev = 0;
                    imgStats[i]->sum = 0;
                }
                rsgis::img::RSGISImageStatistics calcStats;
                calcStats.calcImageStatistics(&datasets[1], 1, imgStats, numImgBands, false, false);
                
                inMin = imgStats[imgBand-1]->min;
                inMax = imgStats[imgBand-1]->max;
                
                for(unsigned int i = 0; i < numImgBands; ++i)
                {
                    delete imgStats[i];
                }
                delete[] imgStats;
                
            }
            
            inMin = floor(inMin);
            inMax = ceil(inMax);
            
            rsgis::img::RSGISGenHistogram genHistogram;
            genHistogram.genHistogram(datasets, 2, outputFile, imgBand, inMin, inMax, imgValue, binWidth);
            
            GDALClose(datasets[0]);
            GDALClose(datasets[1]);
            delete[] datasets;
        }
        catch(rsgis::RSGISException e)
        {
            throw RSGISCmdException(e.what());
        }
    }
    
    void executeBandPercentile(std::string inputImage, float percentile, float noDataValue, bool noDataValueSpecified, std::string outputFile)throw(RSGISCmdException)
    {
        try
        {
            GDALAllRegister();
            GDALDataset *imageDataset = (GDALDataset *) GDALOpenShared(inputImage.c_str(), GA_ReadOnly);
            if(imageDataset == NULL)
            {
                std::string message = std::string("Could not open image ") + inputImage;
                throw rsgis::RSGISImageException(message.c_str());
            }
            
            rsgis::math::RSGISMatrices matrixUtils;
            rsgis::img::RSGISImagePercentiles calcPercentiles;
            
            rsgis::math::Matrix *bandPercentiles = calcPercentiles.getPercentilesForAllBands(imageDataset, percentile, noDataValue, noDataValueSpecified);
            
            matrixUtils.saveMatrix2GridTxt(bandPercentiles, outputFile);
            matrixUtils.freeMatrix(bandPercentiles);
            
            GDALClose(imageDataset);
        }
        catch(rsgis::RSGISException e)
        {
            throw RSGISCmdException(e.what());
        }
    }
    
    void executeImageDistToGeoms(std::string inputImage, std::string inputVector, std::string imageFormat, std::string outputImage)throw(RSGISCmdException)
    {
        GDALAllRegister();
        OGRRegisterAll();
        
        rsgis::utils::RSGISFileUtils fileUtils;
        rsgis::vec::RSGISVectorUtils vecUtils;
        
        std::string SHPFileInLayer = vecUtils.getLayerName(inputVector);
        
        OGRDataSource *inputSHPDS = NULL;
        OGRLayer *inputSHPLayer = NULL;
        try
        {
            
            GDALDataset *imgDataset = (GDALDataset *) GDALOpenShared(inputImage.c_str(), GA_ReadOnly);
            if(imgDataset == NULL)
            {
                std::string message = std::string("Could not open image ") + inputImage;
                throw rsgis::RSGISImageException(message.c_str());
            }
            
            
            /////////////////////////////////////
            //
            // Open Input Shapfile.
            //
            /////////////////////////////////////
            inputSHPDS = OGRSFDriverRegistrar::Open(inputVector.c_str(), FALSE);
            if(inputSHPDS == NULL)
            {
                std::string message = std::string("Could not open vector file ") + inputVector;
                throw rsgis::RSGISFileException(message.c_str());
            }
            inputSHPLayer = inputSHPDS->GetLayerByName(SHPFileInLayer.c_str());
            if(inputSHPLayer == NULL)
            {
                std::string message = std::string("Could not open vector layer ") + SHPFileInLayer;
                throw rsgis::RSGISFileException(message.c_str());
            }
            
            
            // Get Geometries into memory
            std::vector<OGRGeometry*> *ogrGeoms = new std::vector<OGRGeometry*>();
            rsgis::vec::RSGISGetOGRGeometries *getOGRGeoms = new rsgis::vec::RSGISGetOGRGeometries(ogrGeoms);
            rsgis::vec::RSGISProcessVector processVector = rsgis::vec::RSGISProcessVector(getOGRGeoms);
            processVector.processVectorsNoOutput(inputSHPLayer, false);
            delete getOGRGeoms;
            
            // Create Geometry Collection
            OGRGeometryCollection *geomCollectionOrig = new OGRGeometryCollection();
            for(std::vector<OGRGeometry*>::iterator iterGeoms = ogrGeoms->begin(); iterGeoms != ogrGeoms->end(); ++iterGeoms)
            {
                geomCollectionOrig->addGeometryDirectly(*iterGeoms);
            }
            
            OGRGeometryCollection *geomCollectionLines = new OGRGeometryCollection();
            rsgis::geom::RSGISGeometry geomUtils;
            geomUtils.convertGeometryCollection2Lines(geomCollectionOrig, geomCollectionLines);
            
            // Create blank image
            rsgis::img::RSGISImageUtils imageUtils;
            GDALDataset *outImage = imageUtils.createCopy(imgDataset, 1, outputImage, imageFormat, GDT_Float32);
            
            rsgis::img::RSGISCalcDist2Geom *dist2GeomCalcValue = new rsgis::img::RSGISCalcDist2Geom(1, geomCollectionLines, geomCollectionOrig);
            rsgis::img::RSGISCalcEditImage *calcEditImage = new rsgis::img::RSGISCalcEditImage(dist2GeomCalcValue);
            calcEditImage->calcImage(outImage);
            
            // Clean up memory.
            delete geomCollectionOrig;
            delete geomCollectionLines;
            delete ogrGeoms;
            OGRDataSource::DestroyDataSource(inputSHPDS);
            OGRCleanupAll();
            GDALClose(outImage);
            GDALClose(imgDataset);
        }
        catch (rsgis::RSGISException e)
        {
            throw RSGISCmdException(e.what());
        }

    }
}}


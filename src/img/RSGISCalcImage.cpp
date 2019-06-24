/*
 *  RSGISCalcImage.cpp
 *  RSGIS_LIB
 *
 *  Created by Pete Bunting on 23/04/2008.
 *  Copyright 2008 RSGISLib.
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

#include "RSGISCalcImage.h"

namespace rsgis{namespace img{
	
	RSGISCalcImage::RSGISCalcImage(RSGISCalcImageValue *valueCalc, std::string proj, bool useImageProj)
	{
		this->calc = valueCalc;
		this->numOutBands = valueCalc->getNumOutBands();
		this->proj = proj;
		this->useImageProj = useImageProj;
	}
    
    
    void RSGISCalcImage::calcImage(GDALDataset **datasets, int numDS, std::string outputImage, bool setOutNames, std::string *bandNames, std::string gdalFormat, GDALDataType gdalDataType)
    {
        GDALAllRegister();
		RSGISImageUtils imgUtils;
		double *gdalTranslation = new double[6];
		int **dsOffsets = new int*[numDS];
		for(int i = 0; i < numDS; i++)
		{
			dsOffsets[i] = new int[2];
		}
		int **bandOffsets = NULL;
		int height = 0;
		int width = 0;
		int numInBands = 0;
        int xBlockSize = 0;
        int yBlockSize = 0;
		
		float **inputData = NULL;
		double **outputData = NULL;
		float *inDataColumn = NULL;
		double *outDataColumn = NULL;
		
		GDALDataset *outputImageDS = NULL;
		GDALRasterBand **inputRasterBands = NULL;
		GDALRasterBand **outputRasterBands = NULL;
		GDALDriver *gdalDriver = NULL;
        
        try
		{
			// Find image overlap
			imgUtils.getImageOverlap(datasets, numDS, dsOffsets, &width, &height, gdalTranslation, &xBlockSize, &yBlockSize);
                        
			// Count number of image bands
			for(int i = 0; i < numDS; i++)
			{
				numInBands += datasets[i]->GetRasterCount();
			}
            
			// Create new Image
			gdalDriver = GetGDALDriverManager()->GetDriverByName(gdalFormat.c_str());
			if(gdalDriver == NULL)
			{
				throw RSGISImageBandException("Requested GDAL driver does not exists..");
			}
			std::cout << "New image width = " << width << " height = " << height << " bands = " << this->numOutBands << std::endl;
			
			outputImageDS = gdalDriver->Create(outputImage.c_str(), width, height, this->numOutBands, gdalDataType, NULL);
			
			if(outputImageDS == NULL)
			{
				throw RSGISImageBandException("Output image could not be created. Check filepath.");
			}
			outputImageDS->SetGeoTransform(gdalTranslation);
			if(useImageProj)
			{
				outputImageDS->SetProjection(datasets[0]->GetProjectionRef());
			}
			else
			{
				outputImageDS->SetProjection(proj.c_str());
			}
            
			// Get Image Input Bands
			bandOffsets = new int*[numInBands];
			inputRasterBands = new GDALRasterBand*[numInBands];
			int counter = 0;
			for(int i = 0; i < numDS; i++)
			{
				for(int j = 0; j < datasets[i]->GetRasterCount(); j++)
				{
					inputRasterBands[counter] = datasets[i]->GetRasterBand(j+1);
					bandOffsets[counter] = new int[2];
					bandOffsets[counter][0] = dsOffsets[i][0];
					bandOffsets[counter][1] = dsOffsets[i][1];
					counter++;
				}
			}
            
			//Get Image Output Bands
			outputRasterBands = new GDALRasterBand*[this->numOutBands];
			for(int i = 0; i < this->numOutBands; i++)
			{
				outputRasterBands[i] = outputImageDS->GetRasterBand(i+1);
				if (setOutNames) // Set output band names
				{
					outputRasterBands[i]->SetDescription(bandNames[i].c_str());
				}
			}
            int outXBlockSize = 0;
            int outYBlockSize = 0;
            outputRasterBands[0]->GetBlockSize (&outXBlockSize, &outYBlockSize);
            
            if(outYBlockSize > yBlockSize)
            {
                yBlockSize = outYBlockSize;
            }
            
			// Allocate memory
			inputData = new float*[numInBands];
			for(int i = 0; i < numInBands; i++)
			{
				inputData[i] = (float *) CPLMalloc(sizeof(float)*(width*yBlockSize));
			}
			inDataColumn = new float[numInBands];
            
			outputData = new double*[this->numOutBands];
			for(int i = 0; i < this->numOutBands; i++)
			{
				outputData[i] = (double *) CPLMalloc(sizeof(double)*(width*yBlockSize));
			}
			outDataColumn = new double[this->numOutBands];
                      
            int nYBlocks = floor(((double)height) / ((double)yBlockSize));
            int remainRows = height - (nYBlocks * yBlockSize);
            int rowOffset = 0;
            
			int feedback = height/10.0;
			int feedbackCounter = 0;
			std::cout << "Started " << std::flush;
			// Loop images to process data
			for(int i = 0; i < nYBlocks; i++)
			{
				for(int n = 0; n < numInBands; n++)
				{
                    rowOffset = bandOffsets[n][1] + (yBlockSize * i);
					inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, yBlockSize, inputData[n], width, yBlockSize, GDT_Float32, 0, 0);
				}
                
                for(int m = 0; m < yBlockSize; ++m)
                {
                    if((feedback != 0) && (((i*yBlockSize)+m) % feedback) == 0)
                    {
                        std::cout << "." << feedbackCounter << "." << std::flush;
                        feedbackCounter = feedbackCounter + 10;
                    }
                    
                    
                    for(int j = 0; j < width; j++)
                    {
                        for(int n = 0; n < numInBands; n++)
                        {
                            inDataColumn[n] = inputData[n][(m*width)+j];
                        }
                        
                        this->calc->calcImageValue(inDataColumn, numInBands, outDataColumn);
                        
                        for(int n = 0; n < this->numOutBands; n++)
                        {
                            outputData[n][(m*width)+j] = outDataColumn[n];
                        }
                        
                    }
                }
				
				for(int n = 0; n < this->numOutBands; n++)
				{
                    rowOffset = yBlockSize * i;
					outputRasterBands[n]->RasterIO(GF_Write, 0, rowOffset, width, yBlockSize, outputData[n], width, yBlockSize, GDT_Float64, 0, 0);
				}
			}
            
            if(remainRows > 0)
            {
                for(int n = 0; n < numInBands; n++)
				{
                    rowOffset = bandOffsets[n][1] + (yBlockSize * nYBlocks);
					inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, remainRows, inputData[n], width, remainRows, GDT_Float32, 0, 0);
				}
                                
                for(int m = 0; m < remainRows; ++m)
                {
                    if((feedback != 0) && ((((nYBlocks*yBlockSize)+m) % feedback) == 0))
                    {
                        std::cout << "." << feedbackCounter << "." << std::flush;
                        feedbackCounter = feedbackCounter + 10;
                    }
                    
                    for(int j = 0; j < width; j++)
                    {
                        for(int n = 0; n < numInBands; n++)
                        {
                            inDataColumn[n] = inputData[n][(m*width)+j];
                        }
                        
                        this->calc->calcImageValue(inDataColumn, numInBands, outDataColumn);
                        
                        for(int n = 0; n < this->numOutBands; n++)
                        {
                            outputData[n][(m*width)+j] = outDataColumn[n];
                        }
                        
                    }
                }
				
				for(int n = 0; n < this->numOutBands; n++)
				{
                    rowOffset = (yBlockSize * nYBlocks);
					outputRasterBands[n]->RasterIO(GF_Write, 0, rowOffset, width, remainRows, outputData[n], width, remainRows, GDT_Float64, 0, 0);
				}
            }
			std::cout << " Complete.\n";
		}
		catch(RSGISImageCalcException& e)
		{
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					if(dsOffsets[i] != NULL)
					{
						delete[] dsOffsets[i];
					}
				} 
				delete[] dsOffsets;
			}
			
			if(bandOffsets != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(bandOffsets[i] != NULL)
					{
						delete[] bandOffsets[i];
					}
				}
				delete[] bandOffsets;
			}			
			
			if(inputData != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(inputData[i] != NULL)
					{
						delete[] inputData[i];
					}
				}
				delete[] inputData;
			}
			
			if(outputData != NULL)
			{
				for(int i = 0; i < this->numOutBands; i++)
				{
					if(outputData[i] != NULL)
					{
						delete[] outputData[i];
					}
				}
				delete[] outputData;
			}
			
			if(inDataColumn != NULL)
			{
				delete[] inDataColumn;
			}
			
			if(outDataColumn != NULL)
			{
				delete[] outDataColumn;
			}
			
			if(inputRasterBands != NULL)
			{
				delete[] inputRasterBands;
			}
			
			if(outputRasterBands != NULL)
			{
				delete[] outputRasterBands;
			}
			throw e;
		}
		catch(RSGISImageBandException& e)
		{
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					if(dsOffsets[i] != NULL)
					{
						delete[] dsOffsets[i];
					}
				} 
				delete[] dsOffsets;
			}
			
			if(bandOffsets != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(bandOffsets[i] != NULL)
					{
						delete[] bandOffsets[i];
					}
				}
				delete[] bandOffsets;
			}			
			
			if(inputData != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(inputData[i] != NULL)
					{
						delete[] inputData[i];
					}
				}
				delete[] inputData;
			}
			
			if(outputData != NULL)
			{
				for(int i = 0; i < this->numOutBands; i++)
				{
					if(outputData[i] != NULL)
					{
						delete[] outputData[i];
					}
				}
				delete[] outputData;
			}
			
			if(inDataColumn != NULL)
			{
				delete[] inDataColumn;
			}
			
			if(outDataColumn != NULL)
			{
				delete[] outDataColumn;
			}
			
			if(inputRasterBands != NULL)
			{
				delete[] inputRasterBands;
			}
			
			if(outputRasterBands != NULL)
			{
				delete[] outputRasterBands;
			}
			throw e;
		}
		
		GDALClose(outputImageDS);
		
		if(gdalTranslation != NULL)
		{
			delete[] gdalTranslation;
		}
		
		if(dsOffsets != NULL)
		{
			for(int i = 0; i < numDS; i++)
			{
				if(dsOffsets[i] != NULL)
				{
					delete[] dsOffsets[i];
				}
			} 
			delete[] dsOffsets;
		}
		
		if(bandOffsets != NULL)
		{
			for(int i = 0; i < numInBands; i++)
			{
				if(bandOffsets[i] != NULL)
				{
					delete[] bandOffsets[i];
				}
			}
			delete[] bandOffsets;
		}
		
		if(inputData != NULL)
		{
			for(int i = 0; i < numInBands; i++)
			{
				if(inputData[i] != NULL)
				{
					CPLFree(inputData[i]);
				}
			}
			delete[] inputData;
		}
		
		if(outputData != NULL)
		{
			for(int i = 0; i < this->numOutBands; i++)
			{
				if(outputData[i] != NULL)
				{
					CPLFree(outputData[i]);
				}
			}
			delete[] outputData;
		}
		
		if(inDataColumn != NULL)
		{
			delete[] inDataColumn;
		}
		
		if(outDataColumn != NULL)
		{
			delete[] outDataColumn;
		}
		
		if(inputRasterBands != NULL)
		{
			delete[] inputRasterBands;
		}
		
		if(outputRasterBands != NULL)
		{
			delete[] outputRasterBands;
		}
    }
    
    
    
    void RSGISCalcImage::calcImage(GDALDataset **datasets, int numDS, std::string outputImage, std::string outputRefIntImage, std::string gdalFormat, GDALDataType gdalDataType)
    {
        GDALAllRegister();
        RSGISImageUtils imgUtils;
        double *gdalTranslation = new double[6];
        int **dsOffsets = new int*[numDS];
        for(int i = 0; i < numDS; i++)
        {
            dsOffsets[i] = new int[2];
        }
        int **bandOffsets = NULL;
        int height = 0;
        int width = 0;
        int numInBands = 0;
        int xBlockSize = 0;
        int yBlockSize = 0;
        
        float **inputData = NULL;
        double **outputData = NULL;
        double *outputRefData = NULL;
        float *inDataColumn = NULL;
        double *outDataColumn = NULL;
        double outRefData = 0;
        
        GDALDataset *outputImageDS = NULL;
        GDALDataset *outputRefImageDS = NULL;
        GDALRasterBand **inputRasterBands = NULL;
        GDALRasterBand **outputRasterBands = NULL;
        GDALRasterBand *outputRefRasterBand = NULL;
        GDALDriver *gdalDriver = NULL;
        
        try
        {
            // Find image overlap
            imgUtils.getImageOverlap(datasets, numDS, dsOffsets, &width, &height, gdalTranslation, &xBlockSize, &yBlockSize);
            
            // Count number of image bands
            for(int i = 0; i < numDS; i++)
            {
                numInBands += datasets[i]->GetRasterCount();
            }
            
            // Create new Image
            gdalDriver = GetGDALDriverManager()->GetDriverByName(gdalFormat.c_str());
            if(gdalDriver == NULL)
            {
                throw RSGISImageBandException("Requested GDAL driver does not exists..");
            }
            std::cout << "New image width = " << width << " height = " << height << " bands = " << this->numOutBands << std::endl;
            
            outputImageDS = gdalDriver->Create(outputImage.c_str(), width, height, this->numOutBands, gdalDataType, NULL);
            if(outputImageDS == NULL)
            {
                throw RSGISImageBandException("Output image could not be created. Check filepath.");
            }
            outputImageDS->SetGeoTransform(gdalTranslation);
            if(useImageProj)
            {
                outputImageDS->SetProjection(datasets[0]->GetProjectionRef());
            }
            else
            {
                outputImageDS->SetProjection(proj.c_str());
            }
            
            outputRefImageDS = gdalDriver->Create(outputRefIntImage.c_str(), width, height, 1, GDT_UInt32, NULL);
            if(outputRefImageDS == NULL)
            {
                throw RSGISImageBandException("Output reference image could not be created. Check filepath.");
            }
            outputRefImageDS->SetGeoTransform(gdalTranslation);
            if(useImageProj)
            {
                outputRefImageDS->SetProjection(datasets[0]->GetProjectionRef());
            }
            else
            {
                outputRefImageDS->SetProjection(proj.c_str());
            }
            
            // Get Image Input Bands
            bandOffsets = new int*[numInBands];
            inputRasterBands = new GDALRasterBand*[numInBands];
            int counter = 0;
            for(int i = 0; i < numDS; i++)
            {
                for(int j = 0; j < datasets[i]->GetRasterCount(); j++)
                {
                    inputRasterBands[counter] = datasets[i]->GetRasterBand(j+1);
                    bandOffsets[counter] = new int[2];
                    bandOffsets[counter][0] = dsOffsets[i][0];
                    bandOffsets[counter][1] = dsOffsets[i][1];
                    counter++;
                }
            }
            
            //Get Image Output Bands
            outputRasterBands = new GDALRasterBand*[this->numOutBands];
            for(int i = 0; i < this->numOutBands; i++)
            {
                outputRasterBands[i] = outputImageDS->GetRasterBand(i+1);
            }
            outputRefRasterBand = outputRefImageDS->GetRasterBand(1);
            
            int outXBlockSize = 0;
            int outYBlockSize = 0;
            outputRasterBands[0]->GetBlockSize(&outXBlockSize, &outYBlockSize);
            
            if(outYBlockSize > yBlockSize)
            {
                yBlockSize = outYBlockSize;
            }
            
            // Allocate memory
            inputData = new float*[numInBands];
            for(int i = 0; i < numInBands; i++)
            {
                inputData[i] = (float *) CPLMalloc(sizeof(float)*(width*yBlockSize));
            }
            inDataColumn = new float[numInBands];
            
            outputData = new double*[this->numOutBands];
            for(int i = 0; i < this->numOutBands; i++)
            {
                outputData[i] = (double *) CPLMalloc(sizeof(double)*(width*yBlockSize));
            }
            outDataColumn = new double[this->numOutBands];
            outputRefData = (double *) CPLMalloc(sizeof(double)*(width*yBlockSize));
            
            int nYBlocks = floor(((double)height) / ((double)yBlockSize));
            int remainRows = height - (nYBlocks * yBlockSize);
            int rowOffset = 0;
            
            int feedback = height/10.0;
            int feedbackCounter = 0;
            std::cout << "Started " << std::flush;
            // Loop images to process data
            for(int i = 0; i < nYBlocks; i++)
            {
                for(int n = 0; n < numInBands; n++)
                {
                    rowOffset = bandOffsets[n][1] + (yBlockSize * i);
                    inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, yBlockSize, inputData[n], width, yBlockSize, GDT_Float32, 0, 0);
                }
                
                for(int m = 0; m < yBlockSize; ++m)
                {
                    if((feedback != 0) && (((i*yBlockSize)+m) % feedback) == 0)
                    {
                        std::cout << "." << feedbackCounter << "." << std::flush;
                        feedbackCounter = feedbackCounter + 10;
                    }
                    
                    
                    for(int j = 0; j < width; j++)
                    {
                        for(int n = 0; n < numInBands; n++)
                        {
                            inDataColumn[n] = inputData[n][(m*width)+j];
                        }
                        
                        this->calc->calcImageValue(inDataColumn, numInBands, outDataColumn, &outRefData, 1);
                        
                        for(int n = 0; n < this->numOutBands; n++)
                        {
                            outputData[n][(m*width)+j] = outDataColumn[n];
                        }
                        outputRefData[(m*width)+j] = outRefData;
                    }
                }
                
                rowOffset = yBlockSize * i;
                for(int n = 0; n < this->numOutBands; n++)
                {
                    outputRasterBands[n]->RasterIO(GF_Write, 0, rowOffset, width, yBlockSize, outputData[n], width, yBlockSize, GDT_Float64, 0, 0);
                }
                outputRefRasterBand->RasterIO(GF_Write, 0, rowOffset, width, yBlockSize, outputRefData, width, yBlockSize, GDT_Float64, 0, 0);
            }
            
            if(remainRows > 0)
            {
                for(int n = 0; n < numInBands; n++)
                {
                    rowOffset = bandOffsets[n][1] + (yBlockSize * nYBlocks);
                    inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, remainRows, inputData[n], width, remainRows, GDT_Float32, 0, 0);
                }
                
                for(int m = 0; m < remainRows; ++m)
                {
                    if((feedback != 0) && ((((nYBlocks*yBlockSize)+m) % feedback) == 0))
                    {
                        std::cout << "." << feedbackCounter << "." << std::flush;
                        feedbackCounter = feedbackCounter + 10;
                    }
                    
                    for(int j = 0; j < width; j++)
                    {
                        for(int n = 0; n < numInBands; n++)
                        {
                            inDataColumn[n] = inputData[n][(m*width)+j];
                        }
                        
                        this->calc->calcImageValue(inDataColumn, numInBands, outDataColumn, &outRefData, 1);
                        
                        for(int n = 0; n < this->numOutBands; n++)
                        {
                            outputData[n][(m*width)+j] = outDataColumn[n];
                        }
                        outputRefData[(m*width)+j] = outRefData;
                    }
                }
                
                for(int n = 0; n < this->numOutBands; n++)
                {
                    rowOffset = (yBlockSize * nYBlocks);
                    outputRasterBands[n]->RasterIO(GF_Write, 0, rowOffset, width, remainRows, outputData[n], width, remainRows, GDT_Float64, 0, 0);
                }
                outputRefRasterBand->RasterIO(GF_Write, 0, rowOffset, width, remainRows, outputRefData, width, remainRows, GDT_Float64, 0, 0);
            }
            std::cout << " Complete.\n";
        }
        catch(RSGISImageCalcException& e)
        {
            if(gdalTranslation != NULL)
            {
                delete[] gdalTranslation;
            }
            
            if(dsOffsets != NULL)
            {
                for(int i = 0; i < numDS; i++)
                {
                    if(dsOffsets[i] != NULL)
                    {
                        delete[] dsOffsets[i];
                    }
                }
                delete[] dsOffsets;
            }
            
            if(bandOffsets != NULL)
            {
                for(int i = 0; i < numInBands; i++)
                {
                    if(bandOffsets[i] != NULL)
                    {
                        delete[] bandOffsets[i];
                    }
                }
                delete[] bandOffsets;
            }
            
            if(inputData != NULL)
            {
                for(int i = 0; i < numInBands; i++)
                {
                    if(inputData[i] != NULL)
                    {
                        delete[] inputData[i];
                    }
                }
                delete[] inputData;
            }
            
            if(outputData != NULL)
            {
                for(int i = 0; i < this->numOutBands; i++)
                {
                    if(outputData[i] != NULL)
                    {
                        delete[] outputData[i];
                    }
                }
                delete[] outputData;
            }
            
            if(outputRefData != NULL)
            {
                delete[] outputRefData;
            }
            
            if(inDataColumn != NULL)
            {
                delete[] inDataColumn;
            }
            
            if(outDataColumn != NULL)
            {
                delete[] outDataColumn;
            }
            
            if(inputRasterBands != NULL)
            {
                delete[] inputRasterBands;
            }
            
            if(outputRasterBands != NULL)
            {
                delete[] outputRasterBands;
            }
            throw e;
        }
        catch(RSGISImageBandException& e)
        {
            if(gdalTranslation != NULL)
            {
                delete[] gdalTranslation;
            }
            
            if(dsOffsets != NULL)
            {
                for(int i = 0; i < numDS; i++)
                {
                    if(dsOffsets[i] != NULL)
                    {
                        delete[] dsOffsets[i];
                    }
                }
                delete[] dsOffsets;
            }
            
            if(bandOffsets != NULL)
            {
                for(int i = 0; i < numInBands; i++)
                {
                    if(bandOffsets[i] != NULL)
                    {
                        delete[] bandOffsets[i];
                    }
                }
                delete[] bandOffsets;
            }
            
            if(inputData != NULL)
            {
                for(int i = 0; i < numInBands; i++)
                {
                    if(inputData[i] != NULL)
                    {
                        delete[] inputData[i];
                    }
                }
                delete[] inputData;
            }
            
            if(outputData != NULL)
            {
                for(int i = 0; i < this->numOutBands; i++)
                {
                    if(outputData[i] != NULL)
                    {
                        delete[] outputData[i];
                    }
                }
                delete[] outputData;
            }
            
            if(outputRefData != NULL)
            {
                delete[] outputRefData;
            }
            
            if(inDataColumn != NULL)
            {
                delete[] inDataColumn;
            }
            
            if(outDataColumn != NULL)
            {
                delete[] outDataColumn;
            }
            
            if(inputRasterBands != NULL)
            {
                delete[] inputRasterBands;
            }
            
            if(outputRasterBands != NULL)
            {
                delete[] outputRasterBands;
            }
            throw e;
        }
        
        GDALClose(outputImageDS);
        GDALClose(outputRefImageDS);
        
        if(gdalTranslation != NULL)
        {
            delete[] gdalTranslation;
        }
        
        if(dsOffsets != NULL)
        {
            for(int i = 0; i < numDS; i++)
            {
                if(dsOffsets[i] != NULL)
                {
                    delete[] dsOffsets[i];
                }
            }
            delete[] dsOffsets;
        }
        
        if(bandOffsets != NULL)
        {
            for(int i = 0; i < numInBands; i++)
            {
                if(bandOffsets[i] != NULL)
                {
                    delete[] bandOffsets[i];
                }
            }
            delete[] bandOffsets;
        }
        
        if(inputData != NULL)
        {
            for(int i = 0; i < numInBands; i++)
            {
                if(inputData[i] != NULL)
                {
                    CPLFree(inputData[i]);
                }
            }
            delete[] inputData;
        }
        
        if(outputData != NULL)
        {
            for(int i = 0; i < this->numOutBands; i++)
            {
                if(outputData[i] != NULL)
                {
                    CPLFree(outputData[i]);
                }
            }
            delete[] outputData;
        }
        
        if(outputRefData != NULL)
        {
            delete[] outputRefData;
        }
        
        if(inDataColumn != NULL)
        {
            delete[] inDataColumn;
        }
        
        if(outDataColumn != NULL)
        {
            delete[] outDataColumn;
        }
        
        if(inputRasterBands != NULL)
        {
            delete[] inputRasterBands;
        }
        
        if(outputRasterBands != NULL)
        {
            delete[] outputRasterBands;
        }
    }
    
    
    
    
    
    
    void RSGISCalcImage::calcImage(GDALDataset **datasets, int numDS, GDALDataset *outputImageDS)
	{
		GDALAllRegister();
		RSGISImageUtils imgUtils;
		double *gdalTranslation = new double[6];
		int **dsOffsets = new int*[numDS];
		for(int i = 0; i < numDS; i++)
		{
			dsOffsets[i] = new int[2];
		}
		int **bandOffsets = NULL;
		int height = 0;
		int width = 0;
		int numInBands = 0;
		
		float **inputData = NULL;
		double **outputData = NULL;
		float *inDataColumn = NULL;
		double *outDataColumn = NULL;
        int xBlockSize = 0;
        int yBlockSize = 0;
		
		GDALRasterBand **inputRasterBands = NULL;
		GDALRasterBand **outputRasterBands = NULL;
		
		try
		{
			// Find image overlap
			imgUtils.getImageOverlap(datasets, numDS, dsOffsets, &width, &height, gdalTranslation, &xBlockSize, &yBlockSize);
                        
			// Count number of image bands
			for(int i = 0; i < numDS; i++)
			{
				numInBands += datasets[i]->GetRasterCount();
			}
            
			
			if(outputImageDS->GetRasterXSize() != width)
            {
                throw RSGISImageCalcException("The output dataset does not have the correct width\n");
            }
            
            if(outputImageDS->GetRasterYSize() != height)
            {
                throw RSGISImageCalcException("The output dataset does not have the correct height\n");
            }
            
            if(outputImageDS->GetRasterCount() != this->numOutBands)
            {
                throw RSGISImageCalcException("The output dataset does not have the correct number of image bands\n");
            }
            
			// Get Image Input Bands
			bandOffsets = new int*[numInBands];
			inputRasterBands = new GDALRasterBand*[numInBands];
			int counter = 0;
			for(int i = 0; i < numDS; i++)
			{
				for(int j = 0; j < datasets[i]->GetRasterCount(); j++)
				{
					inputRasterBands[counter] = datasets[i]->GetRasterBand(j+1);
					bandOffsets[counter] = new int[2];
					bandOffsets[counter][0] = dsOffsets[i][0];
					bandOffsets[counter][1] = dsOffsets[i][1];
					counter++;
				}
			}
            
			//Get Image Output Bands
			outputRasterBands = new GDALRasterBand*[this->numOutBands];
			for(int i = 0; i < this->numOutBands; i++)
			{
				outputRasterBands[i] = outputImageDS->GetRasterBand(i+1);
			}
            int outXBlockSize = 0;
            int outYBlockSize = 0;
            outputRasterBands[0]->GetBlockSize (&outXBlockSize, &outYBlockSize);
            
            if(outYBlockSize > yBlockSize)
            {
                yBlockSize = outYBlockSize;
            }
            
			// Allocate memory
			inputData = new float*[numInBands];
			for(int i = 0; i < numInBands; i++)
			{
				inputData[i] = (float *) CPLMalloc(sizeof(float)*width*yBlockSize);
			}
			inDataColumn = new float[numInBands];
            
			outputData = new double*[this->numOutBands];
			for(int i = 0; i < this->numOutBands; i++)
			{
				outputData[i] = (double *) CPLMalloc(sizeof(double)*width*yBlockSize);
			}
			outDataColumn = new double[this->numOutBands];
            
			int nYBlocks = height / yBlockSize;
            int remainRows = height - (nYBlocks * yBlockSize);
            int rowOffset = 0;
            
			int feedback = height/10;
			int feedbackCounter = 0;
			std::cout << "Started " << std::flush;
			// Loop images to process data
			for(int i = 0; i < nYBlocks; i++)
			{
				for(int n = 0; n < numInBands; n++)
				{
                    rowOffset = bandOffsets[n][1] + (yBlockSize * i);
					inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, yBlockSize, inputData[n], width, yBlockSize, GDT_Float32, 0, 0);
				}
                
                for(int m = 0; m < yBlockSize; ++m)
                {
                    if((feedback != 0) && ((((i*yBlockSize)+m) % feedback) == 0))
                    {
                        std::cout << "." << feedbackCounter << "." << std::flush;
                        feedbackCounter = feedbackCounter + 10;
                    }
                    
                    
                    for(int j = 0; j < width; j++)
                    {
                        for(int n = 0; n < numInBands; n++)
                        {
                            inDataColumn[n] = inputData[n][(m*width)+j];
                        }
                        
                        this->calc->calcImageValue(inDataColumn, numInBands, outDataColumn);
                        
                        for(int n = 0; n < this->numOutBands; n++)
                        {
                            outputData[n][(m*width)+j] = outDataColumn[n];
                        }
                        
                    }
                }
				
				for(int n = 0; n < this->numOutBands; n++)
				{
                    rowOffset = yBlockSize * i;
					outputRasterBands[n]->RasterIO(GF_Write, 0, rowOffset, width, yBlockSize, outputData[n], width, yBlockSize, GDT_Float64, 0, 0);
				}
			}
            
            if(remainRows > 0)
            {
                for(int n = 0; n < numInBands; n++)
				{
                    rowOffset = bandOffsets[n][1] + (yBlockSize * nYBlocks);
					inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, remainRows, inputData[n], width, remainRows, GDT_Float32, 0, 0);
				}
                
                for(int m = 0; m < remainRows; ++m)
                {
                    if((feedback != 0) && ((((nYBlocks*yBlockSize)+m) % feedback) == 0))
                    {
                        std::cout << "." << feedbackCounter << "." << std::flush;
                        feedbackCounter = feedbackCounter + 10;
                    }
                    
                    for(int j = 0; j < width; j++)
                    {
                        for(int n = 0; n < numInBands; n++)
                        {
                            inDataColumn[n] = inputData[n][(m*width)+j];
                        }
                        
                        this->calc->calcImageValue(inDataColumn, numInBands, outDataColumn);
                        
                        for(int n = 0; n < this->numOutBands; n++)
                        {
                            outputData[n][(m*width)+j] = outDataColumn[n];
                        }
                        
                    }
                }
				
				for(int n = 0; n < this->numOutBands; n++)
				{
                    rowOffset = (yBlockSize * nYBlocks);
					outputRasterBands[n]->RasterIO(GF_Write, 0, rowOffset, width, remainRows, outputData[n], width, remainRows, GDT_Float64, 0, 0);
				}
            }
			std::cout << " Complete.\n";
		}
		catch(RSGISImageCalcException& e)
		{			
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					if(dsOffsets[i] != NULL)
					{
						delete[] dsOffsets[i];
					}
				} 
				delete[] dsOffsets;
			}
			
			if(bandOffsets != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(bandOffsets[i] != NULL)
					{
						delete[] bandOffsets[i];
					}
				}
				delete[] bandOffsets;
			}			
			
			if(inputData != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(inputData[i] != NULL)
					{
						delete[] inputData[i];
					}
				}
				delete[] inputData;
			}
			
			if(outputData != NULL)
			{
				for(int i = 0; i < this->numOutBands; i++)
				{
					if(outputData[i] != NULL)
					{
						delete[] outputData[i];
					}
				}
				delete[] outputData;
			}
			
			if(inDataColumn != NULL)
			{
				delete[] inDataColumn;
			}
			
			if(outDataColumn != NULL)
			{
				delete[] outDataColumn;
			}
			
			if(inputRasterBands != NULL)
			{
				delete[] inputRasterBands;
			}
			
			if(outputRasterBands != NULL)
			{
				delete[] outputRasterBands;
			}
			throw e;
		}
		catch(RSGISImageBandException& e)
		{			
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					if(dsOffsets[i] != NULL)
					{
						delete[] dsOffsets[i];
					}
				} 
				delete[] dsOffsets;
			}
			
			if(bandOffsets != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(bandOffsets[i] != NULL)
					{
						delete[] bandOffsets[i];
					}
				}
				delete[] bandOffsets;
			}			
			
			if(inputData != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(inputData[i] != NULL)
					{
						delete[] inputData[i];
					}
				}
				delete[] inputData;
			}
			
			if(outputData != NULL)
			{
				for(int i = 0; i < this->numOutBands; i++)
				{
					if(outputData[i] != NULL)
					{
						delete[] outputData[i];
					}
				}
				delete[] outputData;
			}
			
			if(inDataColumn != NULL)
			{
				delete[] inDataColumn;
			}
			
			if(outDataColumn != NULL)
			{
				delete[] outDataColumn;
			}
			
			if(inputRasterBands != NULL)
			{
				delete[] inputRasterBands;
			}
			
			if(outputRasterBands != NULL)
			{
				delete[] outputRasterBands;
			}
			throw e;
		}
				
		if(gdalTranslation != NULL)
		{
			delete[] gdalTranslation;
		}
		
		if(dsOffsets != NULL)
		{
			for(int i = 0; i < numDS; i++)
			{
				if(dsOffsets[i] != NULL)
				{
					delete[] dsOffsets[i];
				}
			} 
			delete[] dsOffsets;
		}
		
		if(bandOffsets != NULL)
		{
			for(int i = 0; i < numInBands; i++)
			{
				if(bandOffsets[i] != NULL)
				{
					delete[] bandOffsets[i];
				}
			}
			delete[] bandOffsets;
		}
		
		if(inputData != NULL)
		{
			for(int i = 0; i < numInBands; i++)
			{
				if(inputData[i] != NULL)
				{
					CPLFree(inputData[i]);
				}
			}
			delete[] inputData;
		}
		
		if(outputData != NULL)
		{
			for(int i = 0; i < this->numOutBands; i++)
			{
				if(outputData[i] != NULL)
				{
					CPLFree(outputData[i]);
				}
			}
			delete[] outputData;
		}
		
		if(inDataColumn != NULL)
		{
			delete[] inDataColumn;
		}
		
		if(outDataColumn != NULL)
		{
			delete[] outDataColumn;
		}
		
		if(inputRasterBands != NULL)
		{
			delete[] inputRasterBands;
		}
		
		if(outputRasterBands != NULL)
		{
			delete[] outputRasterBands;
		}
	}
    
    void RSGISCalcImage::calcImagePartialOutput(GDALDataset **datasets, int numDS, GDALDataset *outputImageDS)
    {
        GDALAllRegister();
        RSGISImageUtils imgUtils;
        double *gdalTranslation = new double[6];
        int **dsOffsets = new int*[numDS+1];
        for(int i = 0; i < (numDS+1); i++)
        {
            dsOffsets[i] = new int[2];
        }
        int **bandOffsets = NULL;
        int height = 0;
        int width = 0;
        int numInBands = 0;
        
        float **inputData = NULL;
        double **outputData = NULL;
        float *inDataColumn = NULL;
        double *outDataColumn = NULL;
        int xBlockSize = 0;
        int yBlockSize = 0;
        
        GDALRasterBand **inputRasterBands = NULL;
        GDALRasterBand **outputRasterBands = NULL;
        
        try
        {
            // Find image overlap
            GDALDataset **tmpDatasets = new GDALDataset*[numDS+1];
            tmpDatasets[0] = outputImageDS;
            for(int i = 1; i <= numDS; ++i)
            {
                tmpDatasets[i] = datasets[i-1];
            }
            imgUtils.getImageOverlap(tmpDatasets, numDS+1, dsOffsets, &width, &height, gdalTranslation, &xBlockSize, &yBlockSize);
            delete[] tmpDatasets;
            
            // Count number of image bands
            for(int i = 0; i < numDS; i++)
            {
                numInBands += datasets[i]->GetRasterCount();
            }
            
            if(outputImageDS->GetRasterCount() != this->numOutBands)
            {
                throw RSGISImageCalcException("The output dataset does not have the correct number of image bands\n");
            }
            
            // Get Image Input Bands
            bandOffsets = new int*[numInBands];
            inputRasterBands = new GDALRasterBand*[numInBands];
            int counter = 0;
            for(int i = 0; i < numDS; i++)
            {
                for(int j = 0; j < datasets[i]->GetRasterCount(); j++)
                {
                    inputRasterBands[counter] = datasets[i]->GetRasterBand(j+1);
                    bandOffsets[counter] = new int[2];
                    bandOffsets[counter][0] = dsOffsets[i+1][0];
                    bandOffsets[counter][1] = dsOffsets[i+1][1];
                    counter++;
                }
            }
            
            //Get Image Output Bands
            outputRasterBands = new GDALRasterBand*[this->numOutBands];
            for(int i = 0; i < this->numOutBands; i++)
            {
                outputRasterBands[i] = outputImageDS->GetRasterBand(i+1);
            }
            int outXOffset = dsOffsets[0][0];
            int outYOffset = dsOffsets[0][1];
            int outXBlockSize = 0;
            int outYBlockSize = 0;
            outputRasterBands[0]->GetBlockSize (&outXBlockSize, &outYBlockSize);
            
            if(outYBlockSize > yBlockSize)
            {
                yBlockSize = outYBlockSize;
            }
            
            // Allocate memory
            inputData = new float*[numInBands];
            for(int i = 0; i < numInBands; i++)
            {
                inputData[i] = (float *) CPLMalloc(sizeof(float)*width*yBlockSize);
            }
            inDataColumn = new float[numInBands];
            
            outputData = new double*[this->numOutBands];
            for(int i = 0; i < this->numOutBands; i++)
            {
                outputData[i] = (double *) CPLMalloc(sizeof(double)*width*yBlockSize);
            }
            outDataColumn = new double[this->numOutBands];
            
            int nYBlocks = height / yBlockSize;
            int remainRows = height - (nYBlocks * yBlockSize);
            int rowOffset = 0;
            
            int feedback = height/10;
            int feedbackCounter = 0;
            std::cout << "Started " << std::flush;
            // Loop images to process data
            for(int i = 0; i < nYBlocks; i++)
            {
                for(int n = 0; n < numInBands; n++)
                {
                    rowOffset = bandOffsets[n][1] + (yBlockSize * i);
                    inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, yBlockSize, inputData[n], width, yBlockSize, GDT_Float32, 0, 0);
                }
                
                for(int m = 0; m < yBlockSize; ++m)
                {
                    if((feedback != 0) && ((((i*yBlockSize)+m) % feedback) == 0))
                    {
                        std::cout << "." << feedbackCounter << "." << std::flush;
                        feedbackCounter = feedbackCounter + 10;
                    }
                    
                    
                    for(int j = 0; j < width; j++)
                    {
                        for(int n = 0; n < numInBands; n++)
                        {
                            inDataColumn[n] = inputData[n][(m*width)+j];
                        }
                        
                        this->calc->calcImageValue(inDataColumn, numInBands, outDataColumn);
                        
                        for(int n = 0; n < this->numOutBands; n++)
                        {
                            outputData[n][(m*width)+j] = outDataColumn[n];
                        }
                        
                    }
                }
                
                for(int n = 0; n < this->numOutBands; n++)
                {
                    rowOffset = outYOffset + (yBlockSize * i);
                    outputRasterBands[n]->RasterIO(GF_Write, outXOffset, rowOffset, width, yBlockSize, outputData[n], width, yBlockSize, GDT_Float64, 0, 0);
                }
            }
            
            if(remainRows > 0)
            {
                for(int n = 0; n < numInBands; n++)
                {
                    rowOffset = bandOffsets[n][1] + (yBlockSize * nYBlocks);
                    inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, remainRows, inputData[n], width, remainRows, GDT_Float32, 0, 0);
                }
                
                for(int m = 0; m < remainRows; ++m)
                {
                    if((feedback != 0) && ((((nYBlocks*yBlockSize)+m) % feedback) == 0))
                    {
                        std::cout << "." << feedbackCounter << "." << std::flush;
                        feedbackCounter = feedbackCounter + 10;
                    }
                    
                    for(int j = 0; j < width; j++)
                    {
                        for(int n = 0; n < numInBands; n++)
                        {
                            inDataColumn[n] = inputData[n][(m*width)+j];
                        }
                        
                        this->calc->calcImageValue(inDataColumn, numInBands, outDataColumn);
                        
                        for(int n = 0; n < this->numOutBands; n++)
                        {
                            outputData[n][(m*width)+j] = outDataColumn[n];
                        }
                        
                    }
                }
                
                for(int n = 0; n < this->numOutBands; n++)
                {
                    rowOffset = outYOffset + (yBlockSize * nYBlocks);
                    outputRasterBands[n]->RasterIO(GF_Write, outXOffset, rowOffset, width, remainRows, outputData[n], width, remainRows, GDT_Float64, 0, 0);
                }
            }
            std::cout << " Complete.\n";
        }
        catch(RSGISImageCalcException& e)
        {
            if(gdalTranslation != NULL)
            {
                delete[] gdalTranslation;
            }
            
            if(dsOffsets != NULL)
            {
                for(int i = 0; i < numDS; i++)
                {
                    if(dsOffsets[i] != NULL)
                    {
                        delete[] dsOffsets[i];
                    }
                }
                delete[] dsOffsets;
            }
            
            if(bandOffsets != NULL)
            {
                for(int i = 0; i < numInBands; i++)
                {
                    if(bandOffsets[i] != NULL)
                    {
                        delete[] bandOffsets[i];
                    }
                }
                delete[] bandOffsets;
            }
            
            if(inputData != NULL)
            {
                for(int i = 0; i < numInBands; i++)
                {
                    if(inputData[i] != NULL)
                    {
                        delete[] inputData[i];
                    }
                }
                delete[] inputData;
            }
            
            if(outputData != NULL)
            {
                for(int i = 0; i < this->numOutBands; i++)
                {
                    if(outputData[i] != NULL)
                    {
                        delete[] outputData[i];
                    }
                }
                delete[] outputData;
            }
            
            if(inDataColumn != NULL)
            {
                delete[] inDataColumn;
            }
            
            if(outDataColumn != NULL)
            {
                delete[] outDataColumn;
            }
            
            if(inputRasterBands != NULL)
            {
                delete[] inputRasterBands;
            }
            
            if(outputRasterBands != NULL)
            {
                delete[] outputRasterBands;
            }
            throw e;
        }
        catch(RSGISImageBandException& e)
        {
            if(gdalTranslation != NULL)
            {
                delete[] gdalTranslation;
            }
            
            if(dsOffsets != NULL)
            {
                for(int i = 0; i < numDS; i++)
                {
                    if(dsOffsets[i] != NULL)
                    {
                        delete[] dsOffsets[i];
                    }
                }
                delete[] dsOffsets;
            }
            
            if(bandOffsets != NULL)
            {
                for(int i = 0; i < numInBands; i++)
                {
                    if(bandOffsets[i] != NULL)
                    {
                        delete[] bandOffsets[i];
                    }
                }
                delete[] bandOffsets;
            }
            
            if(inputData != NULL)
            {
                for(int i = 0; i < numInBands; i++)
                {
                    if(inputData[i] != NULL)
                    {
                        delete[] inputData[i];
                    }
                }
                delete[] inputData;
            }
            
            if(outputData != NULL)
            {
                for(int i = 0; i < this->numOutBands; i++)
                {
                    if(outputData[i] != NULL)
                    {
                        delete[] outputData[i];
                    }
                }
                delete[] outputData;
            }
            
            if(inDataColumn != NULL)
            {
                delete[] inDataColumn;
            }
            
            if(outDataColumn != NULL)
            {
                delete[] outDataColumn;
            }
            
            if(inputRasterBands != NULL)
            {
                delete[] inputRasterBands;
            }
            
            if(outputRasterBands != NULL)
            {
                delete[] outputRasterBands;
            }
            throw e;
        }
        
        if(gdalTranslation != NULL)
        {
            delete[] gdalTranslation;
        }
        
        if(dsOffsets != NULL)
        {
            for(int i = 0; i < numDS; i++)
            {
                if(dsOffsets[i] != NULL)
                {
                    delete[] dsOffsets[i];
                }
            }
            delete[] dsOffsets;
        }
        
        if(bandOffsets != NULL)
        {
            for(int i = 0; i < numInBands; i++)
            {
                if(bandOffsets[i] != NULL)
                {
                    delete[] bandOffsets[i];
                }
            }
            delete[] bandOffsets;
        }
        
        if(inputData != NULL)
        {
            for(int i = 0; i < numInBands; i++)
            {
                if(inputData[i] != NULL)
                {
                    CPLFree(inputData[i]);
                }
            }
            delete[] inputData;
        }
        
        if(outputData != NULL)
        {
            for(int i = 0; i < this->numOutBands; i++)
            {
                if(outputData[i] != NULL)
                {
                    CPLFree(outputData[i]);
                }
            }
            delete[] outputData;
        }
        
        if(inDataColumn != NULL)
        {
            delete[] inDataColumn;
        }
        
        if(outDataColumn != NULL)
        {
            delete[] outDataColumn;
        }
        
        if(inputRasterBands != NULL)
        {
            delete[] inputRasterBands;
        }
        
        if(outputRasterBands != NULL)
        {
            delete[] outputRasterBands;
        }
    }
    
    void RSGISCalcImage::calcImage(GDALDataset **datasets, int numIntDS, int numFloatDS, std::string outputImage, bool setOutNames, std::string *bandNames , std::string gdalFormat, GDALDataType gdalDataType)
    {
        GDALAllRegister();
		RSGISImageUtils imgUtils;
        int numDS = numIntDS + numFloatDS;
		double *gdalTranslation = new double[6];
		int **dsOffsets = new int*[numDS];
		for(int i = 0; i < numDS; i++)
		{
			dsOffsets[i] = new int[2];
		}
		int **bandIntOffsets = NULL;
        int **bandFloatOffsets = NULL;
		int height = 0;
		int width = 0;
		int numIntBands = 0;
        int numFloatBands = 0;
		
		float **inputFloatData = NULL;
		float *inDataFloatColumn = NULL;
        unsigned int **inputIntData = NULL;
		long *inDataIntColumn = NULL;
        
        double **outputData = NULL;
		double *outDataColumn = NULL;
        
        int xBlockSize = 0;
        int yBlockSize = 0;
		
		GDALRasterBand **inputRasterFloatBands = NULL;
        GDALRasterBand **inputRasterIntBands = NULL;
        
        GDALDataset *outputImageDS = NULL;
		GDALRasterBand **outputRasterBands = NULL;
		GDALDriver *gdalDriver = NULL;
		
		try
		{
			// Find image overlap
			imgUtils.getImageOverlap(datasets, numDS, dsOffsets, &width, &height, gdalTranslation, &xBlockSize, &yBlockSize);

            
			// Count number of image bands
			for(int i = 0; i < numIntDS; i++)
			{
				numIntBands += datasets[i]->GetRasterCount();
			}
            for(int i = numIntDS; i < numDS; i++)
			{
				numFloatBands += datasets[i]->GetRasterCount();
			}
			
            // Create new Image
			gdalDriver = GetGDALDriverManager()->GetDriverByName(gdalFormat.c_str());
			if(gdalDriver == NULL)
			{
				throw RSGISImageBandException("Requested GDAL driver does not exists..");
			}
			std::cout << "New image width = " << width << " height = " << height << " bands = " << this->numOutBands << std::endl;
			
			outputImageDS = gdalDriver->Create(outputImage.c_str(), width, height, this->numOutBands, gdalDataType, NULL);
			
			if(outputImageDS == NULL)
			{
				throw RSGISImageBandException("Output image could not be created. Check filepath.");
			}
			outputImageDS->SetGeoTransform(gdalTranslation);
			if(useImageProj)
			{
				outputImageDS->SetProjection(datasets[0]->GetProjectionRef());
			}
			else
			{
				outputImageDS->SetProjection(proj.c_str());
			}
            
			// Get Image Input Bands
			bandIntOffsets = new int*[numIntBands];
			inputRasterIntBands = new GDALRasterBand*[numIntBands];
            
            bandFloatOffsets = new int*[numFloatBands];
			inputRasterFloatBands = new GDALRasterBand*[numFloatBands];
            
			int counter = 0;
			for(int i = 0; i < numIntDS; i++)
			{
				for(int j = 0; j < datasets[i]->GetRasterCount(); j++)
				{
					inputRasterIntBands[counter] = datasets[i]->GetRasterBand(j+1);
					bandIntOffsets[counter] = new int[2];
					bandIntOffsets[counter][0] = dsOffsets[i][0];
					bandIntOffsets[counter][1] = dsOffsets[i][1];
					counter++;
				}
			}
            
            counter = 0;
            for(int i = numIntDS; i < numDS; i++)
			{
				for(int j = 0; j < datasets[i]->GetRasterCount(); j++)
				{
					inputRasterFloatBands[counter] = datasets[i]->GetRasterBand(j+1);
					bandFloatOffsets[counter] = new int[2];
					bandFloatOffsets[counter][0] = dsOffsets[i][0];
					bandFloatOffsets[counter][1] = dsOffsets[i][1];
					counter++;
				}
			}
            
            //Get Image Output Bands
			outputRasterBands = new GDALRasterBand*[this->numOutBands];
			for(int i = 0; i < this->numOutBands; i++)
			{
				outputRasterBands[i] = outputImageDS->GetRasterBand(i+1);
				if (setOutNames) // Set output band names
				{
					outputRasterBands[i]->SetDescription(bandNames[i].c_str());
				}
			}
            int outXBlockSize = 0;
            int outYBlockSize = 0;
            outputRasterBands[0]->GetBlockSize (&outXBlockSize, &outYBlockSize);
            
            if(outYBlockSize > yBlockSize)
            {
                yBlockSize = outYBlockSize;
            }
			
			// Allocate memory
			inputIntData = new unsigned int*[numIntBands];
			for(int i = 0; i < numIntBands; i++)
			{
				inputIntData[i] = (unsigned int *) CPLMalloc(sizeof(unsigned int)*width*yBlockSize);
			}
			inDataIntColumn = new long[numIntBands];
            
            inputFloatData = new float*[numFloatBands];
			for(int i = 0; i < numFloatBands; i++)
			{
				inputFloatData[i] = (float *) CPLMalloc(sizeof(float)*width*yBlockSize);
			}
			inDataFloatColumn = new float[numFloatBands];
            
            outputData = new double*[this->numOutBands];
			for(int i = 0; i < this->numOutBands; i++)
			{
				outputData[i] = (double *) CPLMalloc(sizeof(double)*(width*yBlockSize));
			}
			outDataColumn = new double[this->numOutBands];
            
            int nYBlocks = height / yBlockSize;
            int remainRows = height - (nYBlocks * yBlockSize);
            int rowOffset = 0;
            
			int feedback = height/10;
			int feedbackCounter = 0;
			std::cout << "Started " << std::flush;
			// Loop images to process data
			for(int i = 0; i < nYBlocks; i++)
			{
				for(int n = 0; n < numIntBands; n++)
				{
                    rowOffset = bandIntOffsets[n][1] + (yBlockSize * i);
					inputRasterIntBands[n]->RasterIO(GF_Read, bandIntOffsets[n][0], rowOffset, width, yBlockSize, inputIntData[n], width, yBlockSize, GDT_UInt32, 0, 0);
				}
                
                for(int n = 0; n < numFloatBands; n++)
				{
                    rowOffset = bandFloatOffsets[n][1] + (yBlockSize * i);
					inputRasterFloatBands[n]->RasterIO(GF_Read, bandFloatOffsets[n][0], rowOffset, width, yBlockSize, inputFloatData[n], width, yBlockSize, GDT_Float32, 0, 0);
				}
                
                for(int m = 0; m < yBlockSize; ++m)
                {
                    if((feedback != 0) && ((((i*yBlockSize)+m) % feedback) == 0))
                    {
                        std::cout << "." << feedbackCounter << "." << std::flush;
                        feedbackCounter = feedbackCounter + 10;
                    }
                    
                    
                    for(int j = 0; j < width; j++)
                    {
                        for(int n = 0; n < numIntBands; n++)
                        {
                            inDataIntColumn[n] = inputIntData[n][(m*width)+j];
                        }
                        
                        for(int n = 0; n < numFloatBands; n++)
                        {
                            inDataFloatColumn[n] = inputFloatData[n][(m*width)+j];
                        }
                        
                        this->calc->calcImageValue(inDataIntColumn, numIntBands, inDataFloatColumn, numFloatBands, outDataColumn);
                        
                        for(int n = 0; n < this->numOutBands; n++)
                        {
                            outputData[n][(m*width)+j] = outDataColumn[n];
                        }
                    }
                }
                
                for(int n = 0; n < this->numOutBands; n++)
				{
                    rowOffset = yBlockSize * i;
					outputRasterBands[n]->RasterIO(GF_Write, 0, rowOffset, width, yBlockSize, outputData[n], width, yBlockSize, GDT_Float64, 0, 0);
				}
			}
            
            if(remainRows > 0)
            {
                for(int n = 0; n < numIntBands; n++)
				{
                    rowOffset = bandIntOffsets[n][1] + (yBlockSize * nYBlocks);
					inputRasterIntBands[n]->RasterIO(GF_Read, bandIntOffsets[n][0], rowOffset, width, remainRows, inputIntData[n], width, remainRows, GDT_UInt32, 0, 0);
				}
                
                for(int n = 0; n < numFloatBands; n++)
				{
                    rowOffset = bandFloatOffsets[n][1] + (yBlockSize * nYBlocks);
					inputRasterFloatBands[n]->RasterIO(GF_Read, bandFloatOffsets[n][0], rowOffset, width, remainRows, inputFloatData[n], width, remainRows, GDT_Float32, 0, 0);
				}
                
                for(int m = 0; m < remainRows; ++m)
                {
                    if((feedback != 0) && ((((nYBlocks*yBlockSize)+m) % feedback) == 0))
                    {
                        std::cout << "." << feedbackCounter << "." << std::flush;
                        feedbackCounter = feedbackCounter + 10;
                    }
                    
                    for(int j = 0; j < width; j++)
                    {
                        for(int n = 0; n < numIntBands; n++)
                        {
                            inDataIntColumn[n] = inputIntData[n][(m*width)+j];
                        }
                        
                        for(int n = 0; n < numFloatBands; n++)
                        {
                            inDataFloatColumn[n] = inputFloatData[n][(m*width)+j];
                        }
                        
                        this->calc->calcImageValue(inDataIntColumn, numIntBands, inDataFloatColumn, numFloatBands, outDataColumn);
                        
                        for(int n = 0; n < this->numOutBands; n++)
                        {
                            outputData[n][(m*width)+j] = outDataColumn[n];
                        }
                    }
                    
                    for(int n = 0; n < this->numOutBands; n++)
                    {
                        rowOffset = (yBlockSize * nYBlocks);
                        outputRasterBands[n]->RasterIO(GF_Write, 0, rowOffset, width, remainRows, outputData[n], width, remainRows, GDT_Float64, 0, 0);
                    }
                }
            }
			std::cout << " Complete.\n";
		}
		catch(RSGISImageCalcException& e)
		{
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					if(dsOffsets[i] != NULL)
					{
						delete[] dsOffsets[i];
					}
				}
				delete[] dsOffsets;
			}
			
			if(bandIntOffsets != NULL)
			{
				for(int i = 0; i < numIntBands; i++)
				{
					if(bandIntOffsets[i] != NULL)
					{
						delete[] bandIntOffsets[i];
					}
				}
				delete[] bandIntOffsets;
			}
            
            if(bandFloatOffsets != NULL)
			{
				for(int i = 0; i < numFloatBands; i++)
				{
					if(bandFloatOffsets[i] != NULL)
					{
						delete[] bandFloatOffsets[i];
					}
				}
				delete[] bandFloatOffsets;
			}
			
			if(inputIntData != NULL)
			{
				for(int i = 0; i < numIntBands; i++)
				{
					if(inputIntData[i] != NULL)
					{
						CPLFree(inputIntData[i]);
					}
				}
				delete[] inputIntData;
			}
            
            if(inputFloatData != NULL)
			{
				for(int i = 0; i < numFloatBands; i++)
				{
					if(inputFloatData[i] != NULL)
					{
						delete[] inputFloatData[i];
					}
				}
				delete[] inputFloatData;
			}
            
			if(inDataIntColumn != NULL)
			{
				delete[] inDataIntColumn;
			}
            
            if(inDataFloatColumn != NULL)
			{
				delete[] inDataFloatColumn;
			}
            
			if(inputRasterIntBands != NULL)
			{
				delete[] inputRasterIntBands;
			}
            
            if(inputRasterFloatBands != NULL)
			{
				delete[] inputRasterFloatBands;
			}
            
            if(outputData != NULL)
            {
                for(int i = 0; i < this->numOutBands; i++)
                {
                    if(outputData[i] != NULL)
                    {
                        delete[] outputData[i];
                    }
                }
                delete[] outputData;
            }
            
            if(outDataColumn != NULL)
            {
                delete[] outDataColumn;
            }
            
            if(outputRasterBands != NULL)
            {
                delete[] outputRasterBands;
            }
            
			throw e;
		}
		catch(RSGISImageBandException& e)
		{
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					if(dsOffsets[i] != NULL)
					{
						delete[] dsOffsets[i];
					}
				}
				delete[] dsOffsets;
			}
			
			if(bandIntOffsets != NULL)
			{
				for(int i = 0; i < numIntBands; i++)
				{
					if(bandIntOffsets[i] != NULL)
					{
						delete[] bandIntOffsets[i];
					}
				}
				delete[] bandIntOffsets;
			}
            
            if(bandFloatOffsets != NULL)
			{
				for(int i = 0; i < numFloatBands; i++)
				{
					if(bandFloatOffsets[i] != NULL)
					{
						delete[] bandFloatOffsets[i];
					}
				}
				delete[] bandFloatOffsets;
			}
			
			if(inputIntData != NULL)
			{
				for(int i = 0; i < numIntBands; i++)
				{
					if(inputIntData[i] != NULL)
					{
						CPLFree(inputIntData[i]);
					}
				}
				delete[] inputIntData;
			}
            
            if(inputFloatData != NULL)
			{
				for(int i = 0; i < numFloatBands; i++)
				{
					if(inputFloatData[i] != NULL)
					{
						delete[] inputFloatData[i];
					}
				}
				delete[] inputFloatData;
			}
            
			if(inDataIntColumn != NULL)
			{
				delete[] inDataIntColumn;
			}
            
            if(inDataFloatColumn != NULL)
			{
				delete[] inDataFloatColumn;
			}
            
			if(inputRasterIntBands != NULL)
			{
				delete[] inputRasterIntBands;
			}
            
            if(inputRasterFloatBands != NULL)
			{
				delete[] inputRasterFloatBands;
			}
            
            if(outputData != NULL)
            {
                for(int i = 0; i < this->numOutBands; i++)
                {
                    if(outputData[i] != NULL)
                    {
                        delete[] outputData[i];
                    }
                }
                delete[] outputData;
            }
            
            if(outDataColumn != NULL)
            {
                delete[] outDataColumn;
            }
            
            if(outputRasterBands != NULL)
            {
                delete[] outputRasterBands;
            }
            
			throw e;
		}
        
        GDALClose(outputImageDS);
        
		if(gdalTranslation != NULL)
        {
            delete[] gdalTranslation;
        }
        
        if(dsOffsets != NULL)
        {
            for(int i = 0; i < numDS; i++)
            {
                if(dsOffsets[i] != NULL)
                {
                    delete[] dsOffsets[i];
                }
            }
            delete[] dsOffsets;
        }
        
        if(bandIntOffsets != NULL)
        {
            for(int i = 0; i < numIntBands; i++)
            {
                if(bandIntOffsets[i] != NULL)
                {
                    delete[] bandIntOffsets[i];
                }
            }
            delete[] bandIntOffsets;
        }
        
        if(bandFloatOffsets != NULL)
        {
            for(int i = 0; i < numFloatBands; i++)
            {
                if(bandFloatOffsets[i] != NULL)
                {
                    delete[] bandFloatOffsets[i];
                }
            }
            delete[] bandFloatOffsets;
        }
        
        if(inputIntData != NULL)
        {
            for(int i = 0; i < numIntBands; i++)
            {
                if(inputIntData[i] != NULL)
                {
                    CPLFree(inputIntData[i]);
                }
            }
            delete[] inputIntData;
        }
        
        if(inputFloatData != NULL)
        {
            for(int i = 0; i < numFloatBands; i++)
            {
                if(inputFloatData[i] != NULL)
                {
                    delete[] inputFloatData[i];
                }
            }
            delete[] inputFloatData;
        }
        
        if(inDataIntColumn != NULL)
        {
            delete[] inDataIntColumn;
        }
        
        if(inDataFloatColumn != NULL)
        {
            delete[] inDataFloatColumn;
        }
        
        if(inputRasterIntBands != NULL)
        {
            delete[] inputRasterIntBands;
        }
        
        if(inputRasterFloatBands != NULL)
        {
            delete[] inputRasterFloatBands;
        }
        
        if(outputData != NULL)
        {
            for(int i = 0; i < this->numOutBands; i++)
            {
                if(outputData[i] != NULL)
                {
                    delete[] outputData[i];
                }
            }
            delete[] outputData;
        }
        
        if(outDataColumn != NULL)
        {
            delete[] outDataColumn;
        }
        
        if(outputRasterBands != NULL)
        {
            delete[] outputRasterBands;
        }
    }
    
    
    void RSGISCalcImage::calcImage(GDALDataset **datasets, int numIntDS, int numFloatDS, std::string outputImage, std::string outputRefIntImage, std::string gdalFormat, GDALDataType gdalDataType)
    {
        GDALAllRegister();
        RSGISImageUtils imgUtils;
        int numDS = numIntDS + numFloatDS;
        double *gdalTranslation = new double[6];
        int **dsOffsets = new int*[numDS];
        for(int i = 0; i < numDS; i++)
        {
            dsOffsets[i] = new int[2];
        }
        int **bandIntOffsets = NULL;
        int **bandFloatOffsets = NULL;
        int height = 0;
        int width = 0;
        int numIntBands = 0;
        int numFloatBands = 0;
        
        float **inputFloatData = NULL;
        float *inDataFloatColumn = NULL;
        unsigned int **inputIntData = NULL;
        long *inDataIntColumn = NULL;
        double *outputRefData = NULL;
        double outRefData = 0;
        
        double **outputData = NULL;
        double *outDataColumn = NULL;
        
        int xBlockSize = 0;
        int yBlockSize = 0;
        
        GDALRasterBand **inputRasterFloatBands = NULL;
        GDALRasterBand **inputRasterIntBands = NULL;
        
        GDALDataset *outputImageDS = NULL;
        GDALDataset *outputRefImageDS = NULL;
        GDALRasterBand **outputRasterBands = NULL;
        GDALRasterBand *outputRefRasterBand = NULL;
        GDALDriver *gdalDriver = NULL;
        
        try
        {
            // Find image overlap
            imgUtils.getImageOverlap(datasets, numDS, dsOffsets, &width, &height, gdalTranslation, &xBlockSize, &yBlockSize);
            
            
            // Count number of image bands
            for(int i = 0; i < numIntDS; i++)
            {
                numIntBands += datasets[i]->GetRasterCount();
            }
            for(int i = numIntDS; i < numDS; i++)
            {
                numFloatBands += datasets[i]->GetRasterCount();
            }
            
            // Create new Image
            gdalDriver = GetGDALDriverManager()->GetDriverByName(gdalFormat.c_str());
            if(gdalDriver == NULL)
            {
                throw RSGISImageBandException("Requested GDAL driver does not exists..");
            }
            std::cout << "New image width = " << width << " height = " << height << " bands = " << this->numOutBands << std::endl;
            
            outputImageDS = gdalDriver->Create(outputImage.c_str(), width, height, this->numOutBands, gdalDataType, NULL);
            if(outputImageDS == NULL)
            {
                throw RSGISImageBandException("Output image could not be created. Check filepath.");
            }
            outputImageDS->SetGeoTransform(gdalTranslation);
            if(useImageProj)
            {
                outputImageDS->SetProjection(datasets[0]->GetProjectionRef());
            }
            else
            {
                outputImageDS->SetProjection(proj.c_str());
            }
            
            outputRefImageDS = gdalDriver->Create(outputRefIntImage.c_str(), width, height, 1, GDT_UInt32, NULL);
            if(outputRefImageDS == NULL)
            {
                throw RSGISImageBandException("Output reference image could not be created. Check filepath.");
            }
            outputRefImageDS->SetGeoTransform(gdalTranslation);
            if(useImageProj)
            {
                outputRefImageDS->SetProjection(datasets[0]->GetProjectionRef());
            }
            else
            {
                outputRefImageDS->SetProjection(proj.c_str());
            }
            
            // Get Image Input Bands
            bandIntOffsets = new int*[numIntBands];
            inputRasterIntBands = new GDALRasterBand*[numIntBands];
            
            bandFloatOffsets = new int*[numFloatBands];
            inputRasterFloatBands = new GDALRasterBand*[numFloatBands];
            
            int counter = 0;
            for(int i = 0; i < numIntDS; i++)
            {
                for(int j = 0; j < datasets[i]->GetRasterCount(); j++)
                {
                    inputRasterIntBands[counter] = datasets[i]->GetRasterBand(j+1);
                    bandIntOffsets[counter] = new int[2];
                    bandIntOffsets[counter][0] = dsOffsets[i][0];
                    bandIntOffsets[counter][1] = dsOffsets[i][1];
                    counter++;
                }
            }
            
            counter = 0;
            for(int i = numIntDS; i < numDS; i++)
            {
                for(int j = 0; j < datasets[i]->GetRasterCount(); j++)
                {
                    inputRasterFloatBands[counter] = datasets[i]->GetRasterBand(j+1);
                    bandFloatOffsets[counter] = new int[2];
                    bandFloatOffsets[counter][0] = dsOffsets[i][0];
                    bandFloatOffsets[counter][1] = dsOffsets[i][1];
                    counter++;
                }
            }
            
            //Get Image Output Bands
            outputRasterBands = new GDALRasterBand*[this->numOutBands];
            for(int i = 0; i < this->numOutBands; i++)
            {
                outputRasterBands[i] = outputImageDS->GetRasterBand(i+1);
            }
            outputRefRasterBand = outputRefImageDS->GetRasterBand(1);
            
            int outXBlockSize = 0;
            int outYBlockSize = 0;
            outputRasterBands[0]->GetBlockSize (&outXBlockSize, &outYBlockSize);
            
            if(outYBlockSize > yBlockSize)
            {
                yBlockSize = outYBlockSize;
            }
            
            // Allocate memory
            inputIntData = new unsigned int*[numIntBands];
            for(int i = 0; i < numIntBands; i++)
            {
                inputIntData[i] = (unsigned int *) CPLMalloc(sizeof(unsigned int)*width*yBlockSize);
            }
            inDataIntColumn = new long[numIntBands];
            
            inputFloatData = new float*[numFloatBands];
            for(int i = 0; i < numFloatBands; i++)
            {
                inputFloatData[i] = (float *) CPLMalloc(sizeof(float)*width*yBlockSize);
            }
            inDataFloatColumn = new float[numFloatBands];
            
            outputData = new double*[this->numOutBands];
            for(int i = 0; i < this->numOutBands; i++)
            {
                outputData[i] = (double *) CPLMalloc(sizeof(double)*(width*yBlockSize));
            }
            outDataColumn = new double[this->numOutBands];
            outputRefData = (double *) CPLMalloc(sizeof(double)*(width*yBlockSize));
            
            int nYBlocks = height / yBlockSize;
            int remainRows = height - (nYBlocks * yBlockSize);
            int rowOffset = 0;
            
            int feedback = height/10;
            int feedbackCounter = 0;
            std::cout << "Started " << std::flush;
            // Loop images to process data
            for(int i = 0; i < nYBlocks; i++)
            {
                for(int n = 0; n < numIntBands; n++)
                {
                    rowOffset = bandIntOffsets[n][1] + (yBlockSize * i);
                    inputRasterIntBands[n]->RasterIO(GF_Read, bandIntOffsets[n][0], rowOffset, width, yBlockSize, inputIntData[n], width, yBlockSize, GDT_UInt32, 0, 0);
                }
                
                for(int n = 0; n < numFloatBands; n++)
                {
                    rowOffset = bandFloatOffsets[n][1] + (yBlockSize * i);
                    inputRasterFloatBands[n]->RasterIO(GF_Read, bandFloatOffsets[n][0], rowOffset, width, yBlockSize, inputFloatData[n], width, yBlockSize, GDT_Float32, 0, 0);
                }
                
                for(int m = 0; m < yBlockSize; ++m)
                {
                    if((feedback != 0) && ((((i*yBlockSize)+m) % feedback) == 0))
                    {
                        std::cout << "." << feedbackCounter << "." << std::flush;
                        feedbackCounter = feedbackCounter + 10;
                    }
                    
                    
                    for(int j = 0; j < width; j++)
                    {
                        for(int n = 0; n < numIntBands; n++)
                        {
                            inDataIntColumn[n] = inputIntData[n][(m*width)+j];
                        }
                        
                        for(int n = 0; n < numFloatBands; n++)
                        {
                            inDataFloatColumn[n] = inputFloatData[n][(m*width)+j];
                        }
                        
                        this->calc->calcImageValue(inDataIntColumn, numIntBands, inDataFloatColumn, numFloatBands, outDataColumn, &outRefData, 1);
                        
                        for(int n = 0; n < this->numOutBands; n++)
                        {
                            outputData[n][(m*width)+j] = outDataColumn[n];
                        }
                        outputRefData[(m*width)+j] = outRefData;
                    }
                }
                
                for(int n = 0; n < this->numOutBands; n++)
                {
                    rowOffset = yBlockSize * i;
                    outputRasterBands[n]->RasterIO(GF_Write, 0, rowOffset, width, yBlockSize, outputData[n], width, yBlockSize, GDT_Float64, 0, 0);
                }
                outputRefRasterBand->RasterIO(GF_Write, 0, rowOffset, width, yBlockSize, outputRefData, width, yBlockSize, GDT_Float64, 0, 0);
            }
            
            if(remainRows > 0)
            {
                for(int n = 0; n < numIntBands; n++)
                {
                    rowOffset = bandIntOffsets[n][1] + (yBlockSize * nYBlocks);
                    inputRasterIntBands[n]->RasterIO(GF_Read, bandIntOffsets[n][0], rowOffset, width, remainRows, inputIntData[n], width, remainRows, GDT_UInt32, 0, 0);
                }
                
                for(int n = 0; n < numFloatBands; n++)
                {
                    rowOffset = bandFloatOffsets[n][1] + (yBlockSize * nYBlocks);
                    inputRasterFloatBands[n]->RasterIO(GF_Read, bandFloatOffsets[n][0], rowOffset, width, remainRows, inputFloatData[n], width, remainRows, GDT_Float32, 0, 0);
                }
                
                for(int m = 0; m < remainRows; ++m)
                {
                    if((feedback != 0) && ((((nYBlocks*yBlockSize)+m) % feedback) == 0))
                    {
                        std::cout << "." << feedbackCounter << "." << std::flush;
                        feedbackCounter = feedbackCounter + 10;
                    }
                    
                    for(int j = 0; j < width; j++)
                    {
                        for(int n = 0; n < numIntBands; n++)
                        {
                            inDataIntColumn[n] = inputIntData[n][(m*width)+j];
                        }
                        
                        for(int n = 0; n < numFloatBands; n++)
                        {
                            inDataFloatColumn[n] = inputFloatData[n][(m*width)+j];
                        }
                        
                        this->calc->calcImageValue(inDataIntColumn, numIntBands, inDataFloatColumn, numFloatBands, outDataColumn, &outRefData, 1);
                        
                        for(int n = 0; n < this->numOutBands; n++)
                        {
                            outputData[n][(m*width)+j] = outDataColumn[n];
                        }
                        outputRefData[(m*width)+j] = outRefData;
                    }
                    
                    for(int n = 0; n < this->numOutBands; n++)
                    {
                        rowOffset = (yBlockSize * nYBlocks);
                        outputRasterBands[n]->RasterIO(GF_Write, 0, rowOffset, width, remainRows, outputData[n], width, remainRows, GDT_Float64, 0, 0);
                    }
                    outputRefRasterBand->RasterIO(GF_Write, 0, rowOffset, width, remainRows, outputRefData, width, remainRows, GDT_Float64, 0, 0);
                }
            }
            std::cout << " Complete.\n";
        }
        catch(RSGISImageCalcException& e)
        {
            if(gdalTranslation != NULL)
            {
                delete[] gdalTranslation;
            }
            
            if(dsOffsets != NULL)
            {
                for(int i = 0; i < numDS; i++)
                {
                    if(dsOffsets[i] != NULL)
                    {
                        delete[] dsOffsets[i];
                    }
                }
                delete[] dsOffsets;
            }
            
            if(bandIntOffsets != NULL)
            {
                for(int i = 0; i < numIntBands; i++)
                {
                    if(bandIntOffsets[i] != NULL)
                    {
                        delete[] bandIntOffsets[i];
                    }
                }
                delete[] bandIntOffsets;
            }
            
            if(bandFloatOffsets != NULL)
            {
                for(int i = 0; i < numFloatBands; i++)
                {
                    if(bandFloatOffsets[i] != NULL)
                    {
                        delete[] bandFloatOffsets[i];
                    }
                }
                delete[] bandFloatOffsets;
            }
            
            if(inputIntData != NULL)
            {
                for(int i = 0; i < numIntBands; i++)
                {
                    if(inputIntData[i] != NULL)
                    {
                        CPLFree(inputIntData[i]);
                    }
                }
                delete[] inputIntData;
            }
            
            if(inputFloatData != NULL)
            {
                for(int i = 0; i < numFloatBands; i++)
                {
                    if(inputFloatData[i] != NULL)
                    {
                        delete[] inputFloatData[i];
                    }
                }
                delete[] inputFloatData;
            }
            
            if(inDataIntColumn != NULL)
            {
                delete[] inDataIntColumn;
            }
            
            if(inDataFloatColumn != NULL)
            {
                delete[] inDataFloatColumn;
            }
            
            if(inputRasterIntBands != NULL)
            {
                delete[] inputRasterIntBands;
            }
            
            if(inputRasterFloatBands != NULL)
            {
                delete[] inputRasterFloatBands;
            }
            
            if(outputData != NULL)
            {
                for(int i = 0; i < this->numOutBands; i++)
                {
                    if(outputData[i] != NULL)
                    {
                        delete[] outputData[i];
                    }
                }
                delete[] outputData;
            }
            
            if(outDataColumn != NULL)
            {
                delete[] outDataColumn;
            }
            
            if(outputRasterBands != NULL)
            {
                delete[] outputRasterBands;
            }
            
            throw e;
        }
        catch(RSGISImageBandException& e)
        {
            if(gdalTranslation != NULL)
            {
                delete[] gdalTranslation;
            }
            
            if(dsOffsets != NULL)
            {
                for(int i = 0; i < numDS; i++)
                {
                    if(dsOffsets[i] != NULL)
                    {
                        delete[] dsOffsets[i];
                    }
                }
                delete[] dsOffsets;
            }
            
            if(bandIntOffsets != NULL)
            {
                for(int i = 0; i < numIntBands; i++)
                {
                    if(bandIntOffsets[i] != NULL)
                    {
                        delete[] bandIntOffsets[i];
                    }
                }
                delete[] bandIntOffsets;
            }
            
            if(bandFloatOffsets != NULL)
            {
                for(int i = 0; i < numFloatBands; i++)
                {
                    if(bandFloatOffsets[i] != NULL)
                    {
                        delete[] bandFloatOffsets[i];
                    }
                }
                delete[] bandFloatOffsets;
            }
            
            if(inputIntData != NULL)
            {
                for(int i = 0; i < numIntBands; i++)
                {
                    if(inputIntData[i] != NULL)
                    {
                        CPLFree(inputIntData[i]);
                    }
                }
                delete[] inputIntData;
            }
            
            if(inputFloatData != NULL)
            {
                for(int i = 0; i < numFloatBands; i++)
                {
                    if(inputFloatData[i] != NULL)
                    {
                        delete[] inputFloatData[i];
                    }
                }
                delete[] inputFloatData;
            }
            
            if(inDataIntColumn != NULL)
            {
                delete[] inDataIntColumn;
            }
            
            if(inDataFloatColumn != NULL)
            {
                delete[] inDataFloatColumn;
            }
            
            if(inputRasterIntBands != NULL)
            {
                delete[] inputRasterIntBands;
            }
            
            if(inputRasterFloatBands != NULL)
            {
                delete[] inputRasterFloatBands;
            }
            
            if(outputData != NULL)
            {
                for(int i = 0; i < this->numOutBands; i++)
                {
                    if(outputData[i] != NULL)
                    {
                        delete[] outputData[i];
                    }
                }
                delete[] outputData;
            }
            
            if(outDataColumn != NULL)
            {
                delete[] outDataColumn;
            }
            
            if(outputRasterBands != NULL)
            {
                delete[] outputRasterBands;
            }
            
            throw e;
        }
        
        GDALClose(outputImageDS);
        
        if(gdalTranslation != NULL)
        {
            delete[] gdalTranslation;
        }
        
        if(dsOffsets != NULL)
        {
            for(int i = 0; i < numDS; i++)
            {
                if(dsOffsets[i] != NULL)
                {
                    delete[] dsOffsets[i];
                }
            }
            delete[] dsOffsets;
        }
        
        if(bandIntOffsets != NULL)
        {
            for(int i = 0; i < numIntBands; i++)
            {
                if(bandIntOffsets[i] != NULL)
                {
                    delete[] bandIntOffsets[i];
                }
            }
            delete[] bandIntOffsets;
        }
        
        if(bandFloatOffsets != NULL)
        {
            for(int i = 0; i < numFloatBands; i++)
            {
                if(bandFloatOffsets[i] != NULL)
                {
                    delete[] bandFloatOffsets[i];
                }
            }
            delete[] bandFloatOffsets;
        }
        
        if(inputIntData != NULL)
        {
            for(int i = 0; i < numIntBands; i++)
            {
                if(inputIntData[i] != NULL)
                {
                    CPLFree(inputIntData[i]);
                }
            }
            delete[] inputIntData;
        }
        
        if(inputFloatData != NULL)
        {
            for(int i = 0; i < numFloatBands; i++)
            {
                if(inputFloatData[i] != NULL)
                {
                    delete[] inputFloatData[i];
                }
            }
            delete[] inputFloatData;
        }
        
        if(inDataIntColumn != NULL)
        {
            delete[] inDataIntColumn;
        }
        
        if(inDataFloatColumn != NULL)
        {
            delete[] inDataFloatColumn;
        }
        
        if(inputRasterIntBands != NULL)
        {
            delete[] inputRasterIntBands;
        }
        
        if(inputRasterFloatBands != NULL)
        {
            delete[] inputRasterFloatBands;
        }
        
        if(outputData != NULL)
        {
            for(int i = 0; i < this->numOutBands; i++)
            {
                if(outputData[i] != NULL)
                {
                    delete[] outputData[i];
                }
            }
            delete[] outputData;
        }
        
        if(outDataColumn != NULL)
        {
            delete[] outDataColumn;
        }
        
        if(outputRasterBands != NULL)
        {
            delete[] outputRasterBands;
        }
    }
    
    
    
    
    
    
    void RSGISCalcImage::calcImage(GDALDataset **datasets, int numIntDS, int numFloatDS, geos::geom::Envelope *env, bool quiet)
    {
        GDALAllRegister();
		RSGISImageUtils imgUtils;
        int numDS = numIntDS + numFloatDS;
		double *gdalTranslation = new double[6];
		int **dsOffsets = new int*[numDS];
		for(int i = 0; i < numDS; i++)
		{
			dsOffsets[i] = new int[2];
		}
		int **bandIntOffsets = NULL;
        int **bandFloatOffsets = NULL;
		int height = 0;
		int width = 0;
		int numIntBands = 0;
        int numFloatBands = 0;
		
		float **inputFloatData = NULL;
		float *inDataFloatColumn = NULL;
        unsigned int **inputIntData = NULL;
		long *inDataIntColumn = NULL;
        int xBlockSize = 0;
        int yBlockSize = 0;
		
		GDALRasterBand **inputRasterFloatBands = NULL;
        GDALRasterBand **inputRasterIntBands = NULL;
		
		try
		{
			// Find image overlap
            if(env == NULL)
            {
                imgUtils.getImageOverlap(datasets, numDS, dsOffsets, &width, &height, gdalTranslation, &xBlockSize, &yBlockSize);
            }
			else
            {
                imgUtils.getImageOverlapCut2Env(datasets, numDS, dsOffsets, &width, &height, gdalTranslation, env, &xBlockSize, &yBlockSize);
            }
            
			// Count number of image bands
			for(int i = 0; i < numIntDS; i++)
			{
				numIntBands += datasets[i]->GetRasterCount();
			}
            for(int i = numIntDS; i < numDS; i++)
			{
				numFloatBands += datasets[i]->GetRasterCount();
			}
			
			// Get Image Input Bands
			bandIntOffsets = new int*[numIntBands];
			inputRasterIntBands = new GDALRasterBand*[numIntBands];
            
            bandFloatOffsets = new int*[numFloatBands];
			inputRasterFloatBands = new GDALRasterBand*[numFloatBands];
            
			int counter = 0;
			for(int i = 0; i < numIntDS; i++)
			{
				for(int j = 0; j < datasets[i]->GetRasterCount(); j++)
				{
					inputRasterIntBands[counter] = datasets[i]->GetRasterBand(j+1);
					bandIntOffsets[counter] = new int[2];
					bandIntOffsets[counter][0] = dsOffsets[i][0];
					bandIntOffsets[counter][1] = dsOffsets[i][1];
					counter++;
				}
			}
            
            counter = 0;
            for(int i = numIntDS; i < numDS; i++)
			{
				for(int j = 0; j < datasets[i]->GetRasterCount(); j++)
				{
					inputRasterFloatBands[counter] = datasets[i]->GetRasterBand(j+1);
					bandFloatOffsets[counter] = new int[2];
					bandFloatOffsets[counter][0] = dsOffsets[i][0];
					bandFloatOffsets[counter][1] = dsOffsets[i][1];
					counter++;
				}
			}
			
			// Allocate memory
			inputIntData = new unsigned int*[numIntBands];
			for(int i = 0; i < numIntBands; i++)
			{
				inputIntData[i] = (unsigned int *) CPLMalloc(sizeof(unsigned int)*width*yBlockSize);
			}
			inDataIntColumn = new long[numIntBands];
            
            inputFloatData = new float*[numFloatBands];
			for(int i = 0; i < numFloatBands; i++)
			{
				inputFloatData[i] = (float *) CPLMalloc(sizeof(float)*width*yBlockSize);
			}
			inDataFloatColumn = new float[numFloatBands];
            
            
            int nYBlocks = height / yBlockSize;
            int remainRows = height - (nYBlocks * yBlockSize);
            int rowOffset = 0;
            
			int feedback = height/10;
			int feedbackCounter = 0;
            if(!quiet)
            {
                std::cout << "Started " << std::flush;
            }
			// Loop images to process data
			for(int i = 0; i < nYBlocks; i++)
			{
				for(int n = 0; n < numIntBands; n++)
				{
                    rowOffset = bandIntOffsets[n][1] + (yBlockSize * i);
					inputRasterIntBands[n]->RasterIO(GF_Read, bandIntOffsets[n][0], rowOffset, width, yBlockSize, inputIntData[n], width, yBlockSize, GDT_UInt32, 0, 0);
				}
                
                for(int n = 0; n < numFloatBands; n++)
				{
                    rowOffset = bandFloatOffsets[n][1] + (yBlockSize * i);
					inputRasterFloatBands[n]->RasterIO(GF_Read, bandFloatOffsets[n][0], rowOffset, width, yBlockSize, inputFloatData[n], width, yBlockSize, GDT_Float32, 0, 0);
				}
                
                for(int m = 0; m < yBlockSize; ++m)
                {
                    if((!quiet) && (feedback != 0) && ((((i*yBlockSize)+m) % feedback) == 0))
                    {
                        std::cout << "." << feedbackCounter << "." << std::flush;
                        feedbackCounter = feedbackCounter + 10;
                    }
                    
                    
                    for(int j = 0; j < width; j++)
                    {
                        for(int n = 0; n < numIntBands; n++)
                        {
                            inDataIntColumn[n] = inputIntData[n][(m*width)+j];
                        }
                        
                        for(int n = 0; n < numFloatBands; n++)
                        {
                            inDataFloatColumn[n] = inputFloatData[n][(m*width)+j];
                        }
                        
                        this->calc->calcImageValue(inDataIntColumn, numIntBands, inDataFloatColumn, numFloatBands);
                    }
                }
			}
            
            if(remainRows > 0)
            {
                for(int n = 0; n < numIntBands; n++)
				{
                    rowOffset = bandIntOffsets[n][1] + (yBlockSize * nYBlocks);
					inputRasterIntBands[n]->RasterIO(GF_Read, bandIntOffsets[n][0], rowOffset, width, remainRows, inputIntData[n], width, remainRows, GDT_UInt32, 0, 0);
				}
                
                
                for(int n = 0; n < numFloatBands; n++)
				{
                    rowOffset = bandFloatOffsets[n][1] + (yBlockSize * nYBlocks);
					inputRasterFloatBands[n]->RasterIO(GF_Read, bandFloatOffsets[n][0], rowOffset, width, remainRows, inputFloatData[n], width, remainRows, GDT_Float32, 0, 0);
				}
                
                for(int m = 0; m < remainRows; ++m)
                {
                    if((!quiet) && (feedback != 0) && ((((nYBlocks*yBlockSize)+m) % feedback) == 0))
                    {
                        std::cout << "." << feedbackCounter << "." << std::flush;
                        feedbackCounter = feedbackCounter + 10;
                    }
                    
                    for(int j = 0; j < width; j++)
                    {
                        for(int n = 0; n < numIntBands; n++)
                        {
                            inDataIntColumn[n] = inputIntData[n][(m*width)+j];
                        }
                        
                        for(int n = 0; n < numFloatBands; n++)
                        {
                            inDataFloatColumn[n] = inputFloatData[n][(m*width)+j];
                        }
                        
                        this->calc->calcImageValue(inDataIntColumn, numIntBands, inDataFloatColumn, numFloatBands);
                    }
                }
            }
            if(!quiet)
            {
                std::cout << " Complete.\n";
            }
		}
		catch(RSGISImageCalcException& e)
		{
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					if(dsOffsets[i] != NULL)
					{
						delete[] dsOffsets[i];
					}
				}
				delete[] dsOffsets;
			}
			
			if(bandIntOffsets != NULL)
			{
				for(int i = 0; i < numIntBands; i++)
				{
					if(bandIntOffsets[i] != NULL)
					{
						delete[] bandIntOffsets[i];
					}
				}
				delete[] bandIntOffsets;
			}
            
            if(bandFloatOffsets != NULL)
			{
				for(int i = 0; i < numFloatBands; i++)
				{
					if(bandFloatOffsets[i] != NULL)
					{
						delete[] bandFloatOffsets[i];
					}
				}
				delete[] bandFloatOffsets;
			}
			
			if(inputIntData != NULL)
			{
				for(int i = 0; i < numIntBands; i++)
				{
					if(inputIntData[i] != NULL)
					{
						CPLFree(inputIntData[i]);
					}
				}
				delete[] inputIntData;
			}
            
            if(inputFloatData != NULL)
			{
				for(int i = 0; i < numFloatBands; i++)
				{
					if(inputFloatData[i] != NULL)
					{
						delete[] inputFloatData[i];
					}
				}
				delete[] inputFloatData;
			}
            
			if(inDataIntColumn != NULL)
			{
				delete[] inDataIntColumn;
			}
            
            if(inDataFloatColumn != NULL)
			{
				delete[] inDataFloatColumn;
			}
            
			if(inputRasterIntBands != NULL)
			{
				delete[] inputRasterIntBands;
			}
            
            if(inputRasterFloatBands != NULL)
			{
				delete[] inputRasterFloatBands;
			}
			throw e;
		}
		catch(RSGISImageBandException& e)
		{
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					if(dsOffsets[i] != NULL)
					{
						delete[] dsOffsets[i];
					}
				}
				delete[] dsOffsets;
			}
			
			if(bandIntOffsets != NULL)
			{
				for(int i = 0; i < numIntBands; i++)
				{
					if(bandIntOffsets[i] != NULL)
					{
						delete[] bandIntOffsets[i];
					}
				}
				delete[] bandIntOffsets;
			}
            
            if(bandFloatOffsets != NULL)
			{
				for(int i = 0; i < numFloatBands; i++)
				{
					if(bandFloatOffsets[i] != NULL)
					{
						delete[] bandFloatOffsets[i];
					}
				}
				delete[] bandFloatOffsets;
			}
			
			if(inputIntData != NULL)
			{
				for(int i = 0; i < numIntBands; i++)
				{
					if(inputIntData[i] != NULL)
					{
						CPLFree(inputIntData[i]);
					}
				}
				delete[] inputIntData;
			}
            
            if(inputFloatData != NULL)
			{
				for(int i = 0; i < numFloatBands; i++)
				{
					if(inputFloatData[i] != NULL)
					{
						delete[] inputFloatData[i];
					}
				}
				delete[] inputFloatData;
			}
            
			if(inDataIntColumn != NULL)
			{
				delete[] inDataIntColumn;
			}
            
            if(inDataFloatColumn != NULL)
			{
				delete[] inDataFloatColumn;
			}
            
			if(inputRasterIntBands != NULL)
			{
				delete[] inputRasterIntBands;
			}
            
            if(inputRasterFloatBands != NULL)
			{
				delete[] inputRasterFloatBands;
			}
			throw e;
		}
        
		if(gdalTranslation != NULL)
        {
            delete[] gdalTranslation;
        }
        
        if(dsOffsets != NULL)
        {
            for(int i = 0; i < numDS; i++)
            {
                if(dsOffsets[i] != NULL)
                {
                    delete[] dsOffsets[i];
                }
            }
            delete[] dsOffsets;
        }
        
        if(bandIntOffsets != NULL)
        {
            for(int i = 0; i < numIntBands; i++)
            {
                if(bandIntOffsets[i] != NULL)
                {
                    delete[] bandIntOffsets[i];
                }
            }
            delete[] bandIntOffsets;
        }
        
        if(bandFloatOffsets != NULL)
        {
            for(int i = 0; i < numFloatBands; i++)
            {
                if(bandFloatOffsets[i] != NULL)
                {
                    delete[] bandFloatOffsets[i];
                }
            }
            delete[] bandFloatOffsets;
        }
        
        if(inputIntData != NULL)
        {
            for(int i = 0; i < numIntBands; i++)
            {
                if(inputIntData[i] != NULL)
                {
                    CPLFree(inputIntData[i]);
                }
            }
            delete[] inputIntData;
        }
        
        if(inputFloatData != NULL)
        {
            for(int i = 0; i < numFloatBands; i++)
            {
                if(inputFloatData[i] != NULL)
                {
                    delete[] inputFloatData[i];
                }
            }
            delete[] inputFloatData;
        }
        
        if(inDataIntColumn != NULL)
        {
            delete[] inDataIntColumn;
        }
        
        if(inDataFloatColumn != NULL)
        {
            delete[] inDataFloatColumn;
        }
        
        if(inputRasterIntBands != NULL)
        {
            delete[] inputRasterIntBands;
        }
        
        if(inputRasterFloatBands != NULL)
        {
            delete[] inputRasterFloatBands;
        }
    }
	
    void RSGISCalcImage::calcImage(GDALDataset **datasets, int numIntDS, int numFloatDS, GDALDataset *outputImageDS)
    {
        GDALAllRegister();
		RSGISImageUtils imgUtils;
        int numDS = numIntDS + numFloatDS;
		double *gdalTranslation = new double[6];
		int **dsOffsets = new int*[numDS];
		for(int i = 0; i < numDS; i++)
		{
			dsOffsets[i] = new int[2];
		}
		int **bandIntOffsets = NULL;
        int **bandFloatOffsets = NULL;
		int height = 0;
		int width = 0;
		int numIntBands = 0;
        int numFloatBands = 0;
		
		float **inputFloatData = NULL;
		float *inDataFloatColumn = NULL;
        unsigned int **inputIntData = NULL;
		long *inDataIntColumn = NULL;
        
        double **outputData = NULL;
		double *outDataColumn = NULL;
        
        int xBlockSize = 0;
        int yBlockSize = 0;
		
		GDALRasterBand **inputRasterFloatBands = NULL;
        GDALRasterBand **inputRasterIntBands = NULL;
        
		GDALRasterBand **outputRasterBands = NULL;
		
		try
		{
			// Find image overlap
			imgUtils.getImageOverlap(datasets, numDS, dsOffsets, &width, &height, gdalTranslation, &xBlockSize, &yBlockSize);
            
            if(outputImageDS->GetRasterXSize() != width)
            {
                throw RSGISImageCalcException("The output dataset does not have the correct width\n");
            }
            
            if(outputImageDS->GetRasterYSize() != height)
            {
                throw RSGISImageCalcException("The output dataset does not have the correct height\n");
            }
            
            if(outputImageDS->GetRasterCount() != this->numOutBands)
            {
                throw RSGISImageCalcException("The output dataset does not have the correct number of image bands\n");
            }
            
			// Count number of image bands
			for(int i = 0; i < numIntDS; i++)
			{
				numIntBands += datasets[i]->GetRasterCount();
			}
            for(int i = numIntDS; i < numDS; i++)
			{
				numFloatBands += datasets[i]->GetRasterCount();
			}
			
            // Get Image Input Bands
			bandIntOffsets = new int*[numIntBands];
			inputRasterIntBands = new GDALRasterBand*[numIntBands];
            
            bandFloatOffsets = new int*[numFloatBands];
			inputRasterFloatBands = new GDALRasterBand*[numFloatBands];
            
			int counter = 0;
			for(int i = 0; i < numIntDS; i++)
			{
				for(int j = 0; j < datasets[i]->GetRasterCount(); j++)
				{
					inputRasterIntBands[counter] = datasets[i]->GetRasterBand(j+1);
					bandIntOffsets[counter] = new int[2];
					bandIntOffsets[counter][0] = dsOffsets[i][0];
					bandIntOffsets[counter][1] = dsOffsets[i][1];
					counter++;
				}
			}
            
            counter = 0;
            for(int i = numIntDS; i < numDS; i++)
			{
				for(int j = 0; j < datasets[i]->GetRasterCount(); j++)
				{
					inputRasterFloatBands[counter] = datasets[i]->GetRasterBand(j+1);
					bandFloatOffsets[counter] = new int[2];
					bandFloatOffsets[counter][0] = dsOffsets[i][0];
					bandFloatOffsets[counter][1] = dsOffsets[i][1];
					counter++;
				}
			}
            
            //Get Image Output Bands
			outputRasterBands = new GDALRasterBand*[this->numOutBands];
			for(int i = 0; i < this->numOutBands; i++)
			{
				outputRasterBands[i] = outputImageDS->GetRasterBand(i+1);
			}
            int outXBlockSize = 0;
            int outYBlockSize = 0;
            outputRasterBands[0]->GetBlockSize (&outXBlockSize, &outYBlockSize);
            
            if(outYBlockSize > yBlockSize)
            {
                yBlockSize = outYBlockSize;
            }
			
			// Allocate memory
			inputIntData = new unsigned int*[numIntBands];
			for(int i = 0; i < numIntBands; i++)
			{
				inputIntData[i] = (unsigned int *) CPLMalloc(sizeof(unsigned int)*width*yBlockSize);
			}
			inDataIntColumn = new long[numIntBands];
            
            inputFloatData = new float*[numFloatBands];
			for(int i = 0; i < numFloatBands; i++)
			{
				inputFloatData[i] = (float *) CPLMalloc(sizeof(float)*width*yBlockSize);
			}
			inDataFloatColumn = new float[numFloatBands];
            
            outputData = new double*[this->numOutBands];
			for(int i = 0; i < this->numOutBands; i++)
			{
				outputData[i] = (double *) CPLMalloc(sizeof(double)*(width*yBlockSize));
			}
			outDataColumn = new double[this->numOutBands];
            
            int nYBlocks = height / yBlockSize;
            int remainRows = height - (nYBlocks * yBlockSize);
            int rowOffset = 0;
            
			int feedback = height/10;
			int feedbackCounter = 0;
			std::cout << "Started " << std::flush;
            std::cout.precision(20);
			// Loop images to process data
			for(int i = 0; i < nYBlocks; i++)
			{
				for(int n = 0; n < numIntBands; n++)
				{
                    rowOffset = bandIntOffsets[n][1] + (yBlockSize * i);
					inputRasterIntBands[n]->RasterIO(GF_Read, bandIntOffsets[n][0], rowOffset, width, yBlockSize, inputIntData[n], width, yBlockSize, GDT_UInt32, 0, 0);
				}
                
                for(int n = 0; n < numFloatBands; n++)
				{
                    rowOffset = bandFloatOffsets[n][1] + (yBlockSize * i);
					inputRasterFloatBands[n]->RasterIO(GF_Read, bandFloatOffsets[n][0], rowOffset, width, yBlockSize, inputFloatData[n], width, yBlockSize, GDT_Float32, 0, 0);
				}
                
                for(int m = 0; m < yBlockSize; ++m)
                {
                    if((feedback != 0) && ((((i*yBlockSize)+m) % feedback) == 0))
                    {
                        std::cout << "." << feedbackCounter << "." << std::flush;
                        feedbackCounter = feedbackCounter + 10;
                    }
                    
                    
                    for(int j = 0; j < width; j++)
                    {
                        for(int n = 0; n < numIntBands; n++)
                        {
                            inDataIntColumn[n] = inputIntData[n][(m*width)+j];
                        }
                        
                        for(int n = 0; n < numFloatBands; n++)
                        {
                            inDataFloatColumn[n] = inputFloatData[n][(m*width)+j];
                        }
                        
                        this->calc->calcImageValue(inDataIntColumn, numIntBands, inDataFloatColumn, numFloatBands, outDataColumn);
                        
                        for(int n = 0; n < this->numOutBands; n++)
                        {
                            outputData[n][(m*width)+j] = outDataColumn[n];
                        }
                    }
                }
                
                for(int n = 0; n < this->numOutBands; n++)
				{
                    rowOffset = yBlockSize * i;
					outputRasterBands[n]->RasterIO(GF_Write, 0, rowOffset, width, yBlockSize, outputData[n], width, yBlockSize, GDT_Float64, 0, 0);
				}
			}
            
            if(remainRows > 0)
            {
                for(int n = 0; n < numIntBands; n++)
				{
                    rowOffset = bandIntOffsets[n][1] + (yBlockSize * nYBlocks);
					inputRasterIntBands[n]->RasterIO(GF_Read, bandIntOffsets[n][0], rowOffset, width, remainRows, inputIntData[n], width, remainRows, GDT_UInt32, 0, 0);
				}
                
                for(int n = 0; n < numFloatBands; n++)
				{
                    rowOffset = bandFloatOffsets[n][1] + (yBlockSize * nYBlocks);
					inputRasterFloatBands[n]->RasterIO(GF_Read, bandFloatOffsets[n][0], rowOffset, width, remainRows, inputFloatData[n], width, remainRows, GDT_Float32, 0, 0);
				}
                
                for(int m = 0; m < remainRows; ++m)
                {
                    if((feedback != 0) && ((((nYBlocks*yBlockSize)+m) % feedback) == 0))
                    {
                        std::cout << "." << feedbackCounter << "." << std::flush;
                        feedbackCounter = feedbackCounter + 10;
                    }
                    
                    for(int j = 0; j < width; j++)
                    {
                        for(int n = 0; n < numIntBands; n++)
                        {
                            inDataIntColumn[n] = inputIntData[n][(m*width)+j];
                        }
                        
                        for(int n = 0; n < numFloatBands; n++)
                        {
                            inDataFloatColumn[n] = inputFloatData[n][(m*width)+j];
                        }
                        
                        this->calc->calcImageValue(inDataIntColumn, numIntBands, inDataFloatColumn, numFloatBands, outDataColumn);
                        
                        for(int n = 0; n < this->numOutBands; n++)
                        {
                            outputData[n][(m*width)+j] = outDataColumn[n];
                        }
                    }
                    
                    for(int n = 0; n < this->numOutBands; n++)
                    {
                        rowOffset = (yBlockSize * nYBlocks);
                        outputRasterBands[n]->RasterIO(GF_Write, 0, rowOffset, width, remainRows, outputData[n], width, remainRows, GDT_Float64, 0, 0);
                    }
                }
            }
			std::cout << " Complete.\n";
		}
		catch(RSGISImageCalcException& e)
		{
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					if(dsOffsets[i] != NULL)
					{
						delete[] dsOffsets[i];
					}
				}
				delete[] dsOffsets;
			}
			
			if(bandIntOffsets != NULL)
			{
				for(int i = 0; i < numIntBands; i++)
				{
					if(bandIntOffsets[i] != NULL)
					{
						delete[] bandIntOffsets[i];
					}
				}
				delete[] bandIntOffsets;
			}
            
            if(bandFloatOffsets != NULL)
			{
				for(int i = 0; i < numFloatBands; i++)
				{
					if(bandFloatOffsets[i] != NULL)
					{
						delete[] bandFloatOffsets[i];
					}
				}
				delete[] bandFloatOffsets;
			}
			
			if(inputIntData != NULL)
			{
				for(int i = 0; i < numIntBands; i++)
				{
					if(inputIntData[i] != NULL)
					{
						CPLFree(inputIntData[i]);
					}
				}
				delete[] inputIntData;
			}
            
            if(inputFloatData != NULL)
			{
				for(int i = 0; i < numFloatBands; i++)
				{
					if(inputFloatData[i] != NULL)
					{
						delete[] inputFloatData[i];
					}
				}
				delete[] inputFloatData;
			}
            
			if(inDataIntColumn != NULL)
			{
				delete[] inDataIntColumn;
			}
            
            if(inDataFloatColumn != NULL)
			{
				delete[] inDataFloatColumn;
			}
            
			if(inputRasterIntBands != NULL)
			{
				delete[] inputRasterIntBands;
			}
            
            if(inputRasterFloatBands != NULL)
			{
				delete[] inputRasterFloatBands;
			}
            
            if(outputData != NULL)
            {
                for(int i = 0; i < this->numOutBands; i++)
                {
                    if(outputData[i] != NULL)
                    {
                        delete[] outputData[i];
                    }
                }
                delete[] outputData;
            }
            
            if(outDataColumn != NULL)
            {
                delete[] outDataColumn;
            }
            
            if(outputRasterBands != NULL)
            {
                delete[] outputRasterBands;
            }
            
			throw e;
		}
		catch(RSGISImageBandException& e)
		{
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					if(dsOffsets[i] != NULL)
					{
						delete[] dsOffsets[i];
					}
				}
				delete[] dsOffsets;
			}
			
			if(bandIntOffsets != NULL)
			{
				for(int i = 0; i < numIntBands; i++)
				{
					if(bandIntOffsets[i] != NULL)
					{
						delete[] bandIntOffsets[i];
					}
				}
				delete[] bandIntOffsets;
			}
            
            if(bandFloatOffsets != NULL)
			{
				for(int i = 0; i < numFloatBands; i++)
				{
					if(bandFloatOffsets[i] != NULL)
					{
						delete[] bandFloatOffsets[i];
					}
				}
				delete[] bandFloatOffsets;
			}
			
			if(inputIntData != NULL)
			{
				for(int i = 0; i < numIntBands; i++)
				{
					if(inputIntData[i] != NULL)
					{
						CPLFree(inputIntData[i]);
					}
				}
				delete[] inputIntData;
			}
            
            if(inputFloatData != NULL)
			{
				for(int i = 0; i < numFloatBands; i++)
				{
					if(inputFloatData[i] != NULL)
					{
						delete[] inputFloatData[i];
					}
				}
				delete[] inputFloatData;
			}
            
			if(inDataIntColumn != NULL)
			{
				delete[] inDataIntColumn;
			}
            
            if(inDataFloatColumn != NULL)
			{
				delete[] inDataFloatColumn;
			}
            
			if(inputRasterIntBands != NULL)
			{
				delete[] inputRasterIntBands;
			}
            
            if(inputRasterFloatBands != NULL)
			{
				delete[] inputRasterFloatBands;
			}
            
            if(outputData != NULL)
            {
                for(int i = 0; i < this->numOutBands; i++)
                {
                    if(outputData[i] != NULL)
                    {
                        delete[] outputData[i];
                    }
                }
                delete[] outputData;
            }
            
            if(outDataColumn != NULL)
            {
                delete[] outDataColumn;
            }
            
            if(outputRasterBands != NULL)
            {
                delete[] outputRasterBands;
            }
            
			throw e;
		}
        
		if(gdalTranslation != NULL)
        {
            delete[] gdalTranslation;
        }
        
        if(dsOffsets != NULL)
        {
            for(int i = 0; i < numDS; i++)
            {
                if(dsOffsets[i] != NULL)
                {
                    delete[] dsOffsets[i];
                }
            }
            delete[] dsOffsets;
        }
        
        if(bandIntOffsets != NULL)
        {
            for(int i = 0; i < numIntBands; i++)
            {
                if(bandIntOffsets[i] != NULL)
                {
                    delete[] bandIntOffsets[i];
                }
            }
            delete[] bandIntOffsets;
        }
        
        if(bandFloatOffsets != NULL)
        {
            for(int i = 0; i < numFloatBands; i++)
            {
                if(bandFloatOffsets[i] != NULL)
                {
                    delete[] bandFloatOffsets[i];
                }
            }
            delete[] bandFloatOffsets;
        }
        
        if(inputIntData != NULL)
        {
            for(int i = 0; i < numIntBands; i++)
            {
                if(inputIntData[i] != NULL)
                {
                    CPLFree(inputIntData[i]);
                }
            }
            delete[] inputIntData;
        }
        
        if(inputFloatData != NULL)
        {
            for(int i = 0; i < numFloatBands; i++)
            {
                if(inputFloatData[i] != NULL)
                {
                    delete[] inputFloatData[i];
                }
            }
            delete[] inputFloatData;
        }
        
        if(inDataIntColumn != NULL)
        {
            delete[] inDataIntColumn;
        }
        
        if(inDataFloatColumn != NULL)
        {
            delete[] inDataFloatColumn;
        }
        
        if(inputRasterIntBands != NULL)
        {
            delete[] inputRasterIntBands;
        }
        
        if(inputRasterFloatBands != NULL)
        {
            delete[] inputRasterFloatBands;
        }
        
        if(outputData != NULL)
        {
            for(int i = 0; i < this->numOutBands; i++)
            {
                if(outputData[i] != NULL)
                {
                    delete[] outputData[i];
                }
            }
            delete[] outputData;
        }
        
        if(outDataColumn != NULL)
        {
            delete[] outDataColumn;
        }
        
        if(outputRasterBands != NULL)
        {
            delete[] outputRasterBands;
        }
    }
    
	void RSGISCalcImage::calcImage(GDALDataset **datasets, int numDS)
	{
		GDALAllRegister();
		RSGISImageUtils imgUtils;
		double *gdalTranslation = new double[6];
		int **dsOffsets = new int*[numDS];
		for(int i = 0; i < numDS; i++)
		{
			dsOffsets[i] = new int[2];
		}
		int **bandOffsets = NULL;
		int height = 0;
		int width = 0;
		int numInBands = 0;
		
		float **inputData = NULL;
		float *inDataColumn = NULL;
        int xBlockSize = 0;
        int yBlockSize = 0;
		
		GDALRasterBand **inputRasterBands = NULL;
		
		try
		{
			// Find image overlap
			imgUtils.getImageOverlap(datasets, numDS, dsOffsets, &width, &height, gdalTranslation, &xBlockSize, &yBlockSize);
            
			// Count number of image bands
			for(int i = 0; i < numDS; i++)
			{
				numInBands += datasets[i]->GetRasterCount();
			}
			
			// Get Image Input Bands
			bandOffsets = new int*[numInBands];
			inputRasterBands = new GDALRasterBand*[numInBands];
			int counter = 0;
			for(int i = 0; i < numDS; i++)
			{
				for(int j = 0; j < datasets[i]->GetRasterCount(); j++)
				{
					inputRasterBands[counter] = datasets[i]->GetRasterBand(j+1);
					bandOffsets[counter] = new int[2];
					bandOffsets[counter][0] = dsOffsets[i][0];
					bandOffsets[counter][1] = dsOffsets[i][1];
					counter++;
				}
			}
			
			// Allocate memory
			inputData = new float*[numInBands];
			for(int i = 0; i < numInBands; i++)
			{
				inputData[i] = (float *) CPLMalloc(sizeof(float)*width*yBlockSize);
			}
			inDataColumn = new float[numInBands];
            
            int nYBlocks = height / yBlockSize;
            int remainRows = height - (nYBlocks * yBlockSize);
            int rowOffset = 0;
            
			int feedback = height/10;
			int feedbackCounter = 0;
			std::cout << "Started " << std::flush;
			// Loop images to process data
			for(int i = 0; i < nYBlocks; i++)
			{
				for(int n = 0; n < numInBands; n++)
				{
                    rowOffset = bandOffsets[n][1] + (yBlockSize * i);
					inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, yBlockSize, inputData[n], width, yBlockSize, GDT_Float32, 0, 0);
				}
                
                for(int m = 0; m < yBlockSize; ++m)
                {
                    if((feedback != 0) && ((((i*yBlockSize)+m) % feedback) == 0))
                    {
                        std::cout << "." << feedbackCounter << "." << std::flush;
                        feedbackCounter = feedbackCounter + 10;
                    }
                    
                    for(int j = 0; j < width; j++)
                    {
                        for(int n = 0; n < numInBands; n++)
                        {
                            inDataColumn[n] = inputData[n][(m*width)+j];
                        }
                        
                        this->calc->calcImageValue(inDataColumn, numInBands);
                    }
                }
			}
            
            if(remainRows > 0)
            {
                for(int n = 0; n < numInBands; n++)
				{
                    rowOffset = bandOffsets[n][1] + (yBlockSize * nYBlocks);
					inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, remainRows, inputData[n], width, remainRows, GDT_Float32, 0, 0);
				}
                
                for(int m = 0; m < remainRows; ++m)
                {
                    if((feedback != 0) && ((((nYBlocks*yBlockSize)+m) % feedback) == 0))
                    {
                        std::cout << "." << feedbackCounter << "." << std::flush;
                        feedbackCounter = feedbackCounter + 10;
                    }
                    
                    for(int j = 0; j < width; j++)
                    {
                        for(int n = 0; n < numInBands; n++)
                        {
                            inDataColumn[n] = inputData[n][(m*width)+j];
                        }
                        
                        this->calc->calcImageValue(inDataColumn, numInBands);
                    }
                }
            }
			std::cout << " Complete.\n";
		}
		catch(RSGISImageCalcException& e)
		{
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					if(dsOffsets[i] != NULL)
					{
						delete[] dsOffsets[i];
					}
				} 
				delete[] dsOffsets;
			}
			
			if(bandOffsets != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(bandOffsets[i] != NULL)
					{
						delete[] bandOffsets[i];
					}
				}
				delete[] bandOffsets;
			}			
			
			if(inputData != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(inputData[i] != NULL)
					{
						delete[] inputData[i];
					}
				}
				delete[] inputData;
			}		
			if(inDataColumn != NULL)
			{
				delete[] inDataColumn;
			}
			if(inputRasterBands != NULL)
			{
				delete[] inputRasterBands;
			}
			throw e;
		}
		catch(RSGISImageBandException& e)
		{
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					if(dsOffsets[i] != NULL)
					{
						delete[] dsOffsets[i];
					}
				} 
				delete[] dsOffsets;
			}
			
			if(bandOffsets != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(bandOffsets[i] != NULL)
					{
						delete[] bandOffsets[i];
					}
				}
				delete[] bandOffsets;
			}			
			
			if(inputData != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(inputData[i] != NULL)
					{
						delete[] inputData[i];
					}
				}
				delete[] inputData;
			}		
			if(inDataColumn != NULL)
			{
				delete[] inDataColumn;
			}
			if(inputRasterBands != NULL)
			{
				delete[] inputRasterBands;
			}
			throw e;
		}
				
		if(gdalTranslation != NULL)
		{
			delete[] gdalTranslation;
		}
		
		if(dsOffsets != NULL)
		{
			for(int i = 0; i < numDS; i++)
			{
				if(dsOffsets[i] != NULL)
				{
					delete[] dsOffsets[i];
				}
			} 
			delete[] dsOffsets;
		}
		
		if(bandOffsets != NULL)
		{
			for(int i = 0; i < numInBands; i++)
			{
				if(bandOffsets[i] != NULL)
				{
					delete[] bandOffsets[i];
				}
			}
			delete[] bandOffsets;
		}
		
		if(inputData != NULL)
		{
			for(int i = 0; i < numInBands; i++)
			{
				if(inputData[i] != NULL)
				{
					delete[] inputData[i];
				}
			}
			delete[] inputData;
		}		
		if(inDataColumn != NULL)
		{
			delete[] inDataColumn;
		}
		if(inputRasterBands != NULL)
		{
			delete[] inputRasterBands;
		}
	}
    
    void RSGISCalcImage::calcImageBand(GDALDataset **datasets, int numDS, std::string outputImageBase, std::string gdalFormat)
	{
		GDALAllRegister();
		RSGISImageUtils imgUtils;
        rsgis::math::RSGISMathsUtils mathUtils;
		double *gdalTranslation = new double[6];
		int **dsOffsets = new int*[numDS];
		for(int i = 0; i < numDS; i++)
		{
			dsOffsets[i] = new int[2];
		}
		int **bandOffsets = NULL;
		int height = 0;
		int width = 0;
		int numInBands = 0;
		
		float *inputData = NULL;
		double **outputData = NULL;
		float *inDataColumn = NULL;
		double *outDataColumn = NULL;
		
		GDALDataset *outputImageDS = NULL;
		GDALRasterBand **inputRasterBands = NULL;
		GDALRasterBand **outputRasterBands = NULL;
		GDALDriver *gdalDriver = NULL;
        
        std::string outputImageFileName = "";
		
		try
		{
			// Find image overlap
			imgUtils.getImageOverlap(datasets, numDS, dsOffsets, &width, &height, gdalTranslation);
            
			// Count number of image bands
			for(int i = 0; i < numDS; i++)
			{
				numInBands += datasets[i]->GetRasterCount();
			}
            
			// Create new Image
			gdalDriver = GetGDALDriverManager()->GetDriverByName(gdalFormat.c_str());
			if(gdalDriver == NULL)
			{
				throw RSGISImageBandException("ENVI driver does not exists..");
			}
            
            for(int cImgBand = 0; cImgBand < numInBands; ++cImgBand)
            {
                outputImageFileName = outputImageBase + mathUtils.inttostring(cImgBand) + std::string(".env");
                
                std::cout << "New image width = " << width << " height = " << height << " bands = " << this->numOutBands << std::endl;
				
				outputImageDS = gdalDriver->Create(outputImageFileName.c_str(), width, height, this->numOutBands, GDT_Float32, NULL);
				
                if(outputImageDS == NULL)
                {
                    throw RSGISImageBandException("Output image could not be created. Check filepath.");
                }
                outputImageDS->SetGeoTransform(gdalTranslation);
                if(useImageProj)
                {
                    outputImageDS->SetProjection(datasets[0]->GetProjectionRef());
                }
                else
                {
                    outputImageDS->SetProjection(proj.c_str());
                }
            
                // Get Image Input Bands
                bandOffsets = new int*[numInBands];
                inputRasterBands = new GDALRasterBand*[numInBands];
                int counter = 0;
                for(int i = 0; i < numDS; i++)
                {
                    for(int j = 0; j < datasets[i]->GetRasterCount(); j++)
                    {
                        inputRasterBands[counter] = datasets[i]->GetRasterBand(j+1);
                        bandOffsets[counter] = new int[2];
                        bandOffsets[counter][0] = dsOffsets[i][0];
                        bandOffsets[counter][1] = dsOffsets[i][1];
                        counter++;
                    }
                }
                
                //Get Image Output Bands
                outputRasterBands = new GDALRasterBand*[this->numOutBands];
                for(int i = 0; i < this->numOutBands; i++)
                {
                    outputRasterBands[i] = outputImageDS->GetRasterBand(i+1);
                }
            
                // Allocate memory
                inputData = (float *) CPLMalloc(sizeof(float)*width);
                inDataColumn = new float[1];
                
                outputData = new double*[this->numOutBands];
                for(int i = 0; i < this->numOutBands; i++)
                {
                    outputData[i] = (double *) CPLMalloc(sizeof(double)*width);
                }
                outDataColumn = new double[this->numOutBands];
            
                int feedback = height/10;
                int feedbackCounter = 0;
                std::cout << "Started (Band " << cImgBand+1 << "):\t" << std::flush;
                // Loop images to process data
                for(int i = 0; i < height; i++)
                {
                    if((feedback != 0) && ((i % feedback) == 0))
                    {
                        std::cout << "." << feedbackCounter << "." << std::flush;
                        feedbackCounter = feedbackCounter + 10;
                    }

                    inputRasterBands[cImgBand]->RasterIO(GF_Read, bandOffsets[cImgBand][0], (bandOffsets[cImgBand][1]+i), width, 1, inputData, width, 1, GDT_Float32, 0, 0);
                    
                    for(int j = 0; j < width; j++)
                    {
                        inDataColumn[0] = inputData[j];
                        
                        this->calc->calcImageValue(inDataColumn, numInBands, outDataColumn);
                        
                        for(int n = 0; n < this->numOutBands; n++)
                        {
                            outputData[n][j] = outDataColumn[n];
                        }
                        
                    }
                    
                    for(int n = 0; n < this->numOutBands; n++)
                    {
                        outputRasterBands[n]->RasterIO(GF_Write, 0, i, width, 1, outputData[n], width, 1, GDT_Float64, 0, 0);
                    }
                }
                std::cout << " Complete.\n";
                
                GDALClose(outputImageDS);
                
                delete[] inputData;
                
                if(outputData != NULL)
                {
                    for(int i = 0; i < this->numOutBands; i++)
                    {
                        if(outputData[i] != NULL)
                        {
                            delete[] outputData[i];
                        }
                    }
                    delete[] outputData;
                }
                
                if(inDataColumn != NULL)
                {
                    delete[] inDataColumn;
                }
                
                if(outDataColumn != NULL)
                {
                    delete[] outDataColumn;
                }

                if(inputRasterBands != NULL)
                {
                    delete[] inputRasterBands;
                }
                
                if(outputRasterBands != NULL)
                {
                    delete[] outputRasterBands;
                }
                
                if(bandOffsets != NULL)
                {
                    for(int i = 0; i < numInBands; i++)
                    {
                        if(bandOffsets[i] != NULL)
                        {
                            delete[] bandOffsets[i];
                        }
                    }
                    delete[] bandOffsets;
                }
            }
		}
		catch(RSGISImageCalcException& e)
		{
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					if(dsOffsets[i] != NULL)
					{
						delete[] dsOffsets[i];
					}
				} 
				delete[] dsOffsets;
			}
            
			throw e;
		}
		catch(RSGISImageBandException& e)
		{
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					if(dsOffsets[i] != NULL)
					{
						delete[] dsOffsets[i];
					}
				} 
				delete[] dsOffsets;
			}
			
			throw e;
		}
		
		if(gdalTranslation != NULL)
		{
			delete[] gdalTranslation;
		}
		
		if(dsOffsets != NULL)
		{
			for(int i = 0; i < numDS; i++)
			{
				if(dsOffsets[i] != NULL)
				{
					delete[] dsOffsets[i];
				}
			} 
			delete[] dsOffsets;
		}

	}

    
    void RSGISCalcImage::calcImageInEnv(GDALDataset **datasets, int numDS, std::string outputImage, geos::geom::Envelope *env, bool setOutNames, std::string *bandNames, std::string gdalFormat, GDALDataType gdalDataType)
    {
        GDALAllRegister();
		RSGISImageUtils imgUtils;
		double *gdalTranslation = new double[6];
		int **dsOffsets = new int*[numDS];
		for(int i = 0; i < numDS; i++)
		{
			dsOffsets[i] = new int[2];
		}
		int **bandOffsets = NULL;
		int height = 0;
		int width = 0;
		int numInBands = 0;
        int xBlockSize = 0;
        int yBlockSize = 0;
		
		float **inputData = NULL;
		double **outputData = NULL;
		float *inDataColumn = NULL;
		double *outDataColumn = NULL;
		
		GDALDataset *outputImageDS = NULL;
		GDALRasterBand **inputRasterBands = NULL;
		GDALRasterBand **outputRasterBands = NULL;
		GDALDriver *gdalDriver = NULL;
        
        try
		{
			// Find image overlap
            imgUtils.getImageOverlapCut2Env(datasets, numDS, dsOffsets, &width, &height, gdalTranslation, env, &xBlockSize, &yBlockSize);

            // Count number of image bands
			for(int i = 0; i < numDS; i++)
			{
				numInBands += datasets[i]->GetRasterCount();
			}
            
			// Create new Image
			gdalDriver = GetGDALDriverManager()->GetDriverByName(gdalFormat.c_str());
			if(gdalDriver == NULL)
			{
				throw RSGISImageBandException("Requested GDAL driver does not exists..");
			}
			std::cout << "New image width = " << width << " height = " << height << " bands = " << this->numOutBands << std::endl;
			
			outputImageDS = gdalDriver->Create(outputImage.c_str(), width, height, this->numOutBands, gdalDataType, NULL);
			
			if(outputImageDS == NULL)
			{
				throw RSGISImageBandException("Output image could not be created. Check filepath.");
			}
			outputImageDS->SetGeoTransform(gdalTranslation);
			if(useImageProj)
			{
				outputImageDS->SetProjection(datasets[0]->GetProjectionRef());
			}
			else
			{
				outputImageDS->SetProjection(proj.c_str());
			}
            
			// Get Image Input Bands
			bandOffsets = new int*[numInBands];
			inputRasterBands = new GDALRasterBand*[numInBands];
			int counter = 0;
			for(int i = 0; i < numDS; i++)
			{
				for(int j = 0; j < datasets[i]->GetRasterCount(); j++)
				{
					inputRasterBands[counter] = datasets[i]->GetRasterBand(j+1);
					bandOffsets[counter] = new int[2];
					bandOffsets[counter][0] = dsOffsets[i][0];
					bandOffsets[counter][1] = dsOffsets[i][1];
					counter++;
				}
			}
            
			//Get Image Output Bands
			outputRasterBands = new GDALRasterBand*[this->numOutBands];
			for(int i = 0; i < this->numOutBands; i++)
			{
				outputRasterBands[i] = outputImageDS->GetRasterBand(i+1);
				if (setOutNames) // Set output band names
				{
					outputRasterBands[i]->SetDescription(bandNames[i].c_str());
				}
			}
            int outXBlockSize = 0;
            int outYBlockSize = 0;
            outputRasterBands[0]->GetBlockSize (&outXBlockSize, &outYBlockSize);
            
            if(outYBlockSize > yBlockSize)
            {
                yBlockSize = outYBlockSize;
            }
            
			// Allocate memory
			inputData = new float*[numInBands];
			for(int i = 0; i < numInBands; i++)
			{
				inputData[i] = (float *) CPLMalloc(sizeof(float)*(width*yBlockSize));
			}
			inDataColumn = new float[numInBands];
            
			outputData = new double*[this->numOutBands];
			for(int i = 0; i < this->numOutBands; i++)
			{
				outputData[i] = (double *) CPLMalloc(sizeof(double)*(width*yBlockSize));
			}
			outDataColumn = new double[this->numOutBands];
            
            int nYBlocks = floor(((double)height) / ((double)yBlockSize));
            int remainRows = height - (nYBlocks * yBlockSize);
            int rowOffset = 0;
            
			int feedback = height/10.0;
			int feedbackCounter = 0;
			std::cout << "Started " << std::flush;
			// Loop images to process data
			for(int i = 0; i < nYBlocks; i++)
			{
				for(int n = 0; n < numInBands; n++)
				{
                    rowOffset = bandOffsets[n][1] + (yBlockSize * i);
					inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, yBlockSize, inputData[n], width, yBlockSize, GDT_Float32, 0, 0);
				}
                
                for(int m = 0; m < yBlockSize; ++m)
                {
                    if((feedback != 0) && (((i*yBlockSize)+m) % feedback) == 0)
                    {
                        std::cout << "." << feedbackCounter << "." << std::flush;
                        feedbackCounter = feedbackCounter + 10;
                    }
                    
                    
                    for(int j = 0; j < width; j++)
                    {
                        for(int n = 0; n < numInBands; n++)
                        {
                            inDataColumn[n] = inputData[n][(m*width)+j];
                        }
                        
                        this->calc->calcImageValue(inDataColumn, numInBands, outDataColumn);
                        
                        for(int n = 0; n < this->numOutBands; n++)
                        {
                            outputData[n][(m*width)+j] = outDataColumn[n];
                        }
                        
                    }
                }
				
				for(int n = 0; n < this->numOutBands; n++)
				{
                    rowOffset = yBlockSize * i;
					outputRasterBands[n]->RasterIO(GF_Write, 0, rowOffset, width, yBlockSize, outputData[n], width, yBlockSize, GDT_Float64, 0, 0);
				}
			}
            
            if(remainRows > 0)
            {
                for(int n = 0; n < numInBands; n++)
				{
                    rowOffset = bandOffsets[n][1] + (yBlockSize * nYBlocks);
					inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, remainRows, inputData[n], width, remainRows, GDT_Float32, 0, 0);
				}
                
                for(int m = 0; m < remainRows; ++m)
                {
                    if((feedback != 0) && ((((nYBlocks*yBlockSize)+m) % feedback) == 0))
                    {
                        std::cout << "." << feedbackCounter << "." << std::flush;
                        feedbackCounter = feedbackCounter + 10;
                    }
                    
                    for(int j = 0; j < width; j++)
                    {
                        for(int n = 0; n < numInBands; n++)
                        {
                            inDataColumn[n] = inputData[n][(m*width)+j];
                        }
                        
                        this->calc->calcImageValue(inDataColumn, numInBands, outDataColumn);
                        
                        for(int n = 0; n < this->numOutBands; n++)
                        {
                            outputData[n][(m*width)+j] = outDataColumn[n];
                        }
                        
                    }
                }
				
				for(int n = 0; n < this->numOutBands; n++)
				{
                    rowOffset = (yBlockSize * nYBlocks);
					outputRasterBands[n]->RasterIO(GF_Write, 0, rowOffset, width, remainRows, outputData[n], width, remainRows, GDT_Float64, 0, 0);
				}
            }
			std::cout << " Complete.\n";
		}
		catch(RSGISImageCalcException& e)
		{
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					if(dsOffsets[i] != NULL)
					{
						delete[] dsOffsets[i];
					}
				}
				delete[] dsOffsets;
			}
			
			if(bandOffsets != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(bandOffsets[i] != NULL)
					{
						delete[] bandOffsets[i];
					}
				}
				delete[] bandOffsets;
			}
			
			if(inputData != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(inputData[i] != NULL)
					{
						delete[] inputData[i];
					}
				}
				delete[] inputData;
			}
			
			if(outputData != NULL)
			{
				for(int i = 0; i < this->numOutBands; i++)
				{
					if(outputData[i] != NULL)
					{
						delete[] outputData[i];
					}
				}
				delete[] outputData;
			}
			
			if(inDataColumn != NULL)
			{
				delete[] inDataColumn;
			}
			
			if(outDataColumn != NULL)
			{
				delete[] outDataColumn;
			}
			
			if(inputRasterBands != NULL)
			{
				delete[] inputRasterBands;
			}
			
			if(outputRasterBands != NULL)
			{
				delete[] outputRasterBands;
			}
			throw e;
		}
		catch(RSGISImageBandException& e)
		{
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					if(dsOffsets[i] != NULL)
					{
						delete[] dsOffsets[i];
					}
				}
				delete[] dsOffsets;
			}
			
			if(bandOffsets != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(bandOffsets[i] != NULL)
					{
						delete[] bandOffsets[i];
					}
				}
				delete[] bandOffsets;
			}
			
			if(inputData != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(inputData[i] != NULL)
					{
						delete[] inputData[i];
					}
				}
				delete[] inputData;
			}
			
			if(outputData != NULL)
			{
				for(int i = 0; i < this->numOutBands; i++)
				{
					if(outputData[i] != NULL)
					{
						delete[] outputData[i];
					}
				}
				delete[] outputData;
			}
			
			if(inDataColumn != NULL)
			{
				delete[] inDataColumn;
			}
			
			if(outDataColumn != NULL)
			{
				delete[] outDataColumn;
			}
			
			if(inputRasterBands != NULL)
			{
				delete[] inputRasterBands;
			}
			
			if(outputRasterBands != NULL)
			{
				delete[] outputRasterBands;
			}
			throw e;
		}
		
		GDALClose(outputImageDS);
		
		if(gdalTranslation != NULL)
		{
			delete[] gdalTranslation;
		}
		
		if(dsOffsets != NULL)
		{
			for(int i = 0; i < numDS; i++)
			{
				if(dsOffsets[i] != NULL)
				{
					delete[] dsOffsets[i];
				}
			}
			delete[] dsOffsets;
		}
		
		if(bandOffsets != NULL)
		{
			for(int i = 0; i < numInBands; i++)
			{
				if(bandOffsets[i] != NULL)
				{
					delete[] bandOffsets[i];
				}
			}
			delete[] bandOffsets;
		}
		
		if(inputData != NULL)
		{
			for(int i = 0; i < numInBands; i++)
			{
				if(inputData[i] != NULL)
				{
					CPLFree(inputData[i]);
				}
			}
			delete[] inputData;
		}
		
		if(outputData != NULL)
		{
			for(int i = 0; i < this->numOutBands; i++)
			{
				if(outputData[i] != NULL)
				{
					CPLFree(outputData[i]);
				}
			}
			delete[] outputData;
		}
		
		if(inDataColumn != NULL)
		{
			delete[] inDataColumn;
		}
		
		if(outDataColumn != NULL)
		{
			delete[] outDataColumn;
		}
		
		if(inputRasterBands != NULL)
		{
			delete[] inputRasterBands;
		}
		
		if(outputRasterBands != NULL)
		{
			delete[] outputRasterBands;
		}
    }
    
    void RSGISCalcImage::calcImageInEnv(GDALDataset **datasets, int numDS, geos::geom::Envelope *env, bool quiet)
    {
        GDALAllRegister();
		RSGISImageUtils imgUtils;
		double *gdalTranslation = new double[6];
		int **dsOffsets = new int*[numDS];
		for(int i = 0; i < numDS; i++)
		{
			dsOffsets[i] = new int[2];
		}
		int **bandOffsets = NULL;
		int height = 0;
		int width = 0;
		int numInBands = 0;
        int xBlockSize = 0;
        int yBlockSize = 0;
		
		float **inputData = NULL;
		float *inDataColumn = NULL;
		
		GDALRasterBand **inputRasterBands = NULL;

        
        try
		{
			// Find image overlap
            imgUtils.getImageOverlapCut2Env(datasets, numDS, dsOffsets, &width, &height, gdalTranslation, env, &xBlockSize, &yBlockSize);
            
			// Count number of image bands
			for(int i = 0; i < numDS; i++)
			{
				numInBands += datasets[i]->GetRasterCount();
			}
            
			// Get Image Input Bands
			bandOffsets = new int*[numInBands];
			inputRasterBands = new GDALRasterBand*[numInBands];
			int counter = 0;
			for(int i = 0; i < numDS; i++)
			{
				for(int j = 0; j < datasets[i]->GetRasterCount(); j++)
				{
					inputRasterBands[counter] = datasets[i]->GetRasterBand(j+1);
					bandOffsets[counter] = new int[2];
					bandOffsets[counter][0] = dsOffsets[i][0];
					bandOffsets[counter][1] = dsOffsets[i][1];
					counter++;
				}
			}
            
			// Allocate memory
			inputData = new float*[numInBands];
			for(int i = 0; i < numInBands; i++)
			{
				inputData[i] = (float *) CPLMalloc(sizeof(float)*(width*yBlockSize));
			}
			inDataColumn = new float[numInBands];
            
            int nYBlocks = floor(((double)height) / ((double)yBlockSize));
            int remainRows = height - (nYBlocks * yBlockSize);
            int rowOffset = 0;
            
			int feedback = height/10.0;
			int feedbackCounter = 0;
            if(!quiet)
            {
                std::cout << "Started " << std::flush;
            }
			// Loop images to process data
			for(int i = 0; i < nYBlocks; i++)
			{
				for(int n = 0; n < numInBands; n++)
				{
                    rowOffset = bandOffsets[n][1] + (yBlockSize * i);
					inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, yBlockSize, inputData[n], width, yBlockSize, GDT_Float32, 0, 0);
				}
                
                for(int m = 0; m < yBlockSize; ++m)
                {
                    if((!quiet) && (feedback != 0) && (((i*yBlockSize)+m) % feedback) == 0)
                    {
                        std::cout << "." << feedbackCounter << "." << std::flush;
                        feedbackCounter = feedbackCounter + 10;
                    }
                    
                    
                    for(int j = 0; j < width; j++)
                    {
                        for(int n = 0; n < numInBands; n++)
                        {
                            inDataColumn[n] = inputData[n][(m*width)+j];
                        }
                        
                        this->calc->calcImageValue(inDataColumn, numInBands);
                        
                    }
                }
			}
            
            if(remainRows > 0)
            {
                for(int n = 0; n < numInBands; n++)
				{
                    rowOffset = bandOffsets[n][1] + (yBlockSize * nYBlocks);
					inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, remainRows, inputData[n], width, remainRows, GDT_Float32, 0, 0);
				}
                
                for(int m = 0; m < remainRows; ++m)
                {
                    if((!quiet) && (feedback != 0) && ((((nYBlocks*yBlockSize)+m) % feedback) == 0))
                    {
                        std::cout << "." << feedbackCounter << "." << std::flush;
                        feedbackCounter = feedbackCounter + 10;
                    }
                    
                    for(int j = 0; j < width; j++)
                    {
                        for(int n = 0; n < numInBands; n++)
                        {
                            inDataColumn[n] = inputData[n][(m*width)+j];
                        }
                        
                        this->calc->calcImageValue(inDataColumn, numInBands);
                    }
                }
            }
            if(!quiet)
            {
                std::cout << " Complete.\n";
            }
		}
		catch(RSGISImageCalcException& e)
		{
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					if(dsOffsets[i] != NULL)
					{
						delete[] dsOffsets[i];
					}
				}
				delete[] dsOffsets;
			}
			
			if(bandOffsets != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(bandOffsets[i] != NULL)
					{
						delete[] bandOffsets[i];
					}
				}
				delete[] bandOffsets;
			}
			
			if(inputData != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(inputData[i] != NULL)
					{
						delete[] inputData[i];
					}
				}
				delete[] inputData;
			}
			
			if(inDataColumn != NULL)
			{
				delete[] inDataColumn;
			}
			
			if(inputRasterBands != NULL)
			{
				delete[] inputRasterBands;
			}

			throw e;
		}
		catch(RSGISImageBandException& e)
		{
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					if(dsOffsets[i] != NULL)
					{
						delete[] dsOffsets[i];
					}
				}
				delete[] dsOffsets;
			}
			
			if(bandOffsets != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(bandOffsets[i] != NULL)
					{
						delete[] bandOffsets[i];
					}
				}
				delete[] bandOffsets;
			}
			
			if(inputData != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(inputData[i] != NULL)
					{
						delete[] inputData[i];
					}
				}
				delete[] inputData;
			}
			
			if(inDataColumn != NULL)
			{
				delete[] inDataColumn;
			}
			
			if(inputRasterBands != NULL)
			{
				delete[] inputRasterBands;
			}
			throw e;
		}
				
		if(gdalTranslation != NULL)
		{
			delete[] gdalTranslation;
		}
		
		if(dsOffsets != NULL)
		{
			for(int i = 0; i < numDS; i++)
			{
				if(dsOffsets[i] != NULL)
				{
					delete[] dsOffsets[i];
				}
			}
			delete[] dsOffsets;
		}
		
		if(bandOffsets != NULL)
		{
			for(int i = 0; i < numInBands; i++)
			{
				if(bandOffsets[i] != NULL)
				{
					delete[] bandOffsets[i];
				}
			}
			delete[] bandOffsets;
		}
		
		if(inputData != NULL)
		{
			for(int i = 0; i < numInBands; i++)
			{
				if(inputData[i] != NULL)
				{
					CPLFree(inputData[i]);
				}
			}
			delete[] inputData;
		}
		
		if(inDataColumn != NULL)
		{
			delete[] inDataColumn;
		}
		
		if(inputRasterBands != NULL)
		{
			delete[] inputRasterBands;
		}
    }
    
    void RSGISCalcImage::calcImageInEnv(GDALDataset **datasets, int numIntDS, int numFloatDS, geos::geom::Envelope *env, bool quiet)
    {
        GDALAllRegister();
        RSGISImageUtils imgUtils;
        int numDS = numIntDS + numFloatDS;
        double *gdalTranslation = new double[6];
        int **dsOffsets = new int*[numDS];
        for(int i = 0; i < numDS; i++)
        {
            dsOffsets[i] = new int[2];
        }
        int **bandIntOffsets = NULL;
        int **bandFloatOffsets = NULL;
        int height = 0;
        int width = 0;
        int numIntBands = 0;
        int numFloatBands = 0;
        
        float **inputFloatData = NULL;
        float *inDataFloatColumn = NULL;
        unsigned int **inputIntData = NULL;
        long *inDataIntColumn = NULL;
        int xBlockSize = 0;
        int yBlockSize = 0;
        
        GDALRasterBand **inputRasterFloatBands = NULL;
        GDALRasterBand **inputRasterIntBands = NULL;
        
        try
        {
            // Find image overlap
            imgUtils.getImageOverlapCut2Env(datasets, numDS, dsOffsets, &width, &height, gdalTranslation, env, &xBlockSize, &yBlockSize);
            
            // Count number of image bands
            for(int i = 0; i < numIntDS; i++)
            {
                numIntBands += datasets[i]->GetRasterCount();
            }
            for(int i = numIntDS; i < numDS; i++)
            {
                numFloatBands += datasets[i]->GetRasterCount();
            }
            
            // Get Image Input Bands
            bandIntOffsets = new int*[numIntBands];
            inputRasterIntBands = new GDALRasterBand*[numIntBands];
            
            bandFloatOffsets = new int*[numFloatBands];
            inputRasterFloatBands = new GDALRasterBand*[numFloatBands];
            
            int counter = 0;
            for(int i = 0; i < numIntDS; i++)
            {
                for(int j = 0; j < datasets[i]->GetRasterCount(); j++)
                {
                    inputRasterIntBands[counter] = datasets[i]->GetRasterBand(j+1);
                    bandIntOffsets[counter] = new int[2];
                    bandIntOffsets[counter][0] = dsOffsets[i][0];
                    bandIntOffsets[counter][1] = dsOffsets[i][1];
                    counter++;
                }
            }
            
            counter = 0;
            for(int i = numIntDS; i < numDS; i++)
            {
                for(int j = 0; j < datasets[i]->GetRasterCount(); j++)
                {
                    inputRasterFloatBands[counter] = datasets[i]->GetRasterBand(j+1);
                    bandFloatOffsets[counter] = new int[2];
                    bandFloatOffsets[counter][0] = dsOffsets[i][0];
                    bandFloatOffsets[counter][1] = dsOffsets[i][1];
                    counter++;
                }
            }
            
            // Allocate memory
            inputIntData = new unsigned int*[numIntBands];
            for(int i = 0; i < numIntBands; i++)
            {
                inputIntData[i] = (unsigned int *) CPLMalloc(sizeof(unsigned int)*width*yBlockSize);
            }
            inDataIntColumn = new long[numIntBands];
            
            inputFloatData = new float*[numFloatBands];
            for(int i = 0; i < numFloatBands; i++)
            {
                inputFloatData[i] = (float *) CPLMalloc(sizeof(float)*width*yBlockSize);
            }
            inDataFloatColumn = new float[numFloatBands];
            
            
            int nYBlocks = height / yBlockSize;
            int remainRows = height - (nYBlocks * yBlockSize);
            int rowOffset = 0;
            
            int feedback = height/10;
            int feedbackCounter = 0;
            if(!quiet)
            {
                std::cout << "Started " << std::flush;
            }
            // Loop images to process data
            for(int i = 0; i < nYBlocks; i++)
            {
                for(int n = 0; n < numIntBands; n++)
                {
                    rowOffset = bandIntOffsets[n][1] + (yBlockSize * i);
                    inputRasterIntBands[n]->RasterIO(GF_Read, bandIntOffsets[n][0], rowOffset, width, yBlockSize, inputIntData[n], width, yBlockSize, GDT_UInt32, 0, 0);
                }
                
                for(int n = 0; n < numFloatBands; n++)
                {
                    rowOffset = bandFloatOffsets[n][1] + (yBlockSize * i);
                    inputRasterFloatBands[n]->RasterIO(GF_Read, bandFloatOffsets[n][0], rowOffset, width, yBlockSize, inputFloatData[n], width, yBlockSize, GDT_Float32, 0, 0);
                }
                
                for(int m = 0; m < yBlockSize; ++m)
                {
                    if((!quiet) && (feedback != 0) && ((((i*yBlockSize)+m) % feedback) == 0))
                    {
                        std::cout << "." << feedbackCounter << "." << std::flush;
                        feedbackCounter = feedbackCounter + 10;
                    }
                    
                    
                    for(int j = 0; j < width; j++)
                    {
                        for(int n = 0; n < numIntBands; n++)
                        {
                            inDataIntColumn[n] = inputIntData[n][(m*width)+j];
                        }
                        
                        for(int n = 0; n < numFloatBands; n++)
                        {
                            inDataFloatColumn[n] = inputFloatData[n][(m*width)+j];
                        }
                        
                        this->calc->calcImageValue(inDataIntColumn, numIntBands, inDataFloatColumn, numFloatBands);
                    }
                }
            }
            
            if(remainRows > 0)
            {
                for(int n = 0; n < numIntBands; n++)
                {
                    rowOffset = bandIntOffsets[n][1] + (yBlockSize * nYBlocks);
                    inputRasterIntBands[n]->RasterIO(GF_Read, bandIntOffsets[n][0], rowOffset, width, remainRows, inputIntData[n], width, remainRows, GDT_UInt32, 0, 0);
                }
                
                
                for(int n = 0; n < numFloatBands; n++)
                {
                    rowOffset = bandFloatOffsets[n][1] + (yBlockSize * nYBlocks);
                    inputRasterFloatBands[n]->RasterIO(GF_Read, bandFloatOffsets[n][0], rowOffset, width, remainRows, inputFloatData[n], width, remainRows, GDT_Float32, 0, 0);
                }
                
                for(int m = 0; m < remainRows; ++m)
                {
                    if((!quiet) && (feedback != 0) && ((((nYBlocks*yBlockSize)+m) % feedback) == 0))
                    {
                        std::cout << "." << feedbackCounter << "." << std::flush;
                        feedbackCounter = feedbackCounter + 10;
                    }
                    
                    for(int j = 0; j < width; j++)
                    {
                        for(int n = 0; n < numIntBands; n++)
                        {
                            inDataIntColumn[n] = inputIntData[n][(m*width)+j];
                        }
                        
                        for(int n = 0; n < numFloatBands; n++)
                        {
                            inDataFloatColumn[n] = inputFloatData[n][(m*width)+j];
                        }
                        
                        this->calc->calcImageValue(inDataIntColumn, numIntBands, inDataFloatColumn, numFloatBands);
                    }
                }
            }
            if(!quiet)
            {
                std::cout << " Complete.\n";
            }
        }
        catch(RSGISImageCalcException& e)
        {
            if(gdalTranslation != NULL)
            {
                delete[] gdalTranslation;
            }
            
            if(dsOffsets != NULL)
            {
                for(int i = 0; i < numDS; i++)
                {
                    if(dsOffsets[i] != NULL)
                    {
                        delete[] dsOffsets[i];
                    }
                }
                delete[] dsOffsets;
            }
            
            if(bandIntOffsets != NULL)
            {
                for(int i = 0; i < numIntBands; i++)
                {
                    if(bandIntOffsets[i] != NULL)
                    {
                        delete[] bandIntOffsets[i];
                    }
                }
                delete[] bandIntOffsets;
            }
            
            if(bandFloatOffsets != NULL)
            {
                for(int i = 0; i < numFloatBands; i++)
                {
                    if(bandFloatOffsets[i] != NULL)
                    {
                        delete[] bandFloatOffsets[i];
                    }
                }
                delete[] bandFloatOffsets;
            }
            
            if(inputIntData != NULL)
            {
                for(int i = 0; i < numIntBands; i++)
                {
                    if(inputIntData[i] != NULL)
                    {
                        CPLFree(inputIntData[i]);
                    }
                }
                delete[] inputIntData;
            }
            
            if(inputFloatData != NULL)
            {
                for(int i = 0; i < numFloatBands; i++)
                {
                    if(inputFloatData[i] != NULL)
                    {
                        delete[] inputFloatData[i];
                    }
                }
                delete[] inputFloatData;
            }
            
            if(inDataIntColumn != NULL)
            {
                delete[] inDataIntColumn;
            }
            
            if(inDataFloatColumn != NULL)
            {
                delete[] inDataFloatColumn;
            }
            
            if(inputRasterIntBands != NULL)
            {
                delete[] inputRasterIntBands;
            }
            
            if(inputRasterFloatBands != NULL)
            {
                delete[] inputRasterFloatBands;
            }
            throw e;
        }
        catch(RSGISImageBandException& e)
        {
            if(gdalTranslation != NULL)
            {
                delete[] gdalTranslation;
            }
            
            if(dsOffsets != NULL)
            {
                for(int i = 0; i < numDS; i++)
                {
                    if(dsOffsets[i] != NULL)
                    {
                        delete[] dsOffsets[i];
                    }
                }
                delete[] dsOffsets;
            }
            
            if(bandIntOffsets != NULL)
            {
                for(int i = 0; i < numIntBands; i++)
                {
                    if(bandIntOffsets[i] != NULL)
                    {
                        delete[] bandIntOffsets[i];
                    }
                }
                delete[] bandIntOffsets;
            }
            
            if(bandFloatOffsets != NULL)
            {
                for(int i = 0; i < numFloatBands; i++)
                {
                    if(bandFloatOffsets[i] != NULL)
                    {
                        delete[] bandFloatOffsets[i];
                    }
                }
                delete[] bandFloatOffsets;
            }
            
            if(inputIntData != NULL)
            {
                for(int i = 0; i < numIntBands; i++)
                {
                    if(inputIntData[i] != NULL)
                    {
                        CPLFree(inputIntData[i]);
                    }
                }
                delete[] inputIntData;
            }
            
            if(inputFloatData != NULL)
            {
                for(int i = 0; i < numFloatBands; i++)
                {
                    if(inputFloatData[i] != NULL)
                    {
                        delete[] inputFloatData[i];
                    }
                }
                delete[] inputFloatData;
            }
            
            if(inDataIntColumn != NULL)
            {
                delete[] inDataIntColumn;
            }
            
            if(inDataFloatColumn != NULL)
            {
                delete[] inDataFloatColumn;
            }
            
            if(inputRasterIntBands != NULL)
            {
                delete[] inputRasterIntBands;
            }
            
            if(inputRasterFloatBands != NULL)
            {
                delete[] inputRasterFloatBands;
            }
            throw e;
        }
        
        if(gdalTranslation != NULL)
        {
            delete[] gdalTranslation;
        }
        
        if(dsOffsets != NULL)
        {
            for(int i = 0; i < numDS; i++)
            {
                if(dsOffsets[i] != NULL)
                {
                    delete[] dsOffsets[i];
                }
            }
            delete[] dsOffsets;
        }
        
        if(bandIntOffsets != NULL)
        {
            for(int i = 0; i < numIntBands; i++)
            {
                if(bandIntOffsets[i] != NULL)
                {
                    delete[] bandIntOffsets[i];
                }
            }
            delete[] bandIntOffsets;
        }
        
        if(bandFloatOffsets != NULL)
        {
            for(int i = 0; i < numFloatBands; i++)
            {
                if(bandFloatOffsets[i] != NULL)
                {
                    delete[] bandFloatOffsets[i];
                }
            }
            delete[] bandFloatOffsets;
        }
        
        if(inputIntData != NULL)
        {
            for(int i = 0; i < numIntBands; i++)
            {
                if(inputIntData[i] != NULL)
                {
                    CPLFree(inputIntData[i]);
                }
            }
            delete[] inputIntData;
        }
        
        if(inputFloatData != NULL)
        {
            for(int i = 0; i < numFloatBands; i++)
            {
                if(inputFloatData[i] != NULL)
                {
                    delete[] inputFloatData[i];
                }
            }
            delete[] inputFloatData;
        }
        
        if(inDataIntColumn != NULL)
        {
            delete[] inDataIntColumn;
        }
        
        if(inDataFloatColumn != NULL)
        {
            delete[] inDataFloatColumn;
        }
        
        if(inputRasterIntBands != NULL)
        {
            delete[] inputRasterIntBands;
        }
        
        if(inputRasterFloatBands != NULL)
        {
            delete[] inputRasterFloatBands;
        }
    }
    
    void RSGISCalcImage::calcImagePosPxl(GDALDataset **datasets, int numDS)
	{
		GDALAllRegister();
		RSGISImageUtils imgUtils;
		double *gdalTranslation = new double[6];
		int **dsOffsets = new int*[numDS];
		for(int i = 0; i < numDS; i++)
		{
			dsOffsets[i] = new int[2];
		}
		int **bandOffsets = NULL;
		int height = 0;
		int width = 0;
		int numInBands = 0;
		
		float **inputData = NULL;
		float *inDataColumn = NULL;
        int xBlockSize = 0;
        int yBlockSize = 0;
		
		GDALRasterBand **inputRasterBands = NULL;
		
		try
		{
			// Find image overlap
			imgUtils.getImageOverlap(datasets, numDS, dsOffsets, &width, &height, gdalTranslation, &xBlockSize, &yBlockSize);
            
			// Count number of image bands
			for(int i = 0; i < numDS; i++)
			{
				numInBands += datasets[i]->GetRasterCount();
			}
			
			// Get Image Input Bands
			bandOffsets = new int*[numInBands];
			inputRasterBands = new GDALRasterBand*[numInBands];
			int counter = 0;
			for(int i = 0; i < numDS; i++)
			{
				for(int j = 0; j < datasets[i]->GetRasterCount(); j++)
				{
					inputRasterBands[counter] = datasets[i]->GetRasterBand(j+1);
					bandOffsets[counter] = new int[2];
					bandOffsets[counter][0] = dsOffsets[i][0];
					bandOffsets[counter][1] = dsOffsets[i][1];
					counter++;
				}
			}
			
			// Allocate memory
			inputData = new float*[numInBands];
			for(int i = 0; i < numInBands; i++)
			{
				inputData[i] = (float *) CPLMalloc(sizeof(float)*width*yBlockSize);
			}
			inDataColumn = new float[numInBands];
            
            int nYBlocks = height / yBlockSize;
            int remainRows = height - (nYBlocks * yBlockSize);
            int rowOffset = 0;
            
			int feedback = height/10;
			int feedbackCounter = 0;
            unsigned int xPxl = 0;
            unsigned int yPxl = 0;
            geos::geom::Envelope extent;
			std::cout << "Started " << std::flush;
			// Loop images to process data
			for(int i = 0; i < nYBlocks; i++)
			{
				for(int n = 0; n < numInBands; n++)
				{
                    rowOffset = bandOffsets[n][1] + (yBlockSize * i);
					inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, yBlockSize, inputData[n], width, yBlockSize, GDT_Float32, 0, 0);
				}
                
                for(int m = 0; m < yBlockSize; ++m)
                {
                    if((feedback != 0) && ((((i*yBlockSize)+m) % feedback) == 0))
                    {
                        std::cout << "." << feedbackCounter << "." << std::flush;
                        feedbackCounter = feedbackCounter + 10;
                    }
                    
                    xPxl = 0;
                    for(int j = 0; j < width; j++)
                    {
                        for(int n = 0; n < numInBands; n++)
                        {
                            inDataColumn[n] = inputData[n][(m*width)+j];
                        }
                        
                        extent.init(xPxl, xPxl, yPxl, yPxl);
                        
                        this->calc->calcImageValue(inDataColumn, numInBands, extent);
                        ++xPxl;
                    }
                    ++yPxl;
                }
			}
            
            if(remainRows > 0)
            {
                for(int n = 0; n < numInBands; n++)
				{
                    rowOffset = bandOffsets[n][1] + (yBlockSize * nYBlocks);
					inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, remainRows, inputData[n], width, remainRows, GDT_Float32, 0, 0);
				}
                
                for(int m = 0; m < remainRows; ++m)
                {
                    if((feedback != 0) && ((((nYBlocks*yBlockSize)+m) % feedback) == 0))
                    {
                        std::cout << "." << feedbackCounter << "." << std::flush;
                        feedbackCounter = feedbackCounter + 10;
                    }
                    
                    xPxl = 0;
                    for(int j = 0; j < width; j++)
                    {
                        for(int n = 0; n < numInBands; n++)
                        {
                            inDataColumn[n] = inputData[n][(m*width)+j];
                        }
                        
                        extent.init(xPxl, xPxl, yPxl, yPxl);
                        
                        this->calc->calcImageValue(inDataColumn, numInBands, extent);
                        ++xPxl;
                    }
                    ++yPxl;
                }
            }
			std::cout << " Complete.\n";
		}
		catch(RSGISImageCalcException& e)
		{
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					if(dsOffsets[i] != NULL)
					{
						delete[] dsOffsets[i];
					}
				}
				delete[] dsOffsets;
			}
			
			if(bandOffsets != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(bandOffsets[i] != NULL)
					{
						delete[] bandOffsets[i];
					}
				}
				delete[] bandOffsets;
			}
			
			if(inputData != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(inputData[i] != NULL)
					{
						delete[] inputData[i];
					}
				}
				delete[] inputData;
			}
			if(inDataColumn != NULL)
			{
				delete[] inDataColumn;
			}
			if(inputRasterBands != NULL)
			{
				delete[] inputRasterBands;
			}
			throw e;
		}
		catch(RSGISImageBandException& e)
		{
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					if(dsOffsets[i] != NULL)
					{
						delete[] dsOffsets[i];
					}
				}
				delete[] dsOffsets;
			}
			
			if(bandOffsets != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(bandOffsets[i] != NULL)
					{
						delete[] bandOffsets[i];
					}
				}
				delete[] bandOffsets;
			}
			
			if(inputData != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(inputData[i] != NULL)
					{
						delete[] inputData[i];
					}
				}
				delete[] inputData;
			}
			if(inDataColumn != NULL)
			{
				delete[] inDataColumn;
			}
			if(inputRasterBands != NULL)
			{
				delete[] inputRasterBands;
			}
			throw e;
		}
        
		if(gdalTranslation != NULL)
		{
			delete[] gdalTranslation;
		}
		
		if(dsOffsets != NULL)
		{
			for(int i = 0; i < numDS; i++)
			{
				if(dsOffsets[i] != NULL)
				{
					delete[] dsOffsets[i];
				}
			}
			delete[] dsOffsets;
		}
		
		if(bandOffsets != NULL)
		{
			for(int i = 0; i < numInBands; i++)
			{
				if(bandOffsets[i] != NULL)
				{
					delete[] bandOffsets[i];
				}
			}
			delete[] bandOffsets;
		}
		
		if(inputData != NULL)
		{
			for(int i = 0; i < numInBands; i++)
			{
				if(inputData[i] != NULL)
				{
					delete[] inputData[i];
				}
			}
			delete[] inputData;
		}
		if(inDataColumn != NULL)
		{
			delete[] inDataColumn;
		}
		if(inputRasterBands != NULL)
		{
			delete[] inputRasterBands;
		}
	}
    
    void RSGISCalcImage::calcImagePosPxl(GDALDataset **datasets, int numIntDS, int numFloatDS)
    {
        GDALAllRegister();
        RSGISImageUtils imgUtils;
        int numDS = numIntDS + numFloatDS;
        double *gdalTranslation = new double[6];
        int **dsOffsets = new int*[numDS];
        for(int i = 0; i < numDS; i++)
        {
            dsOffsets[i] = new int[2];
        }
        int **bandIntOffsets = NULL;
        int **bandFloatOffsets = NULL;
        int height = 0;
        int width = 0;
        int numIntBands = 0;
        int numFloatBands = 0;
        
        float **inputFloatData = NULL;
        float *inDataFloatColumn = NULL;
        unsigned int **inputIntData = NULL;
        long *inDataIntColumn = NULL;
        int xBlockSize = 0;
        int yBlockSize = 0;
        
        GDALRasterBand **inputRasterFloatBands = NULL;
        GDALRasterBand **inputRasterIntBands = NULL;
        
        geos::geom::Envelope extent;
        unsigned int xPxl = 0;
        unsigned int yPxl = 0;
        
        try
        {
            // Find image overlap
            imgUtils.getImageOverlap(datasets, numDS, dsOffsets, &width, &height, gdalTranslation, &xBlockSize, &yBlockSize);
            
            // Count number of image bands
            for(int i = 0; i < numIntDS; i++)
            {
                numIntBands += datasets[i]->GetRasterCount();
            }
            for(int i = numIntDS; i < numDS; i++)
            {
                numFloatBands += datasets[i]->GetRasterCount();
            }
            
            // Get Image Input Bands
            bandIntOffsets = new int*[numIntBands];
            inputRasterIntBands = new GDALRasterBand*[numIntBands];
            
            bandFloatOffsets = new int*[numFloatBands];
            inputRasterFloatBands = new GDALRasterBand*[numFloatBands];
            
            int counter = 0;
            for(int i = 0; i < numIntDS; i++)
            {
                for(int j = 0; j < datasets[i]->GetRasterCount(); j++)
                {
                    inputRasterIntBands[counter] = datasets[i]->GetRasterBand(j+1);
                    bandIntOffsets[counter] = new int[2];
                    bandIntOffsets[counter][0] = dsOffsets[i][0];
                    bandIntOffsets[counter][1] = dsOffsets[i][1];
                    counter++;
                }
            }
            
            counter = 0;
            for(int i = numIntDS; i < numDS; i++)
            {
                for(int j = 0; j < datasets[i]->GetRasterCount(); j++)
                {
                    inputRasterFloatBands[counter] = datasets[i]->GetRasterBand(j+1);
                    bandFloatOffsets[counter] = new int[2];
                    bandFloatOffsets[counter][0] = dsOffsets[i][0];
                    bandFloatOffsets[counter][1] = dsOffsets[i][1];
                    counter++;
                }
            }
            
            // Allocate memory
            inputIntData = new unsigned int*[numIntBands];
            for(int i = 0; i < numIntBands; i++)
            {
                inputIntData[i] = (unsigned int *) CPLMalloc(sizeof(unsigned int)*width*yBlockSize);
            }
            inDataIntColumn = new long[numIntBands];
            
            inputFloatData = new float*[numFloatBands];
            for(int i = 0; i < numFloatBands; i++)
            {
                inputFloatData[i] = (float *) CPLMalloc(sizeof(float)*width*yBlockSize);
            }
            inDataFloatColumn = new float[numFloatBands];
            
            
            int nYBlocks = height / yBlockSize;
            int remainRows = height - (nYBlocks * yBlockSize);
            int rowOffset = 0;
            
            int feedback = height/10;
            int feedbackCounter = 0;
            std::cout << "Started " << std::flush;
            // Loop images to process data
            for(int i = 0; i < nYBlocks; i++)
            {
                for(int n = 0; n < numIntBands; n++)
                {
                    rowOffset = bandIntOffsets[n][1] + (yBlockSize * i);
                    inputRasterIntBands[n]->RasterIO(GF_Read, bandIntOffsets[n][0], rowOffset, width, yBlockSize, inputIntData[n], width, yBlockSize, GDT_UInt32, 0, 0);
                }
                
                for(int n = 0; n < numFloatBands; n++)
                {
                    rowOffset = bandFloatOffsets[n][1] + (yBlockSize * i);
                    inputRasterFloatBands[n]->RasterIO(GF_Read, bandFloatOffsets[n][0], rowOffset, width, yBlockSize, inputFloatData[n], width, yBlockSize, GDT_Float32, 0, 0);
                }
                
                for(int m = 0; m < yBlockSize; ++m)
                {
                    if((feedback != 0) && ((((i*yBlockSize)+m) % feedback) == 0))
                    {
                        std::cout << "." << feedbackCounter << "." << std::flush;
                        feedbackCounter = feedbackCounter + 10;
                    }
                    
                    xPxl = 0;
                    for(int j = 0; j < width; j++)
                    {
                        for(int n = 0; n < numIntBands; n++)
                        {
                            inDataIntColumn[n] = inputIntData[n][(m*width)+j];
                        }
                        
                        for(int n = 0; n < numFloatBands; n++)
                        {
                            inDataFloatColumn[n] = inputFloatData[n][(m*width)+j];
                        }
                        
                        extent.init(xPxl, xPxl, yPxl, yPxl);
                        
                        this->calc->calcImageValue(inDataIntColumn, numIntBands, inDataFloatColumn, numFloatBands, extent);
                        
                        xPxl += 1;
                    }
                    yPxl += 1;
                }
            }
            
            if(remainRows > 0)
            {
                for(int n = 0; n < numIntBands; n++)
                {
                    rowOffset = bandIntOffsets[n][1] + (yBlockSize * nYBlocks);
                    inputRasterIntBands[n]->RasterIO(GF_Read, bandIntOffsets[n][0], rowOffset, width, remainRows, inputIntData[n], width, remainRows, GDT_UInt32, 0, 0);
                }
                
                
                for(int n = 0; n < numFloatBands; n++)
                {
                    rowOffset = bandFloatOffsets[n][1] + (yBlockSize * nYBlocks);
                    inputRasterFloatBands[n]->RasterIO(GF_Read, bandFloatOffsets[n][0], rowOffset, width, remainRows, inputFloatData[n], width, remainRows, GDT_Float32, 0, 0);
                }
                
                for(int m = 0; m < remainRows; ++m)
                {
                    if((feedback != 0) && ((((nYBlocks*yBlockSize)+m) % feedback) == 0))
                    {
                        std::cout << "." << feedbackCounter << "." << std::flush;
                        feedbackCounter = feedbackCounter + 10;
                    }
                    
                    xPxl = 0;
                    for(int j = 0; j < width; j++)
                    {
                        for(int n = 0; n < numIntBands; n++)
                        {
                            inDataIntColumn[n] = inputIntData[n][(m*width)+j];
                        }
                        
                        for(int n = 0; n < numFloatBands; n++)
                        {
                            inDataFloatColumn[n] = inputFloatData[n][(m*width)+j];
                        }
                        
                        extent.init(xPxl, xPxl, yPxl, yPxl);
                        
                        this->calc->calcImageValue(inDataIntColumn, numIntBands, inDataFloatColumn, numFloatBands, extent);
                        xPxl += 1;
                    }
                    yPxl += 1;
                }
            }
            std::cout << " Complete.\n";
        }
        catch(RSGISImageCalcException& e)
        {
            if(gdalTranslation != NULL)
            {
                delete[] gdalTranslation;
            }
            
            if(dsOffsets != NULL)
            {
                for(int i = 0; i < numDS; i++)
                {
                    if(dsOffsets[i] != NULL)
                    {
                        delete[] dsOffsets[i];
                    }
                }
                delete[] dsOffsets;
            }
            
            if(bandIntOffsets != NULL)
            {
                for(int i = 0; i < numIntBands; i++)
                {
                    if(bandIntOffsets[i] != NULL)
                    {
                        delete[] bandIntOffsets[i];
                    }
                }
                delete[] bandIntOffsets;
            }
            
            if(bandFloatOffsets != NULL)
            {
                for(int i = 0; i < numFloatBands; i++)
                {
                    if(bandFloatOffsets[i] != NULL)
                    {
                        delete[] bandFloatOffsets[i];
                    }
                }
                delete[] bandFloatOffsets;
            }
            
            if(inputIntData != NULL)
            {
                for(int i = 0; i < numIntBands; i++)
                {
                    if(inputIntData[i] != NULL)
                    {
                        CPLFree(inputIntData[i]);
                    }
                }
                delete[] inputIntData;
            }
            
            if(inputFloatData != NULL)
            {
                for(int i = 0; i < numFloatBands; i++)
                {
                    if(inputFloatData[i] != NULL)
                    {
                        delete[] inputFloatData[i];
                    }
                }
                delete[] inputFloatData;
            }
            
            if(inDataIntColumn != NULL)
            {
                delete[] inDataIntColumn;
            }
            
            if(inDataFloatColumn != NULL)
            {
                delete[] inDataFloatColumn;
            }
            
            if(inputRasterIntBands != NULL)
            {
                delete[] inputRasterIntBands;
            }
            
            if(inputRasterFloatBands != NULL)
            {
                delete[] inputRasterFloatBands;
            }
            throw e;
        }
        catch(RSGISImageBandException& e)
        {
            if(gdalTranslation != NULL)
            {
                delete[] gdalTranslation;
            }
            
            if(dsOffsets != NULL)
            {
                for(int i = 0; i < numDS; i++)
                {
                    if(dsOffsets[i] != NULL)
                    {
                        delete[] dsOffsets[i];
                    }
                }
                delete[] dsOffsets;
            }
            
            if(bandIntOffsets != NULL)
            {
                for(int i = 0; i < numIntBands; i++)
                {
                    if(bandIntOffsets[i] != NULL)
                    {
                        delete[] bandIntOffsets[i];
                    }
                }
                delete[] bandIntOffsets;
            }
            
            if(bandFloatOffsets != NULL)
            {
                for(int i = 0; i < numFloatBands; i++)
                {
                    if(bandFloatOffsets[i] != NULL)
                    {
                        delete[] bandFloatOffsets[i];
                    }
                }
                delete[] bandFloatOffsets;
            }
            
            if(inputIntData != NULL)
            {
                for(int i = 0; i < numIntBands; i++)
                {
                    if(inputIntData[i] != NULL)
                    {
                        CPLFree(inputIntData[i]);
                    }
                }
                delete[] inputIntData;
            }
            
            if(inputFloatData != NULL)
            {
                for(int i = 0; i < numFloatBands; i++)
                {
                    if(inputFloatData[i] != NULL)
                    {
                        delete[] inputFloatData[i];
                    }
                }
                delete[] inputFloatData;
            }
            
            if(inDataIntColumn != NULL)
            {
                delete[] inDataIntColumn;
            }
            
            if(inDataFloatColumn != NULL)
            {
                delete[] inDataFloatColumn;
            }
            
            if(inputRasterIntBands != NULL)
            {
                delete[] inputRasterIntBands;
            }
            
            if(inputRasterFloatBands != NULL)
            {
                delete[] inputRasterFloatBands;
            }
            throw e;
        }
        
        if(gdalTranslation != NULL)
        {
            delete[] gdalTranslation;
        }
        
        if(dsOffsets != NULL)
        {
            for(int i = 0; i < numDS; i++)
            {
                if(dsOffsets[i] != NULL)
                {
                    delete[] dsOffsets[i];
                }
            }
            delete[] dsOffsets;
        }
        
        if(bandIntOffsets != NULL)
        {
            for(int i = 0; i < numIntBands; i++)
            {
                if(bandIntOffsets[i] != NULL)
                {
                    delete[] bandIntOffsets[i];
                }
            }
            delete[] bandIntOffsets;
        }
        
        if(bandFloatOffsets != NULL)
        {
            for(int i = 0; i < numFloatBands; i++)
            {
                if(bandFloatOffsets[i] != NULL)
                {
                    delete[] bandFloatOffsets[i];
                }
            }
            delete[] bandFloatOffsets;
        }
        
        if(inputIntData != NULL)
        {
            for(int i = 0; i < numIntBands; i++)
            {
                if(inputIntData[i] != NULL)
                {
                    CPLFree(inputIntData[i]);
                }
            }
            delete[] inputIntData;
        }
        
        if(inputFloatData != NULL)
        {
            for(int i = 0; i < numFloatBands; i++)
            {
                if(inputFloatData[i] != NULL)
                {
                    delete[] inputFloatData[i];
                }
            }
            delete[] inputFloatData;
        }
        
        if(inDataIntColumn != NULL)
        {
            delete[] inDataIntColumn;
        }
        
        if(inDataFloatColumn != NULL)
        {
            delete[] inDataFloatColumn;
        }
        
        if(inputRasterIntBands != NULL)
        {
            delete[] inputRasterIntBands;
        }
        
        if(inputRasterFloatBands != NULL)
        {
            delete[] inputRasterFloatBands;
        }
    }
    
    void RSGISCalcImage::calcImageExtent(GDALDataset **datasets, int numDS, geos::geom::Envelope *env, bool quiet)
	{
		GDALAllRegister();
		RSGISImageUtils imgUtils;
		double *gdalTranslation = new double[6];
		int **dsOffsets = new int*[numDS];
		for(int i = 0; i < numDS; i++)
		{
			dsOffsets[i] = new int[2];
		}
		int **bandOffsets = NULL;
		int height = 0;
		int width = 0;
		int numInBands = 0;
		
		float **inputData = NULL;
		float *inDataColumn = NULL;
		
		GDALRasterBand **inputRasterBands = NULL;
        
        geos::geom::Envelope extent;
		double pxlTLX = 0;
		double pxlTLY = 0;
		double pxlWidth = 0;
		double pxlHeight = 0;
		
		try
		{
			// Find image overlap
            if(env == NULL)
            {
                imgUtils.getImageOverlap(datasets, numDS, dsOffsets, &width, &height, gdalTranslation);
            }
            else
            {
                imgUtils.getImageOverlapCut2Env(datasets, numDS, dsOffsets, &width, &height, gdalTranslation, env);
            }
			
			// Count number of image bands
			for(int i = 0; i < numDS; i++)
			{
				numInBands += datasets[i]->GetRasterCount();
			}
			
			pxlTLX = gdalTranslation[0];
			pxlTLY = gdalTranslation[3];
			pxlWidth = gdalTranslation[1];
			pxlHeight = gdalTranslation[5];
            
            if(pxlHeight < 0)
            {
                pxlHeight *= (-1);
            }
			
			// Get Image Input Bands
			bandOffsets = new int*[numInBands];
			inputRasterBands = new GDALRasterBand*[numInBands];
			int counter = 0;
			for(int i = 0; i < numDS; i++)
			{
				for(int j = 0; j < datasets[i]->GetRasterCount(); j++)
				{
					inputRasterBands[counter] = datasets[i]->GetRasterBand(j+1);
					bandOffsets[counter] = new int[2];
					bandOffsets[counter][0] = dsOffsets[i][0];
					bandOffsets[counter][1] = dsOffsets[i][1];
					counter++;
				}
			}
			
			// Allocate memory
			inputData = new float*[numInBands];
			for(int i = 0; i < numInBands; i++)
			{
				inputData[i] = (float *) CPLMalloc(sizeof(float)*width);
			}
			inDataColumn = new float[numInBands];
			
			int feedback = height/10;
			int feedbackCounter = 0;
            if(!quiet)
            {
                std::cout << "Started " << std::flush;
            }
			// Loop images to process data
			for(int i = 0; i < height; i++)
			{
				if((!quiet) && (feedback != 0) && ((i % feedback) == 0))
				{
                    std::cout << "." << feedbackCounter << "." << std::flush;
                    feedbackCounter = feedbackCounter + 10;
				}
				
				for(int n = 0; n < numInBands; n++)
				{
					inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], (bandOffsets[n][1]+i), width, 1, inputData[n], width, 1, GDT_Float32, 0, 0);
				}

                for(int j = 0; j < width; j++)
				{
                    for(int n = 0; n < numInBands; n++)
					{
						inDataColumn[n] = inputData[n][j];
					}
					
					extent.init(pxlTLX, (pxlTLX+pxlWidth), pxlTLY, (pxlTLY-pxlHeight));
					
					this->calc->calcImageValue(inDataColumn, numInBands, extent);
					
					pxlTLX += pxlWidth;
				}
				pxlTLY -= pxlHeight;
				pxlTLX = gdalTranslation[0];
			}
            if(!quiet)
            {
                std::cout << " Complete.\n";
            }
		}
		catch(RSGISImageCalcException& e)
		{
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					if(dsOffsets[i] != NULL)
					{
						delete[] dsOffsets[i];
					}
				}
				delete[] dsOffsets;
			}
			
			if(bandOffsets != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(bandOffsets[i] != NULL)
					{
						delete[] bandOffsets[i];
					}
				}
				delete[] bandOffsets;
			}
			
			if(inputData != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(inputData[i] != NULL)
					{
						delete[] inputData[i];
					}
				}
				delete[] inputData;
			}
			
			if(inDataColumn != NULL)
			{
				delete[] inDataColumn;
			}
			
			if(inputRasterBands != NULL)
			{
				delete[] inputRasterBands;
			}
			throw e;
		}
		catch(RSGISImageBandException& e)
		{
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					if(dsOffsets[i] != NULL)
					{
						delete[] dsOffsets[i];
					}
				}
				delete[] dsOffsets;
			}
			
			if(bandOffsets != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(bandOffsets[i] != NULL)
					{
						delete[] bandOffsets[i];
					}
				}
				delete[] bandOffsets;
			}
			
			if(inputData != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(inputData[i] != NULL)
					{
						delete[] inputData[i];
					}
				}
				delete[] inputData;
			}
			
			if(inDataColumn != NULL)
			{
				delete[] inDataColumn;
			}
			
			if(inputRasterBands != NULL)
			{
				delete[] inputRasterBands;
			}
			throw e;
		}
        
		if(gdalTranslation != NULL)
		{
			delete[] gdalTranslation;
		}
		
		if(dsOffsets != NULL)
		{
			for(int i = 0; i < numDS; i++)
			{
				if(dsOffsets[i] != NULL)
				{
					delete[] dsOffsets[i];
				}
			}
			delete[] dsOffsets;
		}
		
		if(bandOffsets != NULL)
		{
			for(int i = 0; i < numInBands; i++)
			{
				if(bandOffsets[i] != NULL)
				{
					delete[] bandOffsets[i];
				}
			}
			delete[] bandOffsets;
		}
		
		if(inputData != NULL)
		{
			for(int i = 0; i < numInBands; i++)
			{
				if(inputData[i] != NULL)
				{
					delete[] inputData[i];
				}
			}
			delete[] inputData;
		}
        
		if(inDataColumn != NULL)
		{
			delete[] inDataColumn;
		}
        
		if(inputRasterBands != NULL)
		{
			delete[] inputRasterBands;
		}
	}
    
    void RSGISCalcImage::calcImageExtent(GDALDataset **datasets, int numIntDS, int numFloatDS)
    {
        GDALAllRegister();
		RSGISImageUtils imgUtils;
        int numDS = numIntDS + numFloatDS;
		double *gdalTranslation = new double[6];
		int **dsOffsets = new int*[numDS];
		for(int i = 0; i < numDS; i++)
		{
			dsOffsets[i] = new int[2];
		}
		int **bandIntOffsets = NULL;
        int **bandFloatOffsets = NULL;
		int height = 0;
		int width = 0;
		int numIntBands = 0;
        int numFloatBands = 0;
		
		float **inputFloatData = NULL;
		float *inDataFloatColumn = NULL;
        unsigned int **inputIntData = NULL;
		long *inDataIntColumn = NULL;
        int xBlockSize = 0;
        int yBlockSize = 0;
		
		GDALRasterBand **inputRasterFloatBands = NULL;
        GDALRasterBand **inputRasterIntBands = NULL;
        
        geos::geom::Envelope extent;
        double imgTLX = 0;
        double imgTLY = 0;
        double pxlTLX = 0;
        double pxlTLY = 0;
        double pxlWidth = 0;
        double pxlHeight = 0;
		
		try
		{
			// Find image overlap
			imgUtils.getImageOverlap(datasets, numDS, dsOffsets, &width, &height, gdalTranslation, &xBlockSize, &yBlockSize);
            
            
            imgTLX = gdalTranslation[0];
			imgTLY = gdalTranslation[3];
			pxlWidth = gdalTranslation[1];
			pxlHeight = gdalTranslation[5];
            
            if(pxlHeight < 0)
            {
                pxlHeight *= (-1);
            }
            
			// Count number of image bands
			for(int i = 0; i < numIntDS; i++)
			{
				numIntBands += datasets[i]->GetRasterCount();
			}
            for(int i = numIntDS; i < numDS; i++)
			{
				numFloatBands += datasets[i]->GetRasterCount();
			}
			
			// Get Image Input Bands
			bandIntOffsets = new int*[numIntBands];
			inputRasterIntBands = new GDALRasterBand*[numIntBands];
            
            bandFloatOffsets = new int*[numFloatBands];
			inputRasterFloatBands = new GDALRasterBand*[numFloatBands];
            
			int counter = 0;
			for(int i = 0; i < numIntDS; i++)
			{
				for(int j = 0; j < datasets[i]->GetRasterCount(); j++)
				{
					inputRasterIntBands[counter] = datasets[i]->GetRasterBand(j+1);
					bandIntOffsets[counter] = new int[2];
					bandIntOffsets[counter][0] = dsOffsets[i][0];
					bandIntOffsets[counter][1] = dsOffsets[i][1];
					counter++;
				}
			}
            
            counter = 0;
            for(int i = numIntDS; i < numDS; i++)
			{
				for(int j = 0; j < datasets[i]->GetRasterCount(); j++)
				{
					inputRasterFloatBands[counter] = datasets[i]->GetRasterBand(j+1);
					bandFloatOffsets[counter] = new int[2];
					bandFloatOffsets[counter][0] = dsOffsets[i][0];
					bandFloatOffsets[counter][1] = dsOffsets[i][1];
					counter++;
				}
			}
			
			// Allocate memory
			inputIntData = new unsigned int*[numIntBands];
			for(int i = 0; i < numIntBands; i++)
			{
				inputIntData[i] = (unsigned int *) CPLMalloc(sizeof(unsigned int)*width*yBlockSize);
			}
			inDataIntColumn = new long[numIntBands];
            
            inputFloatData = new float*[numFloatBands];
			for(int i = 0; i < numFloatBands; i++)
			{
				inputFloatData[i] = (float *) CPLMalloc(sizeof(float)*width*yBlockSize);
			}
			inDataFloatColumn = new float[numFloatBands];
            
            
            int nYBlocks = height / yBlockSize;
            int remainRows = height - (nYBlocks * yBlockSize);
            int rowOffset = 0;
            pxlTLY = imgTLY;
            
			int feedback = height/10;
			int feedbackCounter = 0;
			std::cout << "Started " << std::flush;
			// Loop images to process data
			for(int i = 0; i < nYBlocks; i++)
			{
				for(int n = 0; n < numIntBands; n++)
				{
                    rowOffset = bandIntOffsets[n][1] + (yBlockSize * i);
					inputRasterIntBands[n]->RasterIO(GF_Read, bandIntOffsets[n][0], rowOffset, width, yBlockSize, inputIntData[n], width, yBlockSize, GDT_UInt32, 0, 0);
				}
                
                for(int n = 0; n < numFloatBands; n++)
				{
                    rowOffset = bandFloatOffsets[n][1] + (yBlockSize * i);
					inputRasterFloatBands[n]->RasterIO(GF_Read, bandFloatOffsets[n][0], rowOffset, width, yBlockSize, inputFloatData[n], width, yBlockSize, GDT_Float32, 0, 0);
				}
                
                for(int m = 0; m < yBlockSize; ++m)
                {
                    if((feedback != 0) && ((((i*yBlockSize)+m) % feedback) == 0))
                    {
                        std::cout << "." << feedbackCounter << "." << std::flush;
                        feedbackCounter = feedbackCounter + 10;
                    }
                    
                    pxlTLX = imgTLX;
                    for(int j = 0; j < width; j++)
                    {
                        for(int n = 0; n < numIntBands; n++)
                        {
                            inDataIntColumn[n] = inputIntData[n][(m*width)+j];
                        }
                        
                        for(int n = 0; n < numFloatBands; n++)
                        {
                            inDataFloatColumn[n] = inputFloatData[n][(m*width)+j];
                        }
                        
                        extent.init(pxlTLX, (pxlTLX+pxlWidth), pxlTLY, (pxlTLY-pxlHeight));
                        
                        this->calc->calcImageValue(inDataIntColumn, numIntBands, inDataFloatColumn, numFloatBands, extent);
                        
                        pxlTLX += pxlWidth;
                    }
                    pxlTLY -= pxlHeight;
                }
			}
            
            if(remainRows > 0)
            {
                for(int n = 0; n < numIntBands; n++)
				{
                    rowOffset = bandIntOffsets[n][1] + (yBlockSize * nYBlocks);
					inputRasterIntBands[n]->RasterIO(GF_Read, bandIntOffsets[n][0], rowOffset, width, remainRows, inputIntData[n], width, remainRows, GDT_UInt32, 0, 0);
				}
                
                
                for(int n = 0; n < numFloatBands; n++)
				{
                    rowOffset = bandFloatOffsets[n][1] + (yBlockSize * nYBlocks);
					inputRasterFloatBands[n]->RasterIO(GF_Read, bandFloatOffsets[n][0], rowOffset, width, remainRows, inputFloatData[n], width, remainRows, GDT_Float32, 0, 0);
				}
                
                for(int m = 0; m < remainRows; ++m)
                {
                    if((feedback != 0) && ((((nYBlocks*yBlockSize)+m) % feedback) == 0))
                    {
                        std::cout << "." << feedbackCounter << "." << std::flush;
                        feedbackCounter = feedbackCounter + 10;
                    }
                    
                    pxlTLX = imgTLX;
                    for(int j = 0; j < width; j++)
                    {
                        for(int n = 0; n < numIntBands; n++)
                        {
                            inDataIntColumn[n] = inputIntData[n][(m*width)+j];
                        }
                        
                        for(int n = 0; n < numFloatBands; n++)
                        {
                            inDataFloatColumn[n] = inputFloatData[n][(m*width)+j];
                        }
                        
                        extent.init(pxlTLX, (pxlTLX+pxlWidth), pxlTLY, (pxlTLY-pxlHeight));
                        
                        this->calc->calcImageValue(inDataIntColumn, numIntBands, inDataFloatColumn, numFloatBands, extent);
                        pxlTLX += pxlWidth;
                    }
                    pxlTLY -= pxlHeight;
                }
            }
			std::cout << " Complete.\n";
		}
		catch(RSGISImageCalcException& e)
		{
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					if(dsOffsets[i] != NULL)
					{
						delete[] dsOffsets[i];
					}
				}
				delete[] dsOffsets;
			}
			
			if(bandIntOffsets != NULL)
			{
				for(int i = 0; i < numIntBands; i++)
				{
					if(bandIntOffsets[i] != NULL)
					{
						delete[] bandIntOffsets[i];
					}
				}
				delete[] bandIntOffsets;
			}
            
            if(bandFloatOffsets != NULL)
			{
				for(int i = 0; i < numFloatBands; i++)
				{
					if(bandFloatOffsets[i] != NULL)
					{
						delete[] bandFloatOffsets[i];
					}
				}
				delete[] bandFloatOffsets;
			}
			
			if(inputIntData != NULL)
			{
				for(int i = 0; i < numIntBands; i++)
				{
					if(inputIntData[i] != NULL)
					{
						CPLFree(inputIntData[i]);
					}
				}
				delete[] inputIntData;
			}
            
            if(inputFloatData != NULL)
			{
				for(int i = 0; i < numFloatBands; i++)
				{
					if(inputFloatData[i] != NULL)
					{
						delete[] inputFloatData[i];
					}
				}
				delete[] inputFloatData;
			}
            
			if(inDataIntColumn != NULL)
			{
				delete[] inDataIntColumn;
			}
            
            if(inDataFloatColumn != NULL)
			{
				delete[] inDataFloatColumn;
			}
            
			if(inputRasterIntBands != NULL)
			{
				delete[] inputRasterIntBands;
			}
            
            if(inputRasterFloatBands != NULL)
			{
				delete[] inputRasterFloatBands;
			}
			throw e;
		}
		catch(RSGISImageBandException& e)
		{
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					if(dsOffsets[i] != NULL)
					{
						delete[] dsOffsets[i];
					}
				}
				delete[] dsOffsets;
			}
			
			if(bandIntOffsets != NULL)
			{
				for(int i = 0; i < numIntBands; i++)
				{
					if(bandIntOffsets[i] != NULL)
					{
						delete[] bandIntOffsets[i];
					}
				}
				delete[] bandIntOffsets;
			}
            
            if(bandFloatOffsets != NULL)
			{
				for(int i = 0; i < numFloatBands; i++)
				{
					if(bandFloatOffsets[i] != NULL)
					{
						delete[] bandFloatOffsets[i];
					}
				}
				delete[] bandFloatOffsets;
			}
			
			if(inputIntData != NULL)
			{
				for(int i = 0; i < numIntBands; i++)
				{
					if(inputIntData[i] != NULL)
					{
						CPLFree(inputIntData[i]);
					}
				}
				delete[] inputIntData;
			}
            
            if(inputFloatData != NULL)
			{
				for(int i = 0; i < numFloatBands; i++)
				{
					if(inputFloatData[i] != NULL)
					{
						delete[] inputFloatData[i];
					}
				}
				delete[] inputFloatData;
			}
            
			if(inDataIntColumn != NULL)
			{
				delete[] inDataIntColumn;
			}
            
            if(inDataFloatColumn != NULL)
			{
				delete[] inDataFloatColumn;
			}
            
			if(inputRasterIntBands != NULL)
			{
				delete[] inputRasterIntBands;
			}
            
            if(inputRasterFloatBands != NULL)
			{
				delete[] inputRasterFloatBands;
			}
			throw e;
		}
        
		if(gdalTranslation != NULL)
        {
            delete[] gdalTranslation;
        }
        
        if(dsOffsets != NULL)
        {
            for(int i = 0; i < numDS; i++)
            {
                if(dsOffsets[i] != NULL)
                {
                    delete[] dsOffsets[i];
                }
            }
            delete[] dsOffsets;
        }
        
        if(bandIntOffsets != NULL)
        {
            for(int i = 0; i < numIntBands; i++)
            {
                if(bandIntOffsets[i] != NULL)
                {
                    delete[] bandIntOffsets[i];
                }
            }
            delete[] bandIntOffsets;
        }
        
        if(bandFloatOffsets != NULL)
        {
            for(int i = 0; i < numFloatBands; i++)
            {
                if(bandFloatOffsets[i] != NULL)
                {
                    delete[] bandFloatOffsets[i];
                }
            }
            delete[] bandFloatOffsets;
        }
        
        if(inputIntData != NULL)
        {
            for(int i = 0; i < numIntBands; i++)
            {
                if(inputIntData[i] != NULL)
                {
                    CPLFree(inputIntData[i]);
                }
            }
            delete[] inputIntData;
        }
        
        if(inputFloatData != NULL)
        {
            for(int i = 0; i < numFloatBands; i++)
            {
                if(inputFloatData[i] != NULL)
                {
                    delete[] inputFloatData[i];
                }
            }
            delete[] inputFloatData;
        }
        
        if(inDataIntColumn != NULL)
        {
            delete[] inDataIntColumn;
        }
        
        if(inDataFloatColumn != NULL)
        {
            delete[] inDataFloatColumn;
        }
        
        if(inputRasterIntBands != NULL)
        {
            delete[] inputRasterIntBands;
        }
        
        if(inputRasterFloatBands != NULL)
        {
            delete[] inputRasterFloatBands;
        }
    }
	
	void RSGISCalcImage::calcImageExtent(GDALDataset **datasets, int numDS, std::string outputImage, std::string gdalFormat, GDALDataType gdalDataType)
	{
		GDALAllRegister();
		RSGISImageUtils imgUtils;
		double *gdalTranslation = new double[6];
		int **dsOffsets = new int*[numDS];
		for(int i = 0; i < numDS; i++)
		{
			dsOffsets[i] = new int[2];
		}
		int **bandOffsets = NULL;
		int height = 0;
		int width = 0;
		int numInBands = 0;
		
		float **inputData = NULL;
		double **outputData = NULL;
		float *inDataColumn = NULL;
		double *outDataColumn = NULL;
		
		GDALDataset *outputImageDS = NULL;
		GDALRasterBand **inputRasterBands = NULL;
		GDALRasterBand **outputRasterBands = NULL;
		GDALDriver *gdalDriver = NULL;
		geos::geom::Envelope extent;
		double pxlTLX = 0;
		double pxlTLY = 0;
		double pxlWidth = 0;
		double pxlHeight = 0;
		
		try
		{
			// Find image overlap
			imgUtils.getImageOverlap(datasets, numDS, dsOffsets, &width, &height, gdalTranslation);
			
			// Count number of image bands
			for(int i = 0; i < numDS; i++)
			{
				numInBands += datasets[i]->GetRasterCount();
			}
			
			// Create new Image
			gdalDriver = GetGDALDriverManager()->GetDriverByName(gdalFormat.c_str());
			if(gdalDriver == NULL)
			{
				throw RSGISImageBandException(gdalFormat + std::string(" driver does not exists.."));
			}

			outputImageDS = gdalDriver->Create(outputImage.c_str(), width, height, this->numOutBands, gdalDataType, NULL);
			
			if(outputImageDS == NULL)
			{
				throw RSGISImageBandException("Output image could not be created. Check filepath.");
			}
			outputImageDS->SetGeoTransform(gdalTranslation);
			if(useImageProj)
			{
				outputImageDS->SetProjection(datasets[0]->GetProjectionRef());
			}
			else
			{
				outputImageDS->SetProjection(proj.c_str());
			}
			
			pxlTLX = gdalTranslation[0];
			pxlTLY = gdalTranslation[3];
			pxlWidth = gdalTranslation[1];
			pxlHeight = gdalTranslation[5];
            
            if(pxlHeight < 0)
            {
                pxlHeight *= (-1);
            }
			
			// Get Image Input Bands
			bandOffsets = new int*[numInBands];
			inputRasterBands = new GDALRasterBand*[numInBands];
			int counter = 0;
			for(int i = 0; i < numDS; i++)
			{
				for(int j = 0; j < datasets[i]->GetRasterCount(); j++)
				{
					inputRasterBands[counter] = datasets[i]->GetRasterBand(j+1);
					bandOffsets[counter] = new int[2];
					bandOffsets[counter][0] = dsOffsets[i][0];
					bandOffsets[counter][1] = dsOffsets[i][1];
					counter++;
				}
			}
			
			//Get Image Output Bands
			outputRasterBands = new GDALRasterBand*[this->numOutBands];
			for(int i = 0; i < this->numOutBands; i++)
			{
				outputRasterBands[i] = outputImageDS->GetRasterBand(i+1);
			}
			
			// Allocate memory
			inputData = new float*[numInBands];
			for(int i = 0; i < numInBands; i++)
			{
				inputData[i] = (float *) CPLMalloc(sizeof(float)*width);
			}
			inDataColumn = new float[numInBands];
			
			outputData = new double*[this->numOutBands];
			for(int i = 0; i < this->numOutBands; i++)
			{
				outputData[i] = (double *) CPLMalloc(sizeof(double)*width);
			}
			outDataColumn = new double[this->numOutBands];
			
			int feedback = height/10;
			int feedbackCounter = 0;
			std::cout << "Started " << std::flush;
			// Loop images to process data
			for(int i = 0; i < height; i++)
			{
				if((feedback != 0) && ((i % feedback) == 0))
				{
					std::cout << "." << feedbackCounter << "." << std::flush;
					feedbackCounter = feedbackCounter + 10;
				}
				
				for(int n = 0; n < numInBands; n++)
				{
					inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], (bandOffsets[n][1]+i), width, 1, inputData[n], width, 1, GDT_Float32, 0, 0);
				}
				
				for(int j = 0; j < width; j++)
				{
					for(int n = 0; n < numInBands; n++)
					{
						inDataColumn[n] = inputData[n][j];
					}
					
					extent.init(pxlTLX, (pxlTLX+pxlWidth), pxlTLY, (pxlTLY-pxlHeight));
					
					this->calc->calcImageValue(inDataColumn, numInBands, outDataColumn, extent);
					
					pxlTLX += pxlWidth;
					
					for(int n = 0; n < this->numOutBands; n++)
					{
						outputData[n][j] = outDataColumn[n];
					}
					
				}
				pxlTLY -= pxlHeight;
				pxlTLX = gdalTranslation[0];
				
				for(int n = 0; n < this->numOutBands; n++)
				{
					outputRasterBands[n]->RasterIO(GF_Write, 0, i, width, 1, outputData[n], width, 1, GDT_Float64, 0, 0);
				}
			}
			std::cout << " Complete.\n";
		}
		catch(RSGISImageCalcException& e)
		{			
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					if(dsOffsets[i] != NULL)
					{
						delete[] dsOffsets[i];
					}
				} 
				delete[] dsOffsets;
			}
			
			if(bandOffsets != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(bandOffsets[i] != NULL)
					{
						delete[] bandOffsets[i];
					}
				}
				delete[] bandOffsets;
			}			
			
			if(inputData != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(inputData[i] != NULL)
					{
						delete[] inputData[i];
					}
				}
				delete[] inputData;
			}
			
			if(outputData != NULL)
			{
				for(int i = 0; i < this->numOutBands; i++)
				{
					if(outputData[i] != NULL)
					{
						delete[] outputData[i];
					}
				}
				delete[] outputData;
			}
			
			if(inDataColumn != NULL)
			{
				delete[] inDataColumn;
			}
			
			if(outDataColumn != NULL)
			{
				delete[] outDataColumn;
			}
			
			if(inputRasterBands != NULL)
			{
				delete[] inputRasterBands;
			}
			
			if(outputRasterBands != NULL)
			{
				delete[] outputRasterBands;
			}
			throw e;
		}
		catch(RSGISImageBandException& e)
		{			
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					if(dsOffsets[i] != NULL)
					{
						delete[] dsOffsets[i];
					}
				} 
				delete[] dsOffsets;
			}
			
			if(bandOffsets != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(bandOffsets[i] != NULL)
					{
						delete[] bandOffsets[i];
					}
				}
				delete[] bandOffsets;
			}			
			
			if(inputData != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(inputData[i] != NULL)
					{
						delete[] inputData[i];
					}
				}
				delete[] inputData;
			}
			
			if(outputData != NULL)
			{
				for(int i = 0; i < this->numOutBands; i++)
				{
					if(outputData[i] != NULL)
					{
						delete[] outputData[i];
					}
				}
				delete[] outputData;
			}
			
			if(inDataColumn != NULL)
			{
				delete[] inDataColumn;
			}
			
			if(outDataColumn != NULL)
			{
				delete[] outDataColumn;
			}
			
			if(inputRasterBands != NULL)
			{
				delete[] inputRasterBands;
			}
			
			if(outputRasterBands != NULL)
			{
				delete[] outputRasterBands;
			}
			throw e;
		}
		
		GDALClose(outputImageDS);
		
		if(gdalTranslation != NULL)
		{
			delete[] gdalTranslation;
		}
		
		if(dsOffsets != NULL)
		{
			for(int i = 0; i < numDS; i++)
			{
				if(dsOffsets[i] != NULL)
				{
					delete[] dsOffsets[i];
				}
			} 
			delete[] dsOffsets;
		}
		
		if(bandOffsets != NULL)
		{
			for(int i = 0; i < numInBands; i++)
			{
				if(bandOffsets[i] != NULL)
				{
					delete[] bandOffsets[i];
				}
			}
			delete[] bandOffsets;
		}		
		
		if(inputData != NULL)
		{
			for(int i = 0; i < numInBands; i++)
			{
				if(inputData[i] != NULL)
				{
					delete[] inputData[i];
				}
			}
			delete[] inputData;
		}
		
		if(outputData != NULL)
		{
			for(int i = 0; i < this->numOutBands; i++)
			{
				if(outputData[i] != NULL)
				{
					delete[] outputData[i];
				}
			}
			delete[] outputData;
		}
		
		if(inDataColumn != NULL)
		{
			delete[] inDataColumn;
		}
		
		if(outDataColumn != NULL)
		{
			delete[] outDataColumn;
		}
		
		if(inputRasterBands != NULL)
		{
			delete[] inputRasterBands;
		}
		
		if(outputRasterBands != NULL)
		{
			delete[] outputRasterBands;
		}
	}
	
    void RSGISCalcImage::calcImageWindowData(GDALDataset **datasets, int numDS, int windowSize)
	{
		GDALAllRegister();
		RSGISImageUtils imgUtils;
		double *gdalTranslation = new double[6];
		int **dsOffsets = new int*[numDS];
		for(int i = 0; i < numDS; i++)
		{
			dsOffsets[i] = new int[2];
		}
		int **bandOffsets = NULL;
		int height = 0;
		int width = 0;
		int numInBands = 0;
        int xBlockSize = 0;
        int yBlockSize = 0;
        size_t numPxlsInBlock = 0;
		
        float **inputDataUpper = NULL;
		float **inputDataMain = NULL;
        float **inputDataLower = NULL;
		float ***inDataBlock = NULL;
		double *outDataColumn = NULL;
		
		GDALRasterBand **inputRasterBands = NULL;
		
		try
		{
			if(windowSize % 2 == 0)
			{
				throw RSGISImageCalcException("Window size needs to be an odd number (min = 3).");
			}
			else if(windowSize < 3)
			{
				throw RSGISImageCalcException("Window size needs to be 3 or greater and an odd number.");
			}
			int windowMid = floor(((float)windowSize)/2.0); // Starting at 0!! NOT 1 otherwise would be ceil.
			
            // Find image overlap
            imgUtils.getImageOverlap(datasets, numDS, dsOffsets, &width, &height, gdalTranslation, &xBlockSize, &yBlockSize);
			
			// Count number of image bands
			for(int i = 0; i < numDS; i++)
			{
				numInBands += datasets[i]->GetRasterCount();
			}
			
			// Get Image Input Bands
			bandOffsets = new int*[numInBands];
			inputRasterBands = new GDALRasterBand*[numInBands];
			int counter = 0;
			for(int i = 0; i < numDS; i++)
			{
				for(int j = 0; j < datasets[i]->GetRasterCount(); j++)
				{
					inputRasterBands[counter] = datasets[i]->GetRasterBand(j+1);
					bandOffsets[counter] = new int[2];
					bandOffsets[counter][0] = dsOffsets[i][0];
					bandOffsets[counter][1] = dsOffsets[i][1];
					counter++;
				}
			}
            
            int numOfLines = yBlockSize;
            if(yBlockSize < windowSize)
            {
                numOfLines = ceil(((float)windowSize)/((float)yBlockSize))*yBlockSize;
            }
            
			// Allocate memory
            numPxlsInBlock = width*numOfLines;
			inputDataUpper = new float*[numInBands];
			for(int i = 0; i < numInBands; i++)
			{
                inputDataUpper[i] = (float *) CPLMalloc(sizeof(float)*numPxlsInBlock);
                for(int k = 0; k < numPxlsInBlock; k++)
                {
                    inputDataUpper[i][k] = 0;
                }
			}
            
            inputDataMain = new float*[numInBands];
			for(int i = 0; i < numInBands; i++)
			{
                inputDataMain[i] = (float *) CPLMalloc(sizeof(float)*numPxlsInBlock);
                for(int k = 0; k < numPxlsInBlock; k++)
                {
                    inputDataMain[i][k] = 0;
                }
			}
            
            inputDataLower = new float*[numInBands];
			for(int i = 0; i < numInBands; i++)
			{
                inputDataLower[i] = (float *) CPLMalloc(sizeof(float)*numPxlsInBlock);
                for(int k = 0; k < numPxlsInBlock; k++)
                {
                    inputDataLower[i][k] = 0;
                }
			}
			
			inDataBlock = new float**[numInBands];
			for(int i = 0; i < numInBands; i++)
			{
				inDataBlock[i] = new float*[windowSize];
				for(int j = 0; j < windowSize; j++)
				{
					inDataBlock[i][j] = new float[windowSize];
				}
			}
			
			outDataColumn = new double[this->numOutBands];
            
            int nYBlocks = floor(((double)height) / ((double)numOfLines));
            int remainRows = height - (nYBlocks * numOfLines);
            int rowOffset = 0;
            unsigned int line = 0;
            long cLinePxl = 0;
            long cPxl = 0;
            long dLinePxls = 0;
            int dWinX = 0;
            int dWinY = 0;
            
            int feedback = height/10.0;
			int feedbackCounter = 0;
			std::cout << "Started " << std::flush;
			
            if(nYBlocks > 0)
            {
                for(int i = 0; i < nYBlocks; i++)
                {
                    if(i == 0)
                    {
                        // Set Upper Block with Zeros.
                        for(int n = 0; n < numInBands; n++)
                        {
                            for(int k = 0; k < numPxlsInBlock; k++)
                            {
                                inputDataUpper[n][k] = 0;
                            }
                        }
                        
                        // Read Main Block
                        for(int n = 0; n < numInBands; n++)
                        {
                            rowOffset = bandOffsets[n][1] + (numOfLines * i);
                            inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, numOfLines, inputDataMain[n], width, numOfLines, GDT_Float32, 0, 0);
                        }
                        // Read Lower Block
                        for(int n = 0; n < numInBands; n++)
                        {
                            if(nYBlocks == 1)
                            {
                                if(remainRows > 0)
                                {
                                    rowOffset = bandOffsets[n][1] + (numOfLines * (i+1));
                                    inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, remainRows, inputDataLower[n], width, remainRows, GDT_Float32, 0, 0);
                                    for(int k = (remainRows*width); k < numPxlsInBlock; k++)
                                    {
                                        inputDataLower[n][k] = 0;
                                    }
                                }
                                else
                                {
                                    for(int k = 0; k < numPxlsInBlock; k++)
                                    {
                                        inputDataLower[n][k] = 0;
                                    }
                                }
                            }
                            else
                            {
                                rowOffset = bandOffsets[n][1] + (numOfLines * (i+1));
                                inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, numOfLines, inputDataLower[n], width, numOfLines, GDT_Float32, 0, 0);
                            }
                        }
                    }
                    else if(i == (nYBlocks-1))
                    {
                        // Shift Lower Block to Main Block
                        for(int n = 0; n < numInBands; n++)
                        {
                            for(int k = 0; k < numPxlsInBlock; k++)
                            {
                                inputDataUpper[n][k] = inputDataMain[n][k];
                            }
                        }
                        
                        // Shift Lower Block to Main Block
                        for(int n = 0; n < numInBands; n++)
                        {
                            for(int k = 0; k < numPxlsInBlock; k++)
                            {
                                inputDataMain[n][k] = inputDataLower[n][k];
                            }
                        }
                        
                        // Set Lower Block with Zeros.
                        for(int n = 0; n < numInBands; n++)
                        {
                            if(remainRows > 0)
                            {
                                rowOffset = bandOffsets[n][1] + (numOfLines * (i+1));
                                inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, remainRows, inputDataLower[n], width, remainRows, GDT_Float32, 0, 0);
                                for(int k = (remainRows*width); k < numPxlsInBlock; k++)
                                {
                                    inputDataLower[n][k] = 0;
                                }
                            }
                            else
                            {
                                for(int k = 0; k < numPxlsInBlock; k++)
                                {
                                    inputDataLower[n][k] = 0;
                                }
                            }
                        }
                    }
                    else
                    {
                        // Shift Lower Block to Main Block
                        for(int n = 0; n < numInBands; n++)
                        {
                            for(int k = 0; k < numPxlsInBlock; k++)
                            {
                                inputDataUpper[n][k] = inputDataMain[n][k];
                            }
                        }
                        
                        // Shift Lower Block to Main Block
                        for(int n = 0; n < numInBands; n++)
                        {
                            for(int k = 0; k < numPxlsInBlock; k++)
                            {
                                inputDataMain[n][k] = inputDataLower[n][k];
                            }
                        }
                        
                        // Read Lower Block
                        for(int n = 0; n < numInBands; n++)
                        {
                            rowOffset = bandOffsets[n][1] + (numOfLines * (i+1));
                            inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, numOfLines, inputDataLower[n], width, numOfLines, GDT_Float32, 0, 0);
                        }
                    }
                    
                    for(int m = 0; m < numOfLines; ++m)
                    {
                        line = (i*numOfLines)+m;
                        if((feedback != 0) && (line % feedback) == 0)
                        {
                            std::cout << "." << feedbackCounter << "." << std::flush;
                            feedbackCounter = feedbackCounter + 10;
                        }
                        
                        cLinePxl = m*width;
                        
                        for(int j = 0; j < width; j++)
                        {
                            cPxl = cLinePxl+j;
                            if(m < windowMid)
                            {
                                for(int y = 0; y < windowSize; y++)
                                {
                                    dWinY = y-windowMid;
                                    dLinePxls = dWinY * width;
                                    
                                    if((cPxl + dLinePxls) < 0)
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataUpper[n][(numPxlsInBlock+(cPxl+dLinePxls))+dWinX];
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataMain[n][(cPxl+dLinePxls)+dWinX];
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            else if(m > ((numOfLines-1)-windowMid))
                            {
                                for(int y = 0; y < windowSize; y++)
                                {
                                    dWinY = y-windowMid;
                                    dLinePxls = dWinY * width;
                                    
                                    if((cPxl + dLinePxls) >= numPxlsInBlock)
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataLower[n][((cPxl+dLinePxls)-numPxlsInBlock)+dWinX];
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataMain[n][(cPxl+dLinePxls)+dWinX];
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            else
                            {
                                for(int y = 0; y < windowSize; y++)
                                {
                                    dWinY = y-windowMid;
                                    dLinePxls = dWinY * width;
                                    
                                    for(int x = 0; x < windowSize; x++)
                                    {
                                        dWinX = x-windowMid;
                                        
                                        if((j+dWinX) < 0)
                                        {
                                            for(int n = 0; n < numInBands; n++)
                                            {
                                                inDataBlock[n][y][x] = 0;
                                            }
                                        }
                                        else if((j+dWinX) >= width)
                                        {
                                            for(int n = 0; n < numInBands; n++)
                                            {
                                                inDataBlock[n][y][x] = 0;
                                            }
                                        }
                                        else
                                        {
                                            for(int n = 0; n < numInBands; n++)
                                            {
                                                inDataBlock[n][y][x] = inputDataMain[n][(cPxl+dLinePxls)+dWinX];
                                            }
                                        }
                                    }
                                }
                            }
                            
                            this->calc->calcImageValue(inDataBlock, numInBands, windowSize, outDataColumn);
                        }
                        
                    }
                }
                
                if(remainRows > 0)
                {
                    // Shift Lower Block to Main Block
                    for(int n = 0; n < numInBands; n++)
                    {
                        for(int k = 0; k < numPxlsInBlock; k++)
                        {
                            inputDataUpper[n][k] = inputDataMain[n][k];
                        }
                    }
                    
                    // Shift Lower Block to Main Block
                    for(int n = 0; n < numInBands; n++)
                    {
                        for(int k = 0; k < numPxlsInBlock; k++)
                        {
                            inputDataMain[n][k] = inputDataLower[n][k];
                        }
                    }
                    
                    // Read Lower Block
                    for(int n = 0; n < numInBands; n++)
                    {
                        for(int k = 0; k < numPxlsInBlock; k++)
                        {
                            inputDataLower[n][k] = 0;
                        }
                    }
                    
                    for(int m = 0; m < remainRows; ++m)
                    {
                        line = (nYBlocks*numOfLines)+m;

                        if((feedback != 0) && (line % feedback) == 0)
                        {
                            std::cout << "." << feedbackCounter << "." << std::flush;
                            feedbackCounter = feedbackCounter + 10;
                        }
                        
                        cLinePxl = m*width;
                        
                        for(int j = 0; j < width; j++)
                        {
                            cPxl = cLinePxl+j;
                            if(m < windowMid)
                            {
                                for(int y = 0; y < windowSize; y++)
                                {
                                    dWinY = y-windowMid;
                                    dLinePxls = dWinY * width;
                                    
                                    if((cPxl + dLinePxls) < 0)
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataUpper[n][(numPxlsInBlock+(cPxl+dLinePxls))+dWinX];
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataMain[n][(cPxl+dLinePxls)+dWinX];
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            else if(m > ((numOfLines-1)-windowMid))
                            {
                                for(int y = 0; y < windowSize; y++)
                                {
                                    dWinY = y-windowMid;
                                    dLinePxls = dWinY * width;
                                    
                                    if((cPxl + dLinePxls) >= numPxlsInBlock)
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataLower[n][((cPxl+dLinePxls)-numPxlsInBlock)+dWinX];
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataMain[n][(cPxl+dLinePxls)+dWinX];
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            else
                            {
                                for(int y = 0; y < windowSize; y++)
                                {
                                    dWinY = y-windowMid;
                                    dLinePxls = dWinY * width;
                                    
                                    for(int x = 0; x < windowSize; x++)
                                    {
                                        dWinX = x-windowMid;
                                        
                                        if((j+dWinX) < 0)
                                        {
                                            for(int n = 0; n < numInBands; n++)
                                            {
                                                inDataBlock[n][y][x] = 0;
                                            }
                                        }
                                        else if((j+dWinX) >= width)
                                        {
                                            for(int n = 0; n < numInBands; n++)
                                            {
                                                inDataBlock[n][y][x] = 0;
                                            }
                                        }
                                        else
                                        {
                                            for(int n = 0; n < numInBands; n++)
                                            {
                                                inDataBlock[n][y][x] = inputDataMain[n][(cPxl+dLinePxls)+dWinX];
                                            }
                                        }
                                    }
                                }
                            }
                            
                            this->calc->calcImageValue(inDataBlock, numInBands, windowSize, outDataColumn);
                        }
                    }
                }
                
            }
            else
            {
                
            }
            
            
            std::cout << " Complete.\n";
		}
		catch(RSGISImageCalcException& e)
		{
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					delete dsOffsets[i];
				}
				delete[] dsOffsets;
			}
			
			if(bandOffsets != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					delete bandOffsets[i];
				}
				delete[] bandOffsets;
			}
			
			if(inputDataUpper != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					delete[] inputDataUpper[i];
				}
				delete[] inputDataUpper;
			}
            
            if(inputDataMain != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					delete[] inputDataMain[i];
				}
				delete[] inputDataMain;
			}
            
            if(inputDataLower != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					delete[] inputDataLower[i];
				}
				delete[] inputDataLower;
			}
			
			if(inDataBlock != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					for(int j = 0; j < windowSize; j++)
					{
						delete[] inDataBlock[i][j];
					}
					delete[] inDataBlock[i];
				}
				delete[] inDataBlock;
			}
			
			if(outDataColumn != NULL)
			{
				delete[] outDataColumn;
			}
            
			throw e;
		}
		catch(RSGISImageBandException& e)
		{
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					delete dsOffsets[i];
				}
				delete[] dsOffsets;
			}
			
			if(bandOffsets != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					delete bandOffsets[i];
				}
				delete[] bandOffsets;
			}
			
			if(inputDataUpper != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					delete[] inputDataUpper[i];
				}
				delete[] inputDataUpper;
			}
            
            if(inputDataMain != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					delete[] inputDataMain[i];
				}
				delete[] inputDataMain;
			}
            
            if(inputDataLower != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					delete[] inputDataLower[i];
				}
				delete[] inputDataLower;
			}
			
			if(inDataBlock != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					for(int j = 0; j < windowSize; j++)
					{
						delete[] inDataBlock[i][j];
					}
					delete[] inDataBlock[i];
				}
				delete[] inDataBlock;
			}
			
			if(outDataColumn != NULL)
			{
				delete[] outDataColumn;
			}
			
			throw e;
		}
		
		if(gdalTranslation != NULL)
		{
			delete[] gdalTranslation;
		}
		
		if(dsOffsets != NULL)
		{
			for(int i = 0; i < numDS; i++)
			{
				delete dsOffsets[i];
			}
			delete[] dsOffsets;
		}
		
		if(bandOffsets != NULL)
		{
			for(int i = 0; i < numInBands; i++)
			{
				delete bandOffsets[i];
			}
			delete[] bandOffsets;
		}
		
		if(inputDataUpper != NULL)
        {
            for(int i = 0; i < numInBands; i++)
            {
                delete[] inputDataUpper[i];
            }
            delete[] inputDataUpper;
        }
        
        if(inputDataMain != NULL)
        {
            for(int i = 0; i < numInBands; i++)
            {
                delete[] inputDataMain[i];
            }
            delete[] inputDataMain;
        }
        
        if(inputDataLower != NULL)
        {
            for(int i = 0; i < numInBands; i++)
            {
                delete[] inputDataLower[i];
            }
            delete[] inputDataLower;
        }
		
		if(inDataBlock != NULL)
		{
			for(int i = 0; i < numInBands; i++)
			{
				for(int j = 0; j < windowSize; j++)
				{
					delete[] inDataBlock[i][j];
				}
				delete[] inDataBlock[i];
			}
			delete[] inDataBlock;
		}
		
		if(outDataColumn != NULL)
		{
			delete[] outDataColumn;
		}
	}
    
    void RSGISCalcImage::calcImageWindowData(GDALDataset **datasets, int numDS, std::string outputImage, int windowSize, std::string gdalFormat, GDALDataType gdalDataType)
	{
		GDALAllRegister();
		RSGISImageUtils imgUtils;
		double *gdalTranslation = new double[6];
		int **dsOffsets = new int*[numDS];
		for(int i = 0; i < numDS; i++)
		{
			dsOffsets[i] = new int[2];
		}
		int **bandOffsets = NULL;
		int height = 0;
		int width = 0;
		int numInBands = 0;
        int xBlockSize = 0;
        int yBlockSize = 0;
        size_t numPxlsInBlock = 0;
		
        float **inputDataUpper = NULL;
		float **inputDataMain = NULL;
        float **inputDataLower = NULL;
		double **outputData = NULL;
		float ***inDataBlock = NULL;
		double *outDataColumn = NULL;
		
		GDALDataset *outputImageDS = NULL;
		GDALRasterBand **inputRasterBands = NULL;
		GDALRasterBand **outputRasterBands = NULL;
		GDALDriver *gdalDriver = NULL;
		
		try
		{
			if(windowSize % 2 == 0)
			{
				throw RSGISImageCalcException("Window size needs to be an odd number (min = 3).");
			}
			else if(windowSize < 3)
			{
				throw RSGISImageCalcException("Window size needs to be 3 or greater and an odd number.");
			}
			int windowMid = floor(((float)windowSize)/2.0); // Starting at 0!! NOT 1 otherwise would be ceil.
            
			// Find image overlap
            imgUtils.getImageOverlap(datasets, numDS, dsOffsets, &width, &height, gdalTranslation, &xBlockSize, &yBlockSize);
			
			// Count number of image bands
			for(int i = 0; i < numDS; i++)
			{
				numInBands += datasets[i]->GetRasterCount();
			}
			
			// Create new Image
			gdalDriver = GetGDALDriverManager()->GetDriverByName(gdalFormat.c_str());
			if(gdalDriver == NULL)
			{
				throw RSGISImageBandException("Driver does not exists..");
			}
            
			outputImageDS = gdalDriver->Create(outputImage.c_str(), width, height, this->numOutBands, gdalDataType, NULL);
			
			if(outputImageDS == NULL)
			{
				throw RSGISImageBandException("Output image could not be created. Check filepath.");
			}
			outputImageDS->SetGeoTransform(gdalTranslation);
			if(useImageProj)
			{
				outputImageDS->SetProjection(datasets[0]->GetProjectionRef());
			}
			else
			{
				outputImageDS->SetProjection(proj.c_str());
			}
			
			// Get Image Input Bands
			bandOffsets = new int*[numInBands];
			inputRasterBands = new GDALRasterBand*[numInBands];
			int counter = 0;
			for(int i = 0; i < numDS; i++)
			{
				for(int j = 0; j < datasets[i]->GetRasterCount(); j++)
				{
					inputRasterBands[counter] = datasets[i]->GetRasterBand(j+1);
					bandOffsets[counter] = new int[2];
					bandOffsets[counter][0] = dsOffsets[i][0];
					bandOffsets[counter][1] = dsOffsets[i][1];
					counter++;
				}
			}
			
			//Get Image Output Bands
			outputRasterBands = new GDALRasterBand*[this->numOutBands];
			for(int i = 0; i < this->numOutBands; i++)
			{
				outputRasterBands[i] = outputImageDS->GetRasterBand(i+1);
			}
            
            int outXBlockSize = 0;
            int outYBlockSize = 0;
            outputRasterBands[0]->GetBlockSize (&outXBlockSize, &outYBlockSize);
            
            if(outYBlockSize > yBlockSize)
            {
                yBlockSize = outYBlockSize;
            }
            
            int numOfLines = yBlockSize;
            if(yBlockSize < windowSize)
            {
                numOfLines = ceil(((float)windowSize)/((float)yBlockSize))*yBlockSize;
            }
            
			// Allocate memory
            numPxlsInBlock = width*numOfLines;
			inputDataUpper = new float*[numInBands];
			for(int i = 0; i < numInBands; i++)
			{
                inputDataUpper[i] = (float *) CPLMalloc(sizeof(float)*numPxlsInBlock);
                for(int k = 0; k < numPxlsInBlock; k++)
                {
                    inputDataUpper[i][k] = 0;
                }
			}
            
            inputDataMain = new float*[numInBands];
			for(int i = 0; i < numInBands; i++)
			{
                inputDataMain[i] = (float *) CPLMalloc(sizeof(float)*numPxlsInBlock);
                for(int k = 0; k < numPxlsInBlock; k++)
                {
                    inputDataMain[i][k] = 0;
                }
			}
            
            inputDataLower = new float*[numInBands];
			for(int i = 0; i < numInBands; i++)
			{
                inputDataLower[i] = (float *) CPLMalloc(sizeof(float)*numPxlsInBlock);
                for(int k = 0; k < numPxlsInBlock; k++)
                {
                    inputDataLower[i][k] = 0;
                }
			}
			
			inDataBlock = new float**[numInBands];
			for(int i = 0; i < numInBands; i++)
			{
				inDataBlock[i] = new float*[windowSize];
				for(int j = 0; j < windowSize; j++)
				{
					inDataBlock[i][j] = new float[windowSize];
				}
			}
			
			outputData = new double*[this->numOutBands];
			for(int i = 0; i < this->numOutBands; i++)
			{
				outputData[i] = (double *) CPLMalloc(sizeof(double)*numPxlsInBlock);
			}
			outDataColumn = new double[this->numOutBands];
			
            int nYBlocks = floor(((double)height) / ((double)numOfLines));
            int remainRows = height - (nYBlocks * numOfLines);
            int rowOffset = 0;
            unsigned int line = 0;
            long cLinePxl = 0;
            long cPxl = 0;
            long dLinePxls = 0;
            int dWinX = 0;
            int dWinY = 0;
            
            int feedback = height/10.0;
			int feedbackCounter = 0;
			std::cout << "Started " << std::flush;
			
            if(nYBlocks > 0)
            {
                for(int i = 0; i < nYBlocks; i++)
                {
                    if(i == 0)
                    {
                        // Set Upper Block with Zeros.
                        for(int n = 0; n < numInBands; n++)
                        {
                            for(int k = 0; k < numPxlsInBlock; k++)
                            {
                                inputDataUpper[n][k] = 0;
                            }
                        }
                        
                        // Read Main Block
                        for(int n = 0; n < numInBands; n++)
                        {
                            rowOffset = bandOffsets[n][1] + (numOfLines * i);
                            inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, numOfLines, inputDataMain[n], width, numOfLines, GDT_Float32, 0, 0);
                        }
                        // Read Lower Block
                        for(int n = 0; n < numInBands; n++)
                        {
                            if(nYBlocks == 1)
                            {
                                if(remainRows > 0)
                                {
                                    rowOffset = bandOffsets[n][1] + (numOfLines * (i+1));
                                    inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, remainRows, inputDataLower[n], width, remainRows, GDT_Float32, 0, 0);
                                    for(int k = (remainRows*width); k < numPxlsInBlock; k++)
                                    {
                                        inputDataLower[n][k] = 0;
                                    }
                                }
                                else
                                {
                                    for(int k = 0; k < numPxlsInBlock; k++)
                                    {
                                        inputDataLower[n][k] = 0;
                                    }
                                }
                            }
                            else
                            {
                                rowOffset = bandOffsets[n][1] + (numOfLines * (i+1));
                                inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, numOfLines, inputDataLower[n], width, numOfLines, GDT_Float32, 0, 0);
                            }
                        }
                    }
                    else if(i == (nYBlocks-1))
                    {
                        // Shift Lower Block to Main Block
                        for(int n = 0; n < numInBands; n++)
                        {
                            for(int k = 0; k < numPxlsInBlock; k++)
                            {
                                inputDataUpper[n][k] = inputDataMain[n][k];
                            }
                        }
                        
                        // Shift Lower Block to Main Block
                        for(int n = 0; n < numInBands; n++)
                        {
                            for(int k = 0; k < numPxlsInBlock; k++)
                            {
                                inputDataMain[n][k] = inputDataLower[n][k];
                            }
                        }
                        
                        // Set Lower Block with Zeros.
                        for(int n = 0; n < numInBands; n++)
                        {
                            if(remainRows > 0)
                            {
                                rowOffset = bandOffsets[n][1] + (numOfLines * (i+1));
                                inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, remainRows, inputDataLower[n], width, remainRows, GDT_Float32, 0, 0);
                                for(int k = (remainRows*width); k < numPxlsInBlock; k++)
                                {
                                    inputDataLower[n][k] = 0;
                                }
                            }
                            else
                            {
                                for(int k = 0; k < numPxlsInBlock; k++)
                                {
                                    inputDataLower[n][k] = 0;
                                }
                            }
                        }
                    }
                    else
                    {
                        // Shift Lower Block to Main Block
                        for(int n = 0; n < numInBands; n++)
                        {
                            for(int k = 0; k < numPxlsInBlock; k++)
                            {
                                inputDataUpper[n][k] = inputDataMain[n][k];
                            }
                        }
                        
                        // Shift Lower Block to Main Block
                        for(int n = 0; n < numInBands; n++)
                        {
                            for(int k = 0; k < numPxlsInBlock; k++)
                            {
                                inputDataMain[n][k] = inputDataLower[n][k];
                            }
                        }
                        
                        // Read Lower Block
                        for(int n = 0; n < numInBands; n++)
                        {
                            rowOffset = bandOffsets[n][1] + (numOfLines * (i+1));
                            inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, numOfLines, inputDataLower[n], width, numOfLines, GDT_Float32, 0, 0);
                        }
                    }
                    
                    for(int m = 0; m < numOfLines; ++m)
                    {
                        line = (i*numOfLines)+m;
                        if((feedback != 0) && (line % feedback) == 0)
                        {
                            std::cout << "." << feedbackCounter << "." << std::flush;
                            feedbackCounter = feedbackCounter + 10;
                        }
                        
                        cLinePxl = m*width;
                        
                        for(int j = 0; j < width; j++)
                        {
                            cPxl = cLinePxl+j;
                            if(m < windowMid)
                            {
                                for(int y = 0; y < windowSize; y++)
                                {
                                    dWinY = y-windowMid;
                                    dLinePxls = dWinY * width;
                                    
                                    if((cPxl + dLinePxls) < 0)
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataUpper[n][(numPxlsInBlock+(cPxl+dLinePxls))+dWinX];
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataMain[n][(cPxl+dLinePxls)+dWinX];
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            else if(m > ((numOfLines-1)-windowMid))
                            {
                                for(int y = 0; y < windowSize; y++)
                                {
                                    dWinY = y-windowMid;
                                    dLinePxls = dWinY * width;
                                    
                                    if((cPxl + dLinePxls) >= numPxlsInBlock)
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataLower[n][((cPxl+dLinePxls)-numPxlsInBlock)+dWinX];
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataMain[n][(cPxl+dLinePxls)+dWinX];
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            else
                            {
                                for(int y = 0; y < windowSize; y++)
                                {
                                    dWinY = y-windowMid;
                                    dLinePxls = dWinY * width;
                                    
                                    for(int x = 0; x < windowSize; x++)
                                    {
                                        dWinX = x-windowMid;
                                        
                                        if((j+dWinX) < 0)
                                        {
                                            for(int n = 0; n < numInBands; n++)
                                            {
                                                inDataBlock[n][y][x] = 0;
                                            }
                                        }
                                        else if((j+dWinX) >= width)
                                        {
                                            for(int n = 0; n < numInBands; n++)
                                            {
                                                inDataBlock[n][y][x] = 0;
                                            }
                                        }
                                        else
                                        {
                                            for(int n = 0; n < numInBands; n++)
                                            {
                                                inDataBlock[n][y][x] = inputDataMain[n][(cPxl+dLinePxls)+dWinX];
                                            }
                                        }
                                    }
                                }
                            }
                            
                            this->calc->calcImageValue(inDataBlock, numInBands, windowSize, outDataColumn);
                            
                            for(int n = 0; n < this->numOutBands; n++)
                            {
                                outputData[n][cPxl] = outDataColumn[n];
                            }
                        }
                        
                    }
                    
                    for(int n = 0; n < this->numOutBands; n++)
                    {
                        outputRasterBands[n]->RasterIO(GF_Write, 0, (numOfLines * i), width, numOfLines, outputData[n], width, numOfLines, GDT_Float64, 0, 0);
                    }
                }
                                
                if(remainRows > 0)
                {
                    // Shift Lower Block to Main Block
                    for(int n = 0; n < numInBands; n++)
                    {
                        for(int k = 0; k < numPxlsInBlock; k++)
                        {
                            inputDataUpper[n][k] = inputDataMain[n][k];
                        }
                    }
                    
                    // Shift Lower Block to Main Block
                    for(int n = 0; n < numInBands; n++)
                    {
                        for(int k = 0; k < numPxlsInBlock; k++)
                        {
                            inputDataMain[n][k] = inputDataLower[n][k];
                        }
                    }
                    
                    // Read Lower Block
                    for(int n = 0; n < numInBands; n++)
                    {
                        for(int k = 0; k < numPxlsInBlock; k++)
                        {
                            inputDataLower[n][k] = 0;
                        }
                    }
                    
                    for(int m = 0; m < remainRows; ++m)
                    {
                        line = (nYBlocks*numOfLines)+m;
                        if((feedback != 0) && (line % feedback) == 0)
                        {
                            std::cout << "." << feedbackCounter << "." << std::flush;
                            feedbackCounter = feedbackCounter + 10;
                        }
                        
                        cLinePxl = m*width;
                        
                        for(int j = 0; j < width; j++)
                        {
                            cPxl = cLinePxl+j;
                            if(m < windowMid)
                            {
                                for(int y = 0; y < windowSize; y++)
                                {
                                    dWinY = y-windowMid;
                                    dLinePxls = dWinY * width;
                                    
                                    if((cPxl + dLinePxls) < 0)
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataUpper[n][(numPxlsInBlock+(cPxl+dLinePxls))+dWinX];
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataMain[n][(cPxl+dLinePxls)+dWinX];
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            else if(m > ((numOfLines-1)-windowMid))
                            {
                                for(int y = 0; y < windowSize; y++)
                                {
                                    dWinY = y-windowMid;
                                    dLinePxls = dWinY * width;
                                    
                                    if((cPxl + dLinePxls) >= numPxlsInBlock)
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataLower[n][((cPxl+dLinePxls)-numPxlsInBlock)+dWinX];
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataMain[n][(cPxl+dLinePxls)+dWinX];
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            else
                            {
                                for(int y = 0; y < windowSize; y++)
                                {
                                    dWinY = y-windowMid;
                                    dLinePxls = dWinY * width;
                                    
                                    for(int x = 0; x < windowSize; x++)
                                    {
                                        dWinX = x-windowMid;
                                        
                                        if((j+dWinX) < 0)
                                        {
                                            for(int n = 0; n < numInBands; n++)
                                            {
                                                inDataBlock[n][y][x] = 0;
                                            }
                                        }
                                        else if((j+dWinX) >= width)
                                        {
                                            for(int n = 0; n < numInBands; n++)
                                            {
                                                inDataBlock[n][y][x] = 0;
                                            }
                                        }
                                        else
                                        {
                                            for(int n = 0; n < numInBands; n++)
                                            {
                                                inDataBlock[n][y][x] = inputDataMain[n][(cPxl+dLinePxls)+dWinX];
                                            }
                                        }
                                    }
                                }
                            }
                            
                            this->calc->calcImageValue(inDataBlock, numInBands, windowSize, outDataColumn);
                            
                            for(int n = 0; n < this->numOutBands; n++)
                            {                                
                                outputData[n][cPxl] = outDataColumn[n];
                            }
                        }
                    }
                    
                    for(int n = 0; n < this->numOutBands; n++)
                    {
                        outputRasterBands[n]->RasterIO(GF_Write, 0, (nYBlocks*numOfLines), width, remainRows, outputData[n], width, remainRows, GDT_Float64, 0, 0);
                    }
                }
                
            }
            else
            {
                
            }
            

            std::cout << " Complete.\n";
		}
		catch(RSGISImageCalcException& e)
		{
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					delete dsOffsets[i];
				}
				delete[] dsOffsets;
			}
			
			if(bandOffsets != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					delete bandOffsets[i];
				}
				delete[] bandOffsets;
			}
			
			if(inputDataUpper != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					delete[] inputDataUpper[i];
				}
				delete[] inputDataUpper;
			}
            
            if(inputDataMain != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					delete[] inputDataMain[i];
				}
				delete[] inputDataMain;
			}
            
            if(inputDataLower != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					delete[] inputDataLower[i];
				}
				delete[] inputDataLower;
			}
			
			if(inDataBlock != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					for(int j = 0; j < windowSize; j++)
					{
						delete[] inDataBlock[i][j];
					}
					delete[] inDataBlock[i];
				}
				delete[] inDataBlock;
			}
			
			if(outputData != NULL)
			{
				for(int i = 0; i < numOutBands; i++)
				{
					delete outputData[i];
				}
				delete outputData;
			}
			
			if(outDataColumn != NULL)
			{
				delete[] outDataColumn;
			}

			throw e;
		}
		catch(RSGISImageBandException& e)
		{
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					delete dsOffsets[i];
				}
				delete[] dsOffsets;
			}
			
			if(bandOffsets != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					delete bandOffsets[i];
				}
				delete[] bandOffsets;
			}
			
			if(inputDataUpper != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					delete[] inputDataUpper[i];
				}
				delete[] inputDataUpper;
			}
            
            if(inputDataMain != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					delete[] inputDataMain[i];
				}
				delete[] inputDataMain;
			}
            
            if(inputDataLower != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					delete[] inputDataLower[i];
				}
				delete[] inputDataLower;
			}
			
			if(inDataBlock != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					for(int j = 0; j < windowSize; j++)
					{
						delete[] inDataBlock[i][j];
					}
					delete[] inDataBlock[i];
				}
				delete[] inDataBlock;
			}
			
			if(outputData != NULL)
			{
				for(int i = 0; i < numOutBands; i++)
				{
					delete outputData[i];
				}
				delete outputData;
			}
			
			if(outDataColumn != NULL)
			{
				delete[] outDataColumn;
			}
			
			throw e;
		}
		
		if(gdalTranslation != NULL)
		{
			delete[] gdalTranslation;
		}
		
		if(dsOffsets != NULL)
		{
			for(int i = 0; i < numDS; i++)
			{
				delete dsOffsets[i];
			}
			delete[] dsOffsets;
		}
		
		if(bandOffsets != NULL)
		{
			for(int i = 0; i < numInBands; i++)
			{
				delete bandOffsets[i];
			}
			delete[] bandOffsets;
		}
		
		if(inputDataUpper != NULL)
        {
            for(int i = 0; i < numInBands; i++)
            {
                delete[] inputDataUpper[i];
            }
            delete[] inputDataUpper;
        }
        
        if(inputDataMain != NULL)
        {
            for(int i = 0; i < numInBands; i++)
            {
                delete[] inputDataMain[i];
            }
            delete[] inputDataMain;
        }
        
        if(inputDataLower != NULL)
        {
            for(int i = 0; i < numInBands; i++)
            {
                delete[] inputDataLower[i];
            }
            delete[] inputDataLower;
        }
		
		if(inDataBlock != NULL)
		{
			for(int i = 0; i < numInBands; i++)
			{
				for(int j = 0; j < windowSize; j++)
				{
					delete[] inDataBlock[i][j];
				}
				delete[] inDataBlock[i];
			}
			delete[] inDataBlock;
		}
		
		if(outputData != NULL)
		{
			for(int i = 0; i < numOutBands; i++)
			{
				delete outputData[i];
			}
			delete outputData;
		}
		
		if(outDataColumn != NULL)
		{
			delete[] outDataColumn;
		}
		
		GDALClose(outputImageDS);
	}
    
    void RSGISCalcImage::calcImageWindowData(GDALDataset **datasets, int numDS, std::string outputImage, std::string outputRefIntImage, int windowSize, std::string gdalFormat, GDALDataType gdalDataType)
    {
        GDALAllRegister();
        RSGISImageUtils imgUtils;
        double *gdalTranslation = new double[6];
        int **dsOffsets = new int*[numDS];
        for(int i = 0; i < numDS; i++)
        {
            dsOffsets[i] = new int[2];
        }
        int **bandOffsets = NULL;
        int height = 0;
        int width = 0;
        int numInBands = 0;
        int xBlockSize = 0;
        int yBlockSize = 0;
        size_t numPxlsInBlock = 0;
        
        float **inputDataUpper = NULL;
        float **inputDataMain = NULL;
        float **inputDataLower = NULL;
        double **outputData = NULL;
        double *outputRefData = NULL;
        float ***inDataBlock = NULL;
        double *outDataColumn = NULL;
        double outRefData = 0;
        
        GDALDataset *outputImageDS = NULL;
        GDALDataset *outputRefImageDS = NULL;
        GDALRasterBand **inputRasterBands = NULL;
        GDALRasterBand **outputRasterBands = NULL;
        GDALRasterBand *outputRefRasterBand = NULL;
        GDALDriver *gdalDriver = NULL;
        
        try
        {
            if(windowSize % 2 == 0)
            {
                throw RSGISImageCalcException("Window size needs to be an odd number (min = 3).");
            }
            else if(windowSize < 3)
            {
                throw RSGISImageCalcException("Window size needs to be 3 or greater and an odd number.");
            }
            int windowMid = floor(((float)windowSize)/2.0); // Starting at 0!! NOT 1 otherwise would be ceil.
            
            // Find image overlap
            imgUtils.getImageOverlap(datasets, numDS, dsOffsets, &width, &height, gdalTranslation, &xBlockSize, &yBlockSize);
            
            // Count number of image bands
            for(int i = 0; i < numDS; i++)
            {
                numInBands += datasets[i]->GetRasterCount();
            }
            
            // Create new Image
            gdalDriver = GetGDALDriverManager()->GetDriverByName(gdalFormat.c_str());
            if(gdalDriver == NULL)
            {
                throw RSGISImageBandException("Driver does not exists..");
            }
            
            outputImageDS = gdalDriver->Create(outputImage.c_str(), width, height, this->numOutBands, gdalDataType, NULL);
            if(outputImageDS == NULL)
            {
                throw RSGISImageBandException("Output image could not be created. Check filepath.");
            }
            outputImageDS->SetGeoTransform(gdalTranslation);
            if(useImageProj)
            {
                outputImageDS->SetProjection(datasets[0]->GetProjectionRef());
            }
            else
            {
                outputImageDS->SetProjection(proj.c_str());
            }
            
            outputRefImageDS = gdalDriver->Create(outputRefIntImage.c_str(), width, height, 1, GDT_UInt32, NULL);
            if(outputRefImageDS == NULL)
            {
                throw RSGISImageBandException("Output reference image could not be created. Check filepath.");
            }
            outputRefImageDS->SetGeoTransform(gdalTranslation);
            if(useImageProj)
            {
                outputRefImageDS->SetProjection(datasets[0]->GetProjectionRef());
            }
            else
            {
                outputRefImageDS->SetProjection(proj.c_str());
            }
            
            // Get Image Input Bands
            bandOffsets = new int*[numInBands];
            inputRasterBands = new GDALRasterBand*[numInBands];
            int counter = 0;
            for(int i = 0; i < numDS; i++)
            {
                for(int j = 0; j < datasets[i]->GetRasterCount(); j++)
                {
                    inputRasterBands[counter] = datasets[i]->GetRasterBand(j+1);
                    bandOffsets[counter] = new int[2];
                    bandOffsets[counter][0] = dsOffsets[i][0];
                    bandOffsets[counter][1] = dsOffsets[i][1];
                    counter++;
                }
            }
            
            //Get Image Output Bands
            outputRasterBands = new GDALRasterBand*[this->numOutBands];
            for(int i = 0; i < this->numOutBands; i++)
            {
                outputRasterBands[i] = outputImageDS->GetRasterBand(i+1);
            }
            outputRefRasterBand = outputRefImageDS->GetRasterBand(1);
            
            int outXBlockSize = 0;
            int outYBlockSize = 0;
            outputRasterBands[0]->GetBlockSize (&outXBlockSize, &outYBlockSize);
            
            if(outYBlockSize > yBlockSize)
            {
                yBlockSize = outYBlockSize;
            }
            
            int numOfLines = yBlockSize;
            if(yBlockSize < windowSize)
            {
                numOfLines = ceil(((float)windowSize)/((float)yBlockSize))*yBlockSize;
            }
            
            // Allocate memory
            numPxlsInBlock = width*numOfLines;
            inputDataUpper = new float*[numInBands];
            for(int i = 0; i < numInBands; i++)
            {
                inputDataUpper[i] = (float *) CPLMalloc(sizeof(float)*numPxlsInBlock);
                for(int k = 0; k < numPxlsInBlock; k++)
                {
                    inputDataUpper[i][k] = 0;
                }
            }
            
            inputDataMain = new float*[numInBands];
            for(int i = 0; i < numInBands; i++)
            {
                inputDataMain[i] = (float *) CPLMalloc(sizeof(float)*numPxlsInBlock);
                for(int k = 0; k < numPxlsInBlock; k++)
                {
                    inputDataMain[i][k] = 0;
                }
            }
            
            inputDataLower = new float*[numInBands];
            for(int i = 0; i < numInBands; i++)
            {
                inputDataLower[i] = (float *) CPLMalloc(sizeof(float)*numPxlsInBlock);
                for(int k = 0; k < numPxlsInBlock; k++)
                {
                    inputDataLower[i][k] = 0;
                }
            }
            
            inDataBlock = new float**[numInBands];
            for(int i = 0; i < numInBands; i++)
            {
                inDataBlock[i] = new float*[windowSize];
                for(int j = 0; j < windowSize; j++)
                {
                    inDataBlock[i][j] = new float[windowSize];
                }
            }
            
            outputData = new double*[this->numOutBands];
            for(int i = 0; i < this->numOutBands; i++)
            {
                outputData[i] = (double *) CPLMalloc(sizeof(double)*numPxlsInBlock);
            }
            outDataColumn = new double[this->numOutBands];
            outputRefData = (double *) CPLMalloc(sizeof(double)*(width*yBlockSize));
            
            int nYBlocks = floor(((double)height) / ((double)numOfLines));
            int remainRows = height - (nYBlocks * numOfLines);
            int rowOffset = 0;
            unsigned int line = 0;
            long cLinePxl = 0;
            long cPxl = 0;
            long dLinePxls = 0;
            int dWinX = 0;
            int dWinY = 0;
            
            int feedback = height/10.0;
            int feedbackCounter = 0;
            std::cout << "Started " << std::flush;
            
            if(nYBlocks > 0)
            {
                for(int i = 0; i < nYBlocks; i++)
                {
                    if(i == 0)
                    {
                        // Set Upper Block with Zeros.
                        for(int n = 0; n < numInBands; n++)
                        {
                            for(int k = 0; k < numPxlsInBlock; k++)
                            {
                                inputDataUpper[n][k] = 0;
                            }
                        }
                        
                        // Read Main Block
                        for(int n = 0; n < numInBands; n++)
                        {
                            rowOffset = bandOffsets[n][1] + (numOfLines * i);
                            inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, numOfLines, inputDataMain[n], width, numOfLines, GDT_Float32, 0, 0);
                        }
                        // Read Lower Block
                        for(int n = 0; n < numInBands; n++)
                        {
                            if(nYBlocks == 1)
                            {
                                if(remainRows > 0)
                                {
                                    rowOffset = bandOffsets[n][1] + (numOfLines * (i+1));
                                    inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, remainRows, inputDataLower[n], width, remainRows, GDT_Float32, 0, 0);
                                    for(int k = (remainRows*width); k < numPxlsInBlock; k++)
                                    {
                                        inputDataLower[n][k] = 0;
                                    }
                                }
                                else
                                {
                                    for(int k = 0; k < numPxlsInBlock; k++)
                                    {
                                        inputDataLower[n][k] = 0;
                                    }
                                }
                            }
                            else
                            {
                                rowOffset = bandOffsets[n][1] + (numOfLines * (i+1));
                                inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, numOfLines, inputDataLower[n], width, numOfLines, GDT_Float32, 0, 0);
                            }
                        }
                    }
                    else if(i == (nYBlocks-1))
                    {
                        // Shift Lower Block to Main Block
                        for(int n = 0; n < numInBands; n++)
                        {
                            for(int k = 0; k < numPxlsInBlock; k++)
                            {
                                inputDataUpper[n][k] = inputDataMain[n][k];
                            }
                        }
                        
                        // Shift Lower Block to Main Block
                        for(int n = 0; n < numInBands; n++)
                        {
                            for(int k = 0; k < numPxlsInBlock; k++)
                            {
                                inputDataMain[n][k] = inputDataLower[n][k];
                            }
                        }
                        
                        // Set Lower Block with Zeros.
                        for(int n = 0; n < numInBands; n++)
                        {
                            if(remainRows > 0)
                            {
                                rowOffset = bandOffsets[n][1] + (numOfLines * (i+1));
                                inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, remainRows, inputDataLower[n], width, remainRows, GDT_Float32, 0, 0);
                                for(int k = (remainRows*width); k < numPxlsInBlock; k++)
                                {
                                    inputDataLower[n][k] = 0;
                                }
                            }
                            else
                            {
                                for(int k = 0; k < numPxlsInBlock; k++)
                                {
                                    inputDataLower[n][k] = 0;
                                }
                            }
                        }
                    }
                    else
                    {
                        // Shift Lower Block to Main Block
                        for(int n = 0; n < numInBands; n++)
                        {
                            for(int k = 0; k < numPxlsInBlock; k++)
                            {
                                inputDataUpper[n][k] = inputDataMain[n][k];
                            }
                        }
                        
                        // Shift Lower Block to Main Block
                        for(int n = 0; n < numInBands; n++)
                        {
                            for(int k = 0; k < numPxlsInBlock; k++)
                            {
                                inputDataMain[n][k] = inputDataLower[n][k];
                            }
                        }
                        
                        // Read Lower Block
                        for(int n = 0; n < numInBands; n++)
                        {
                            rowOffset = bandOffsets[n][1] + (numOfLines * (i+1));
                            inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, numOfLines, inputDataLower[n], width, numOfLines, GDT_Float32, 0, 0);
                        }
                    }
                    
                    for(int m = 0; m < numOfLines; ++m)
                    {
                        line = (i*numOfLines)+m;
                        if((feedback != 0) && (line % feedback) == 0)
                        {
                            std::cout << "." << feedbackCounter << "." << std::flush;
                            feedbackCounter = feedbackCounter + 10;
                        }
                        
                        cLinePxl = m*width;
                        
                        for(int j = 0; j < width; j++)
                        {
                            cPxl = cLinePxl+j;
                            if(m < windowMid)
                            {
                                for(int y = 0; y < windowSize; y++)
                                {
                                    dWinY = y-windowMid;
                                    dLinePxls = dWinY * width;
                                    
                                    if((cPxl + dLinePxls) < 0)
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataUpper[n][(numPxlsInBlock+(cPxl+dLinePxls))+dWinX];
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataMain[n][(cPxl+dLinePxls)+dWinX];
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            else if(m > ((numOfLines-1)-windowMid))
                            {
                                for(int y = 0; y < windowSize; y++)
                                {
                                    dWinY = y-windowMid;
                                    dLinePxls = dWinY * width;
                                    
                                    if((cPxl + dLinePxls) >= numPxlsInBlock)
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataLower[n][((cPxl+dLinePxls)-numPxlsInBlock)+dWinX];
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataMain[n][(cPxl+dLinePxls)+dWinX];
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            else
                            {
                                for(int y = 0; y < windowSize; y++)
                                {
                                    dWinY = y-windowMid;
                                    dLinePxls = dWinY * width;
                                    
                                    for(int x = 0; x < windowSize; x++)
                                    {
                                        dWinX = x-windowMid;
                                        
                                        if((j+dWinX) < 0)
                                        {
                                            for(int n = 0; n < numInBands; n++)
                                            {
                                                inDataBlock[n][y][x] = 0;
                                            }
                                        }
                                        else if((j+dWinX) >= width)
                                        {
                                            for(int n = 0; n < numInBands; n++)
                                            {
                                                inDataBlock[n][y][x] = 0;
                                            }
                                        }
                                        else
                                        {
                                            for(int n = 0; n < numInBands; n++)
                                            {
                                                inDataBlock[n][y][x] = inputDataMain[n][(cPxl+dLinePxls)+dWinX];
                                            }
                                        }
                                    }
                                }
                            }
                            
                            this->calc->calcImageValue(inDataBlock, numInBands, windowSize, outDataColumn, &outRefData, 1);
                            
                            for(int n = 0; n < this->numOutBands; n++)
                            {
                                outputData[n][cPxl] = outDataColumn[n];
                            }
                            outputRefData[cPxl] = outRefData;
                        }
                        
                    }
                    
                    for(int n = 0; n < this->numOutBands; n++)
                    {
                        outputRasterBands[n]->RasterIO(GF_Write, 0, (numOfLines * i), width, numOfLines, outputData[n], width, numOfLines, GDT_Float64, 0, 0);
                    }
                    outputRefRasterBand->RasterIO(GF_Write, 0, (numOfLines * i), width, numOfLines, outputRefData, width, numOfLines, GDT_Float64, 0, 0);
                }
                
                if(remainRows > 0)
                {
                    // Shift Lower Block to Main Block
                    for(int n = 0; n < numInBands; n++)
                    {
                        for(int k = 0; k < numPxlsInBlock; k++)
                        {
                            inputDataUpper[n][k] = inputDataMain[n][k];
                        }
                    }
                    
                    // Shift Lower Block to Main Block
                    for(int n = 0; n < numInBands; n++)
                    {
                        for(int k = 0; k < numPxlsInBlock; k++)
                        {
                            inputDataMain[n][k] = inputDataLower[n][k];
                        }
                    }
                    
                    // Read Lower Block
                    for(int n = 0; n < numInBands; n++)
                    {
                        for(int k = 0; k < numPxlsInBlock; k++)
                        {
                            inputDataLower[n][k] = 0;
                        }
                    }
                    
                    for(int m = 0; m < remainRows; ++m)
                    {
                        line = (nYBlocks*numOfLines)+m;
                        if((feedback != 0) && (line % feedback) == 0)
                        {
                            std::cout << "." << feedbackCounter << "." << std::flush;
                            feedbackCounter = feedbackCounter + 10;
                        }
                        
                        cLinePxl = m*width;
                        
                        for(int j = 0; j < width; j++)
                        {
                            cPxl = cLinePxl+j;
                            if(m < windowMid)
                            {
                                for(int y = 0; y < windowSize; y++)
                                {
                                    dWinY = y-windowMid;
                                    dLinePxls = dWinY * width;
                                    
                                    if((cPxl + dLinePxls) < 0)
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataUpper[n][(numPxlsInBlock+(cPxl+dLinePxls))+dWinX];
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataMain[n][(cPxl+dLinePxls)+dWinX];
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            else if(m > ((numOfLines-1)-windowMid))
                            {
                                for(int y = 0; y < windowSize; y++)
                                {
                                    dWinY = y-windowMid;
                                    dLinePxls = dWinY * width;
                                    
                                    if((cPxl + dLinePxls) >= numPxlsInBlock)
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataLower[n][((cPxl+dLinePxls)-numPxlsInBlock)+dWinX];
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataMain[n][(cPxl+dLinePxls)+dWinX];
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            else
                            {
                                for(int y = 0; y < windowSize; y++)
                                {
                                    dWinY = y-windowMid;
                                    dLinePxls = dWinY * width;
                                    
                                    for(int x = 0; x < windowSize; x++)
                                    {
                                        dWinX = x-windowMid;
                                        
                                        if((j+dWinX) < 0)
                                        {
                                            for(int n = 0; n < numInBands; n++)
                                            {
                                                inDataBlock[n][y][x] = 0;
                                            }
                                        }
                                        else if((j+dWinX) >= width)
                                        {
                                            for(int n = 0; n < numInBands; n++)
                                            {
                                                inDataBlock[n][y][x] = 0;
                                            }
                                        }
                                        else
                                        {
                                            for(int n = 0; n < numInBands; n++)
                                            {
                                                inDataBlock[n][y][x] = inputDataMain[n][(cPxl+dLinePxls)+dWinX];
                                            }
                                        }
                                    }
                                }
                            }
                            
                            this->calc->calcImageValue(inDataBlock, numInBands, windowSize, outDataColumn, &outRefData, 1);
                            
                            for(int n = 0; n < this->numOutBands; n++)
                            {
                                outputData[n][cPxl] = outDataColumn[n];
                            }
                            outputRefData[cPxl] = outRefData;
                        }
                    }
                    
                    for(int n = 0; n < this->numOutBands; n++)
                    {
                        outputRasterBands[n]->RasterIO(GF_Write, 0, (nYBlocks*numOfLines), width, remainRows, outputData[n], width, remainRows, GDT_Float64, 0, 0);
                    }
                    outputRefRasterBand->RasterIO(GF_Write, 0, (nYBlocks*numOfLines), width, remainRows, outputRefData, width, remainRows, GDT_Float64, 0, 0);
                }
            }
            else
            {
                
            }
            
            
            std::cout << " Complete.\n";
        }
        catch(RSGISImageCalcException& e)
        {
            if(gdalTranslation != NULL)
            {
                delete[] gdalTranslation;
            }
            
            if(dsOffsets != NULL)
            {
                for(int i = 0; i < numDS; i++)
                {
                    delete dsOffsets[i];
                }
                delete[] dsOffsets;
            }
            
            if(bandOffsets != NULL)
            {
                for(int i = 0; i < numInBands; i++)
                {
                    delete bandOffsets[i];
                }
                delete[] bandOffsets;
            }
            
            if(inputDataUpper != NULL)
            {
                for(int i = 0; i < numInBands; i++)
                {
                    delete[] inputDataUpper[i];
                }
                delete[] inputDataUpper;
            }
            
            if(inputDataMain != NULL)
            {
                for(int i = 0; i < numInBands; i++)
                {
                    delete[] inputDataMain[i];
                }
                delete[] inputDataMain;
            }
            
            if(inputDataLower != NULL)
            {
                for(int i = 0; i < numInBands; i++)
                {
                    delete[] inputDataLower[i];
                }
                delete[] inputDataLower;
            }
            
            if(inDataBlock != NULL)
            {
                for(int i = 0; i < numInBands; i++)
                {
                    for(int j = 0; j < windowSize; j++)
                    {
                        delete[] inDataBlock[i][j];
                    }
                    delete[] inDataBlock[i];
                }
                delete[] inDataBlock;
            }
            
            if(outputData != NULL)
            {
                for(int i = 0; i < numOutBands; i++)
                {
                    delete outputData[i];
                }
                delete outputData;
            }
            
            if(outDataColumn != NULL)
            {
                delete[] outDataColumn;
            }
            
            if(outputRefData != NULL)
            {
                delete[] outputRefData;
            }
            
            throw e;
        }
        catch(RSGISImageBandException& e)
        {
            if(gdalTranslation != NULL)
            {
                delete[] gdalTranslation;
            }
            
            if(dsOffsets != NULL)
            {
                for(int i = 0; i < numDS; i++)
                {
                    delete dsOffsets[i];
                }
                delete[] dsOffsets;
            }
            
            if(bandOffsets != NULL)
            {
                for(int i = 0; i < numInBands; i++)
                {
                    delete bandOffsets[i];
                }
                delete[] bandOffsets;
            }
            
            if(inputDataUpper != NULL)
            {
                for(int i = 0; i < numInBands; i++)
                {
                    delete[] inputDataUpper[i];
                }
                delete[] inputDataUpper;
            }
            
            if(inputDataMain != NULL)
            {
                for(int i = 0; i < numInBands; i++)
                {
                    delete[] inputDataMain[i];
                }
                delete[] inputDataMain;
            }
            
            if(inputDataLower != NULL)
            {
                for(int i = 0; i < numInBands; i++)
                {
                    delete[] inputDataLower[i];
                }
                delete[] inputDataLower;
            }
            
            if(inDataBlock != NULL)
            {
                for(int i = 0; i < numInBands; i++)
                {
                    for(int j = 0; j < windowSize; j++)
                    {
                        delete[] inDataBlock[i][j];
                    }
                    delete[] inDataBlock[i];
                }
                delete[] inDataBlock;
            }
            
            if(outputData != NULL)
            {
                for(int i = 0; i < numOutBands; i++)
                {
                    delete outputData[i];
                }
                delete outputData;
            }
            
            if(outputRefData != NULL)
            {
                delete[] outputRefData;
            }
            
            if(outDataColumn != NULL)
            {
                delete[] outDataColumn;
            }
            
            throw e;
        }
        
        if(gdalTranslation != NULL)
        {
            delete[] gdalTranslation;
        }
        
        if(dsOffsets != NULL)
        {
            for(int i = 0; i < numDS; i++)
            {
                delete dsOffsets[i];
            }
            delete[] dsOffsets;
        }
        
        if(bandOffsets != NULL)
        {
            for(int i = 0; i < numInBands; i++)
            {
                delete bandOffsets[i];
            }
            delete[] bandOffsets;
        }
        
        if(inputDataUpper != NULL)
        {
            for(int i = 0; i < numInBands; i++)
            {
                delete[] inputDataUpper[i];
            }
            delete[] inputDataUpper;
        }
        
        if(inputDataMain != NULL)
        {
            for(int i = 0; i < numInBands; i++)
            {
                delete[] inputDataMain[i];
            }
            delete[] inputDataMain;
        }
        
        if(inputDataLower != NULL)
        {
            for(int i = 0; i < numInBands; i++)
            {
                delete[] inputDataLower[i];
            }
            delete[] inputDataLower;
        }
        
        if(inDataBlock != NULL)
        {
            for(int i = 0; i < numInBands; i++)
            {
                for(int j = 0; j < windowSize; j++)
                {
                    delete[] inDataBlock[i][j];
                }
                delete[] inDataBlock[i];
            }
            delete[] inDataBlock;
        }
        
        if(outputData != NULL)
        {
            for(int i = 0; i < numOutBands; i++)
            {
                delete outputData[i];
            }
            delete outputData;
        }
        
        if(outputRefData != NULL)
        {
            delete[] outputRefData;
        }
        
        if(outDataColumn != NULL)
        {
            delete[] outDataColumn;
        }
        
        GDALClose(outputImageDS);
        GDALClose(outputRefImageDS);
    }
    
    
    void RSGISCalcImage::calcImageWindowData(GDALDataset **datasets, int numDS, GDALDataset *outputImageDS, int windowSize, bool passPxlXY)
	{
		if(outputImageDS == NULL)
        {
            throw RSGISImageBandException("Output image is not valid.");
        }
        
		RSGISImageUtils imgUtils;
		double *gdalTranslation = new double[6];
		int **dsOffsets = new int*[numDS];
		for(int i = 0; i < numDS; i++)
		{
			dsOffsets[i] = new int[2];
		}
		int **bandOffsets = NULL;
		int height = 0;
		int width = 0;
		int numInBands = 0;
        int xBlockSize = 0;
        int yBlockSize = 0;
        size_t numPxlsInBlock = 0;
		
        float **inputDataUpper = NULL;
		float **inputDataMain = NULL;
        float **inputDataLower = NULL;
		double **outputData = NULL;
		float ***inDataBlock = NULL;
		double *outDataColumn = NULL;
		
		GDALRasterBand **inputRasterBands = NULL;
		GDALRasterBand **outputRasterBands = NULL;
		
		try
		{
			if(windowSize % 2 == 0)
			{
				throw RSGISImageCalcException("Window size needs to be an odd number (min = 3).");
			}
			else if(windowSize < 3)
			{
				throw RSGISImageCalcException("Window size needs to be 3 or greater and an odd number.");
			}
			int windowMid = floor(((float)windowSize)/2.0); // Starting at 0!! NOT 1 otherwise would be ceil.
            
			// Find image overlap
            imgUtils.getImageOverlap(datasets, numDS, dsOffsets, &width, &height, gdalTranslation, &xBlockSize, &yBlockSize);
			
			// Count number of image bands
			for(int i = 0; i < numDS; i++)
			{
				numInBands += datasets[i]->GetRasterCount();
			}
			
			if(outputImageDS->GetRasterXSize() != width)
            {
                throw RSGISImageCalcException("Inputted dataset is not of the required width.");
            }
            
            if(outputImageDS->GetRasterYSize() != height)
            {
                throw RSGISImageCalcException("Inputted dataset is not of the required height.");
            }
			
            if(outputImageDS->GetRasterCount() != numOutBands)
            {
                throw RSGISImageCalcException("Inputted dataset does not have the required number of image bands.");
            }
            
			// Get Image Input Bands
			bandOffsets = new int*[numInBands];
			inputRasterBands = new GDALRasterBand*[numInBands];
			int counter = 0;
			for(int i = 0; i < numDS; i++)
			{
				for(int j = 0; j < datasets[i]->GetRasterCount(); j++)
				{
					inputRasterBands[counter] = datasets[i]->GetRasterBand(j+1);
					bandOffsets[counter] = new int[2];
					bandOffsets[counter][0] = dsOffsets[i][0];
					bandOffsets[counter][1] = dsOffsets[i][1];
					counter++;
				}
			}
			
			//Get Image Output Bands
			outputRasterBands = new GDALRasterBand*[this->numOutBands];
			for(int i = 0; i < this->numOutBands; i++)
			{
				outputRasterBands[i] = outputImageDS->GetRasterBand(i+1);
			}
            
            int outXBlockSize = 0;
            int outYBlockSize = 0;
            outputRasterBands[0]->GetBlockSize (&outXBlockSize, &outYBlockSize);
            
            if(outYBlockSize > yBlockSize)
            {
                yBlockSize = outYBlockSize;
            }
            
            int numOfLines = yBlockSize;
            if(yBlockSize < windowSize)
            {
                numOfLines = ceil(((float)windowSize)/((float)yBlockSize))*yBlockSize;
            }
            
			// Allocate memory
            numPxlsInBlock = width*numOfLines;
			inputDataUpper = new float*[numInBands];
			for(int i = 0; i < numInBands; i++)
			{
                inputDataUpper[i] = (float *) CPLMalloc(sizeof(float)*numPxlsInBlock);
                for(int k = 0; k < numPxlsInBlock; k++)
                {
                    inputDataUpper[i][k] = 0;
                }
			}
            
            inputDataMain = new float*[numInBands];
			for(int i = 0; i < numInBands; i++)
			{
                inputDataMain[i] = (float *) CPLMalloc(sizeof(float)*numPxlsInBlock);
                for(int k = 0; k < numPxlsInBlock; k++)
                {
                    inputDataMain[i][k] = 0;
                }
			}
            
            inputDataLower = new float*[numInBands];
			for(int i = 0; i < numInBands; i++)
			{
                inputDataLower[i] = (float *) CPLMalloc(sizeof(float)*numPxlsInBlock);
                for(int k = 0; k < numPxlsInBlock; k++)
                {
                    inputDataLower[i][k] = 0;
                }
			}
			
			inDataBlock = new float**[numInBands];
			for(int i = 0; i < numInBands; i++)
			{
				inDataBlock[i] = new float*[windowSize];
				for(int j = 0; j < windowSize; j++)
				{
					inDataBlock[i][j] = new float[windowSize];
				}
			}
			
			outputData = new double*[this->numOutBands];
			for(int i = 0; i < this->numOutBands; i++)
			{
				outputData[i] = (double *) CPLMalloc(sizeof(double)*numPxlsInBlock);
			}
			outDataColumn = new double[this->numOutBands];
			
            
            int nYBlocks = floor(((double)height) / ((double)numOfLines));
            int remainRows = height - (nYBlocks * numOfLines);
            int rowOffset = 0;
            unsigned int line = 0;
            long cLinePxl = 0;
            long cPxl = 0;
            long dLinePxls = 0;
            int dWinX = 0;
            int dWinY = 0;
            long xPxl = 0;
            long yPxl = 0;
            geos::geom::Envelope pxlPos;
            
            int feedback = height/10.0;
			int feedbackCounter = 0;
			std::cout << "Started " << std::flush;
			
            if(nYBlocks > 0)
            {
                for(int i = 0; i < nYBlocks; i++)
                {
                    if(i == 0)
                    {
                        // Set Upper Block with Zeros.
                        for(int n = 0; n < numInBands; n++)
                        {
                            for(int k = 0; k < numPxlsInBlock; k++)
                            {
                                inputDataUpper[n][k] = 0;
                            }
                        }
                        
                        // Read Main Block
                        for(int n = 0; n < numInBands; n++)
                        {
                            rowOffset = bandOffsets[n][1] + (numOfLines * i);
                            inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, numOfLines, inputDataMain[n], width, numOfLines, GDT_Float32, 0, 0);
                        }
                        // Read Lower Block
                        for(int n = 0; n < numInBands; n++)
                        {
                            if(nYBlocks == 1)
                            {
                                if(remainRows > 0)
                                {
                                    rowOffset = bandOffsets[n][1] + (numOfLines * (i+1));
                                    inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, remainRows, inputDataLower[n], width, remainRows, GDT_Float32, 0, 0);
                                    for(int k = (remainRows*width); k < numPxlsInBlock; k++)
                                    {
                                        inputDataLower[n][k] = 0;
                                    }
                                }
                                else
                                {
                                    for(int k = 0; k < numPxlsInBlock; k++)
                                    {
                                        inputDataLower[n][k] = 0;
                                    }
                                }
                            }
                            else
                            {
                                rowOffset = bandOffsets[n][1] + (numOfLines * (i+1));
                                inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, numOfLines, inputDataLower[n], width, numOfLines, GDT_Float32, 0, 0);
                            }
                        }
                    }
                    else if(i == (nYBlocks-1))
                    {
                        // Shift Lower Block to Main Block
                        for(int n = 0; n < numInBands; n++)
                        {
                            for(int k = 0; k < numPxlsInBlock; k++)
                            {
                                inputDataUpper[n][k] = inputDataMain[n][k];
                            }
                        }
                        
                        // Shift Lower Block to Main Block
                        for(int n = 0; n < numInBands; n++)
                        {
                            for(int k = 0; k < numPxlsInBlock; k++)
                            {
                                inputDataMain[n][k] = inputDataLower[n][k];
                            }
                        }
                        
                        // Set Lower Block with Zeros.
                        for(int n = 0; n < numInBands; n++)
                        {
                            if(remainRows > 0)
                            {
                                rowOffset = bandOffsets[n][1] + (numOfLines * (i+1));
                                inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, remainRows, inputDataLower[n], width, remainRows, GDT_Float32, 0, 0);
                                for(int k = (remainRows*width); k < numPxlsInBlock; k++)
                                {
                                    inputDataLower[n][k] = 0;
                                }
                            }
                            else
                            {
                                for(int k = 0; k < numPxlsInBlock; k++)
                                {
                                    inputDataLower[n][k] = 0;
                                }
                            }
                        }
                    }
                    else
                    {
                        // Shift Lower Block to Main Block
                        for(int n = 0; n < numInBands; n++)
                        {
                            for(int k = 0; k < numPxlsInBlock; k++)
                            {
                                inputDataUpper[n][k] = inputDataMain[n][k];
                            }
                        }
                        
                        // Shift Lower Block to Main Block
                        for(int n = 0; n < numInBands; n++)
                        {
                            for(int k = 0; k < numPxlsInBlock; k++)
                            {
                                inputDataMain[n][k] = inputDataLower[n][k];
                            }
                        }
                        
                        // Read Lower Block
                        for(int n = 0; n < numInBands; n++)
                        {
                            rowOffset = bandOffsets[n][1] + (numOfLines * (i+1));
                            inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, numOfLines, inputDataLower[n], width, numOfLines, GDT_Float32, 0, 0);
                        }
                    }
                    
                    for(int m = 0; m < numOfLines; ++m)
                    {
                        line = (i*numOfLines)+m;
                        if((feedback != 0) && (line % feedback) == 0)
                        {
                            std::cout << "." << feedbackCounter << "." << std::flush;
                            feedbackCounter = feedbackCounter + 10;
                        }
                        
                        cLinePxl = m*width;
                        
                        xPxl = 0;
                        for(int j = 0; j < width; j++)
                        {
                            cPxl = cLinePxl+j;
                            if(m < windowMid)
                            {
                                for(int y = 0; y < windowSize; y++)
                                {
                                    dWinY = y-windowMid;
                                    dLinePxls = dWinY * width;
                                    
                                    if((cPxl + dLinePxls) < 0)
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataUpper[n][(numPxlsInBlock+(cPxl+dLinePxls))+dWinX];
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataMain[n][(cPxl+dLinePxls)+dWinX];
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            else if(m > ((numOfLines-1)-windowMid))
                            {
                                for(int y = 0; y < windowSize; y++)
                                {
                                    dWinY = y-windowMid;
                                    dLinePxls = dWinY * width;
                                    
                                    if((cPxl + dLinePxls) >= numPxlsInBlock)
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataLower[n][((cPxl+dLinePxls)-numPxlsInBlock)+dWinX];
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataMain[n][(cPxl+dLinePxls)+dWinX];
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            else
                            {
                                for(int y = 0; y < windowSize; y++)
                                {
                                    dWinY = y-windowMid;
                                    dLinePxls = dWinY * width;
                                    
                                    for(int x = 0; x < windowSize; x++)
                                    {
                                        dWinX = x-windowMid;
                                        
                                        if((j+dWinX) < 0)
                                        {
                                            for(int n = 0; n < numInBands; n++)
                                            {
                                                inDataBlock[n][y][x] = 0;
                                            }
                                        }
                                        else if((j+dWinX) >= width)
                                        {
                                            for(int n = 0; n < numInBands; n++)
                                            {
                                                inDataBlock[n][y][x] = 0;
                                            }
                                        }
                                        else
                                        {
                                            for(int n = 0; n < numInBands; n++)
                                            {
                                                inDataBlock[n][y][x] = inputDataMain[n][(cPxl+dLinePxls)+dWinX];
                                            }
                                        }
                                    }
                                }
                            }
                            
                            if(passPxlXY)
                            {
                                pxlPos.init(xPxl, xPxl, yPxl, yPxl);
                                this->calc->calcImageValue(inDataBlock, numInBands, windowSize, outDataColumn, pxlPos);
                            }
                            else
                            {
                                this->calc->calcImageValue(inDataBlock, numInBands, windowSize, outDataColumn);
                            }
                            
                            for(int n = 0; n < this->numOutBands; n++)
                            {
                                outputData[n][cPxl] = outDataColumn[n];
                            }
                            ++xPxl;
                        }
                        ++yPxl;
                    }
                    
                    for(int n = 0; n < this->numOutBands; n++)
                    {
                        outputRasterBands[n]->RasterIO(GF_Write, 0, (numOfLines * i), width, numOfLines, outputData[n], width, numOfLines, GDT_Float64, 0, 0);
                    }
                }
                
                if(remainRows > 0)
                {
                    // Shift Lower Block to Main Block
                    for(int n = 0; n < numInBands; n++)
                    {
                        for(int k = 0; k < numPxlsInBlock; k++)
                        {
                            inputDataUpper[n][k] = inputDataMain[n][k];
                        }
                    }
                    
                    // Shift Lower Block to Main Block
                    for(int n = 0; n < numInBands; n++)
                    {
                        for(int k = 0; k < numPxlsInBlock; k++)
                        {
                            inputDataMain[n][k] = inputDataLower[n][k];
                        }
                    }
                    
                    // Read Lower Block
                    for(int n = 0; n < numInBands; n++)
                    {
                        for(int k = 0; k < numPxlsInBlock; k++)
                        {
                            inputDataLower[n][k] = 0;
                        }
                    }
                    
                    for(int m = 0; m < remainRows; ++m)
                    {
                        line = (nYBlocks*numOfLines)+m;

                        if((feedback != 0) && (line % feedback) == 0)
                        {
                            std::cout << "." << feedbackCounter << "." << std::flush;
                            feedbackCounter = feedbackCounter + 10;
                        }
                        
                        cLinePxl = m*width;
                        
                        xPxl = 0;
                        for(int j = 0; j < width; j++)
                        {
                            cPxl = cLinePxl+j;
                            if(m < windowMid)
                            {
                                for(int y = 0; y < windowSize; y++)
                                {
                                    dWinY = y-windowMid;
                                    dLinePxls = dWinY * width;
                                    
                                    if((cPxl + dLinePxls) < 0)
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataUpper[n][(numPxlsInBlock+(cPxl+dLinePxls))+dWinX];
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataMain[n][(cPxl+dLinePxls)+dWinX];
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            else if(m > ((numOfLines-1)-windowMid))
                            {
                                for(int y = 0; y < windowSize; y++)
                                {
                                    dWinY = y-windowMid;
                                    dLinePxls = dWinY * width;
                                    
                                    if((cPxl + dLinePxls) >= numPxlsInBlock)
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataLower[n][((cPxl+dLinePxls)-numPxlsInBlock)+dWinX];
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataMain[n][(cPxl+dLinePxls)+dWinX];
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            else
                            {
                                for(int y = 0; y < windowSize; y++)
                                {
                                    dWinY = y-windowMid;
                                    dLinePxls = dWinY * width;
                                    
                                    for(int x = 0; x < windowSize; x++)
                                    {
                                        dWinX = x-windowMid;
                                        
                                        if((j+dWinX) < 0)
                                        {
                                            for(int n = 0; n < numInBands; n++)
                                            {
                                                inDataBlock[n][y][x] = 0;
                                            }
                                        }
                                        else if((j+dWinX) >= width)
                                        {
                                            for(int n = 0; n < numInBands; n++)
                                            {
                                                inDataBlock[n][y][x] = 0;
                                            }
                                        }
                                        else
                                        {
                                            for(int n = 0; n < numInBands; n++)
                                            {
                                                inDataBlock[n][y][x] = inputDataMain[n][(cPxl+dLinePxls)+dWinX];
                                            }
                                        }
                                    }
                                }
                            }
                            
                            if(passPxlXY)
                            {
                                pxlPos.init(xPxl, xPxl, yPxl, yPxl);
                                this->calc->calcImageValue(inDataBlock, numInBands, windowSize, outDataColumn, pxlPos);
                            }
                            else
                            {
                                this->calc->calcImageValue(inDataBlock, numInBands, windowSize, outDataColumn);
                            }
                            
                            for(int n = 0; n < this->numOutBands; n++)
                            {
                                outputData[n][cPxl] = outDataColumn[n];
                            }
                            ++xPxl;
                        }
                        ++yPxl;
                    }
                    
                    for(int n = 0; n < this->numOutBands; n++)
                    {
                        outputRasterBands[n]->RasterIO(GF_Write, 0, (nYBlocks*numOfLines), width, remainRows, outputData[n], width, remainRows, GDT_Float64, 0, 0);
                    }
                }
                
            }
            else
            {
                
            }
            std::cout << " Complete.\n";
		}
		catch(RSGISImageCalcException& e)
		{
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					delete dsOffsets[i];
				}
				delete[] dsOffsets;
			}
			
			if(bandOffsets != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					delete bandOffsets[i];
				}
				delete[] bandOffsets;
			}
			
			if(inputDataUpper != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					delete[] inputDataUpper[i];
				}
				delete[] inputDataUpper;
			}
            
            if(inputDataMain != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					delete[] inputDataMain[i];
				}
				delete[] inputDataMain;
			}
            
            if(inputDataLower != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					delete[] inputDataLower[i];
				}
				delete[] inputDataLower;
			}
			
			if(inDataBlock != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					for(int j = 0; j < windowSize; j++)
					{
						delete[] inDataBlock[i][j];
					}
					delete[] inDataBlock[i];
				}
				delete[] inDataBlock;
			}
			
			if(outputData != NULL)
			{
				for(int i = 0; i < numOutBands; i++)
				{
					delete outputData[i];
				}
				delete outputData;
			}
			
			if(outDataColumn != NULL)
			{
				delete[] outDataColumn;
			}
            
			throw e;
		}
		catch(RSGISImageBandException& e)
		{
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					delete dsOffsets[i];
				}
				delete[] dsOffsets;
			}
			
			if(bandOffsets != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					delete bandOffsets[i];
				}
				delete[] bandOffsets;
			}
			
			if(inputDataUpper != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					delete[] inputDataUpper[i];
				}
				delete[] inputDataUpper;
			}
            
            if(inputDataMain != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					delete[] inputDataMain[i];
				}
				delete[] inputDataMain;
			}
            
            if(inputDataLower != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					delete[] inputDataLower[i];
				}
				delete[] inputDataLower;
			}
			
			if(inDataBlock != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					for(int j = 0; j < windowSize; j++)
					{
						delete[] inDataBlock[i][j];
					}
					delete[] inDataBlock[i];
				}
				delete[] inDataBlock;
			}
			
			if(outputData != NULL)
			{
				for(int i = 0; i < numOutBands; i++)
				{
					delete outputData[i];
				}
				delete outputData;
			}
			
			if(outDataColumn != NULL)
			{
				delete[] outDataColumn;
			}
			
			throw e;
		}
		
		if(gdalTranslation != NULL)
		{
			delete[] gdalTranslation;
		}
		
		if(dsOffsets != NULL)
		{
			for(int i = 0; i < numDS; i++)
			{
				delete dsOffsets[i];
			}
			delete[] dsOffsets;
		}
		
		if(bandOffsets != NULL)
		{
			for(int i = 0; i < numInBands; i++)
			{
				delete bandOffsets[i];
			}
			delete[] bandOffsets;
		}
		
		if(inputDataUpper != NULL)
        {
            for(int i = 0; i < numInBands; i++)
            {
                delete[] inputDataUpper[i];
            }
            delete[] inputDataUpper;
        }
        
        if(inputDataMain != NULL)
        {
            for(int i = 0; i < numInBands; i++)
            {
                delete[] inputDataMain[i];
            }
            delete[] inputDataMain;
        }
        
        if(inputDataLower != NULL)
        {
            for(int i = 0; i < numInBands; i++)
            {
                delete[] inputDataLower[i];
            }
            delete[] inputDataLower;
        }
		
		if(inDataBlock != NULL)
		{
			for(int i = 0; i < numInBands; i++)
			{
				for(int j = 0; j < windowSize; j++)
				{
					delete[] inDataBlock[i][j];
				}
				delete[] inDataBlock[i];
			}
			delete[] inDataBlock;
		}
		
		if(outputData != NULL)
		{
			for(int i = 0; i < numOutBands; i++)
			{
				delete outputData[i];
			}
			delete outputData;
		}
		
		if(outDataColumn != NULL)
		{
			delete[] outDataColumn;
		}
	}
     
    /* Keeps returning a window of data based upon the supplied windowSize until all finished provides the extent on the central pixel (as envelope) at each iteration */
	void RSGISCalcImage::calcImageWindowDataExtent(GDALDataset **datasets, int numDS, std::string outputImage, int windowSize, std::string gdalFormat, GDALDataType gdalDataType)
	{
        GDALAllRegister();
        RSGISImageUtils imgUtils;
        double *gdalTranslation = new double[6];
        int **dsOffsets = new int*[numDS];
        for(int i = 0; i < numDS; i++)
        {
            dsOffsets[i] = new int[2];
        }
        int **bandOffsets = NULL;
        int height = 0;
        int width = 0;
        int numInBands = 0;
        int xBlockSize = 0;
        int yBlockSize = 0;
        size_t numPxlsInBlock = 0;
        
        float **inputDataUpper = NULL;
        float **inputDataMain = NULL;
        float **inputDataLower = NULL;
        double **outputData = NULL;
        float ***inDataBlock = NULL;
        double *outDataColumn = NULL;
        
        GDALDataset *outputImageDS = NULL;
        GDALRasterBand **inputRasterBands = NULL;
        GDALRasterBand **outputRasterBands = NULL;
        GDALDriver *gdalDriver = NULL;
        
        try
        {
            if(windowSize % 2 == 0)
            {
                throw RSGISImageCalcException("Window size needs to be an odd number (min = 3).");
            }
            else if(windowSize < 3)
            {
                throw RSGISImageCalcException("Window size needs to be 3 or greater and an odd number.");
            }
            int windowMid = floor(((float)windowSize)/2.0); // Starting at 0!! NOT 1 otherwise would be ceil.
            
            // Find image overlap
            imgUtils.getImageOverlap(datasets, numDS, dsOffsets, &width, &height, gdalTranslation, &xBlockSize, &yBlockSize);
            
            // Count number of image bands
            for(int i = 0; i < numDS; i++)
            {
                numInBands += datasets[i]->GetRasterCount();
            }
            
            // Create new Image
            gdalDriver = GetGDALDriverManager()->GetDriverByName(gdalFormat.c_str());
            if(gdalDriver == NULL)
            {
                throw RSGISImageBandException("Driver does not exists..");
            }
            
            outputImageDS = gdalDriver->Create(outputImage.c_str(), width, height, this->numOutBands, gdalDataType, NULL);
            
            if(outputImageDS == NULL)
            {
                throw RSGISImageBandException("Output image could not be created. Check filepath.");
            }
            outputImageDS->SetGeoTransform(gdalTranslation);
            if(useImageProj)
            {
                outputImageDS->SetProjection(datasets[0]->GetProjectionRef());
            }
            else
            {
                outputImageDS->SetProjection(proj.c_str());
            }
            
            // Get Image Input Bands
            bandOffsets = new int*[numInBands];
            inputRasterBands = new GDALRasterBand*[numInBands];
            int counter = 0;
            for(int i = 0; i < numDS; i++)
            {
                for(int j = 0; j < datasets[i]->GetRasterCount(); j++)
                {
                    inputRasterBands[counter] = datasets[i]->GetRasterBand(j+1);
                    bandOffsets[counter] = new int[2];
                    bandOffsets[counter][0] = dsOffsets[i][0];
                    bandOffsets[counter][1] = dsOffsets[i][1];
                    counter++;
                }
            }
            
            //Get Image Output Bands
            outputRasterBands = new GDALRasterBand*[this->numOutBands];
            for(int i = 0; i < this->numOutBands; i++)
            {
                outputRasterBands[i] = outputImageDS->GetRasterBand(i+1);
            }
            
            int outXBlockSize = 0;
            int outYBlockSize = 0;
            outputRasterBands[0]->GetBlockSize (&outXBlockSize, &outYBlockSize);
            
            if(outYBlockSize > yBlockSize)
            {
                yBlockSize = outYBlockSize;
            }
            
            int numOfLines = yBlockSize;
            if(yBlockSize < windowSize)
            {
                numOfLines = ceil(((float)windowSize)/((float)yBlockSize))*yBlockSize;
            }
            
            // Allocate memory
            numPxlsInBlock = width*numOfLines;
            inputDataUpper = new float*[numInBands];
            for(int i = 0; i < numInBands; i++)
            {
                inputDataUpper[i] = (float *) CPLMalloc(sizeof(float)*numPxlsInBlock);
                for(int k = 0; k < numPxlsInBlock; k++)
                {
                    inputDataUpper[i][k] = 0;
                }
            }
            
            inputDataMain = new float*[numInBands];
            for(int i = 0; i < numInBands; i++)
            {
                inputDataMain[i] = (float *) CPLMalloc(sizeof(float)*numPxlsInBlock);
                for(int k = 0; k < numPxlsInBlock; k++)
                {
                    inputDataMain[i][k] = 0;
                }
            }
            
            inputDataLower = new float*[numInBands];
            for(int i = 0; i < numInBands; i++)
            {
                inputDataLower[i] = (float *) CPLMalloc(sizeof(float)*numPxlsInBlock);
                for(int k = 0; k < numPxlsInBlock; k++)
                {
                    inputDataLower[i][k] = 0;
                }
            }
            
            inDataBlock = new float**[numInBands];
            for(int i = 0; i < numInBands; i++)
            {
                inDataBlock[i] = new float*[windowSize];
                for(int j = 0; j < windowSize; j++)
                {
                    inDataBlock[i][j] = new float[windowSize];
                }
            }
            
            outputData = new double*[this->numOutBands];
            for(int i = 0; i < this->numOutBands; i++)
            {
                outputData[i] = (double *) CPLMalloc(sizeof(double)*numPxlsInBlock);
            }
            outDataColumn = new double[this->numOutBands];
            
            
            int nYBlocks = floor(((double)height) / ((double)numOfLines));
            int remainRows = height - (nYBlocks * numOfLines);
            int rowOffset = 0;
            unsigned int line = 0;
            long cLinePxl = 0;
            long cPxl = 0;
            long dLinePxls = 0;
            int dWinX = 0;
            int dWinY = 0;
            double pxlTLX = 0;
            double pxlTLY = 0;
            double pxlWidth = 0;
            double pxlHeight = 0;
            geos::geom::Envelope pxlExt;
            
            pxlTLX = gdalTranslation[0];
            pxlTLY = gdalTranslation[3];
            pxlWidth = gdalTranslation[1];
            pxlHeight = gdalTranslation[5];
            
            if(pxlHeight < 0)
            {
                pxlHeight *= (-1);
            }
            
            int feedback = height/10.0;
            int feedbackCounter = 0;
            std::cout << "Started " << std::flush;
            
            if(nYBlocks > 0)
            {
                for(int i = 0; i < nYBlocks; i++)
                {
                    if(i == 0)
                    {
                        // Set Upper Block with Zeros.
                        for(int n = 0; n < numInBands; n++)
                        {
                            for(int k = 0; k < numPxlsInBlock; k++)
                            {
                                inputDataUpper[n][k] = 0;
                            }
                        }
                        
                        // Read Main Block
                        for(int n = 0; n < numInBands; n++)
                        {
                            rowOffset = bandOffsets[n][1] + (numOfLines * i);
                            inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, numOfLines, inputDataMain[n], width, numOfLines, GDT_Float32, 0, 0);
                        }
                        // Read Lower Block
                        for(int n = 0; n < numInBands; n++)
                        {
                            if(nYBlocks == 1)
                            {
                                if(remainRows > 0)
                                {
                                    rowOffset = bandOffsets[n][1] + (numOfLines * (i+1));
                                    inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, remainRows, inputDataLower[n], width, remainRows, GDT_Float32, 0, 0);
                                    for(int k = (remainRows*width); k < numPxlsInBlock; k++)
                                    {
                                        inputDataLower[n][k] = 0;
                                    }
                                }
                                else
                                {
                                    for(int k = 0; k < numPxlsInBlock; k++)
                                    {
                                        inputDataLower[n][k] = 0;
                                    }
                                }
                            }
                            else
                            {
                                rowOffset = bandOffsets[n][1] + (numOfLines * (i+1));
                                inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, numOfLines, inputDataLower[n], width, numOfLines, GDT_Float32, 0, 0);
                            }
                        }
                    }
                    else if(i == (nYBlocks-1))
                    {
                        // Shift Lower Block to Main Block
                        for(int n = 0; n < numInBands; n++)
                        {
                            for(int k = 0; k < numPxlsInBlock; k++)
                            {
                                inputDataUpper[n][k] = inputDataMain[n][k];
                            }
                        }
                        
                        // Shift Lower Block to Main Block
                        for(int n = 0; n < numInBands; n++)
                        {
                            for(int k = 0; k < numPxlsInBlock; k++)
                            {
                                inputDataMain[n][k] = inputDataLower[n][k];
                            }
                        }
                        
                        // Set Lower Block with Zeros.
                        for(int n = 0; n < numInBands; n++)
                        {
                            if(remainRows > 0)
                            {
                                rowOffset = bandOffsets[n][1] + (numOfLines * (i+1));
                                inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, remainRows, inputDataLower[n], width, remainRows, GDT_Float32, 0, 0);
                                for(int k = (remainRows*width); k < numPxlsInBlock; k++)
                                {
                                    inputDataLower[n][k] = 0;
                                }
                            }
                            else
                            {
                                for(int k = 0; k < numPxlsInBlock; k++)
                                {
                                    inputDataLower[n][k] = 0;
                                }
                            }
                        }
                    }
                    else
                    {
                        // Shift Lower Block to Main Block
                        for(int n = 0; n < numInBands; n++)
                        {
                            for(int k = 0; k < numPxlsInBlock; k++)
                            {
                                inputDataUpper[n][k] = inputDataMain[n][k];
                            }
                        }
                        
                        // Shift Lower Block to Main Block
                        for(int n = 0; n < numInBands; n++)
                        {
                            for(int k = 0; k < numPxlsInBlock; k++)
                            {
                                inputDataMain[n][k] = inputDataLower[n][k];
                            }
                        }
                        
                        // Read Lower Block
                        for(int n = 0; n < numInBands; n++)
                        {
                            rowOffset = bandOffsets[n][1] + (numOfLines * (i+1));
                            inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], rowOffset, width, numOfLines, inputDataLower[n], width, numOfLines, GDT_Float32, 0, 0);
                        }
                    }
                    
                    for(int m = 0; m < numOfLines; ++m)
                    {
                        line = (i*numOfLines)+m;
                        if((feedback != 0) && (line % feedback) == 0)
                        {
                            std::cout << "." << feedbackCounter << "." << std::flush;
                            feedbackCounter = feedbackCounter + 10;
                        }
                        
                        cLinePxl = m*width;
                        
                        pxlTLX = gdalTranslation[0];
                        for(int j = 0; j < width; j++)
                        {
                            cPxl = cLinePxl+j;
                            if(m < windowMid)
                            {
                                for(int y = 0; y < windowSize; y++)
                                {
                                    dWinY = y-windowMid;
                                    dLinePxls = dWinY * width;
                                    
                                    if((cPxl + dLinePxls) < 0)
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataUpper[n][(numPxlsInBlock+(cPxl+dLinePxls))+dWinX];
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataMain[n][(cPxl+dLinePxls)+dWinX];
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            else if(m > ((numOfLines-1)-windowMid))
                            {
                                for(int y = 0; y < windowSize; y++)
                                {
                                    dWinY = y-windowMid;
                                    dLinePxls = dWinY * width;
                                    
                                    if((cPxl + dLinePxls) >= numPxlsInBlock)
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataLower[n][((cPxl+dLinePxls)-numPxlsInBlock)+dWinX];
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataMain[n][(cPxl+dLinePxls)+dWinX];
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            else
                            {
                                for(int y = 0; y < windowSize; y++)
                                {
                                    dWinY = y-windowMid;
                                    dLinePxls = dWinY * width;
                                    
                                    for(int x = 0; x < windowSize; x++)
                                    {
                                        dWinX = x-windowMid;
                                        
                                        if((j+dWinX) < 0)
                                        {
                                            for(int n = 0; n < numInBands; n++)
                                            {
                                                inDataBlock[n][y][x] = 0;
                                            }
                                        }
                                        else if((j+dWinX) >= width)
                                        {
                                            for(int n = 0; n < numInBands; n++)
                                            {
                                                inDataBlock[n][y][x] = 0;
                                            }
                                        }
                                        else
                                        {
                                            for(int n = 0; n < numInBands; n++)
                                            {
                                                inDataBlock[n][y][x] = inputDataMain[n][(cPxl+dLinePxls)+dWinX];
                                            }
                                        }
                                    }
                                }
                            }

                            pxlExt.init(pxlTLX, (pxlTLX+pxlWidth), pxlTLY, (pxlTLY-pxlHeight));
                            this->calc->calcImageValue(inDataBlock, numInBands, windowSize, outDataColumn, pxlExt);
                            
                            for(int n = 0; n < this->numOutBands; n++)
                            {
                                outputData[n][cPxl] = outDataColumn[n];
                            }
                            pxlTLX += pxlWidth;
                        }
                        pxlTLY -= pxlHeight;
                    }
                    
                    for(int n = 0; n < this->numOutBands; n++)
                    {
                        outputRasterBands[n]->RasterIO(GF_Write, 0, (numOfLines * i), width, numOfLines, outputData[n], width, numOfLines, GDT_Float64, 0, 0);
                    }
                }
                
                if(remainRows > 0)
                {
                    // Shift Lower Block to Main Block
                    for(int n = 0; n < numInBands; n++)
                    {
                        for(int k = 0; k < numPxlsInBlock; k++)
                        {
                            inputDataUpper[n][k] = inputDataMain[n][k];
                        }
                    }
                    
                    // Shift Lower Block to Main Block
                    for(int n = 0; n < numInBands; n++)
                    {
                        for(int k = 0; k < numPxlsInBlock; k++)
                        {
                            inputDataMain[n][k] = inputDataLower[n][k];
                        }
                    }
                    
                    // Read Lower Block
                    for(int n = 0; n < numInBands; n++)
                    {
                        for(int k = 0; k < numPxlsInBlock; k++)
                        {
                            inputDataLower[n][k] = 0;
                        }
                    }
                    
                    for(int m = 0; m < remainRows; ++m)
                    {
                        line = (nYBlocks*numOfLines)+m;
                        if((feedback != 0) && (line % feedback) == 0)
                        {
                            std::cout << "." << feedbackCounter << "." << std::flush;
                            feedbackCounter = feedbackCounter + 10;
                        }
                        
                        cLinePxl = m*width;
                        
                        pxlTLX = gdalTranslation[0];
                        for(int j = 0; j < width; j++)
                        {
                            cPxl = cLinePxl+j;
                            if(m < windowMid)
                            {
                                for(int y = 0; y < windowSize; y++)
                                {
                                    dWinY = y-windowMid;
                                    dLinePxls = dWinY * width;
                                    
                                    if((cPxl + dLinePxls) < 0)
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataUpper[n][(numPxlsInBlock+(cPxl+dLinePxls))+dWinX];
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataMain[n][(cPxl+dLinePxls)+dWinX];
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            else if(m > ((numOfLines-1)-windowMid))
                            {
                                for(int y = 0; y < windowSize; y++)
                                {
                                    dWinY = y-windowMid;
                                    dLinePxls = dWinY * width;
                                    
                                    if((cPxl + dLinePxls) >= numPxlsInBlock)
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataLower[n][((cPxl+dLinePxls)-numPxlsInBlock)+dWinX];
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        for(int x = 0; x < windowSize; x++)
                                        {
                                            dWinX = x-windowMid;
                                            
                                            if((j+dWinX) < 0)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else if((j+dWinX) >= width)
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = 0;
                                                }
                                            }
                                            else
                                            {
                                                for(int n = 0; n < numInBands; n++)
                                                {
                                                    inDataBlock[n][y][x] = inputDataMain[n][(cPxl+dLinePxls)+dWinX];
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            else
                            {
                                for(int y = 0; y < windowSize; y++)
                                {
                                    dWinY = y-windowMid;
                                    dLinePxls = dWinY * width;
                                    
                                    for(int x = 0; x < windowSize; x++)
                                    {
                                        dWinX = x-windowMid;
                                        
                                        if((j+dWinX) < 0)
                                        {
                                            for(int n = 0; n < numInBands; n++)
                                            {
                                                inDataBlock[n][y][x] = 0;
                                            }
                                        }
                                        else if((j+dWinX) >= width)
                                        {
                                            for(int n = 0; n < numInBands; n++)
                                            {
                                                inDataBlock[n][y][x] = 0;
                                            }
                                        }
                                        else
                                        {
                                            for(int n = 0; n < numInBands; n++)
                                            {
                                                inDataBlock[n][y][x] = inputDataMain[n][(cPxl+dLinePxls)+dWinX];
                                            }
                                        }
                                    }
                                }
                            }
                            

                            pxlExt.init(pxlTLX, (pxlTLX+pxlWidth), pxlTLY, (pxlTLY-pxlHeight));
                            this->calc->calcImageValue(inDataBlock, numInBands, windowSize, outDataColumn, pxlExt);
                            
                            for(int n = 0; n < this->numOutBands; n++)
                            {
                                outputData[n][cPxl] = outDataColumn[n];
                            }
                            pxlTLX += pxlWidth;
                        }
                        pxlTLY -= pxlHeight;
                    }
                    
                    for(int n = 0; n < this->numOutBands; n++)
                    {
                        outputRasterBands[n]->RasterIO(GF_Write, 0, (nYBlocks*numOfLines), width, remainRows, outputData[n], width, remainRows, GDT_Float64, 0, 0);
                    }
                }
                
            }
            else
            {
                
            }
            std::cout << " Complete.\n";
        }
        catch(RSGISImageCalcException& e)
        {
            if(gdalTranslation != NULL)
            {
                delete[] gdalTranslation;
            }
            
            if(dsOffsets != NULL)
            {
                for(int i = 0; i < numDS; i++)
                {
                    delete dsOffsets[i];
                }
                delete[] dsOffsets;
            }
            
            if(bandOffsets != NULL)
            {
                for(int i = 0; i < numInBands; i++)
                {
                    delete bandOffsets[i];
                }
                delete[] bandOffsets;
            }
            
            if(inputDataUpper != NULL)
            {
                for(int i = 0; i < numInBands; i++)
                {
                    delete[] inputDataUpper[i];
                }
                delete[] inputDataUpper;
            }
            
            if(inputDataMain != NULL)
            {
                for(int i = 0; i < numInBands; i++)
                {
                    delete[] inputDataMain[i];
                }
                delete[] inputDataMain;
            }
            
            if(inputDataLower != NULL)
            {
                for(int i = 0; i < numInBands; i++)
                {
                    delete[] inputDataLower[i];
                }
                delete[] inputDataLower;
            }
            
            if(inDataBlock != NULL)
            {
                for(int i = 0; i < numInBands; i++)
                {
                    for(int j = 0; j < windowSize; j++)
                    {
                        delete[] inDataBlock[i][j];
                    }
                    delete[] inDataBlock[i];
                }
                delete[] inDataBlock;
            }
            
            if(outputData != NULL)
            {
                for(int i = 0; i < numOutBands; i++)
                {
                    delete outputData[i];
                }
                delete outputData;
            }
            
            if(outDataColumn != NULL)
            {
                delete[] outDataColumn;
            }
            
            throw e;
        }
        catch(RSGISImageBandException& e)
        {
            if(gdalTranslation != NULL)
            {
                delete[] gdalTranslation;
            }
            
            if(dsOffsets != NULL)
            {
                for(int i = 0; i < numDS; i++)
                {
                    delete dsOffsets[i];
                }
                delete[] dsOffsets;
            }
            
            if(bandOffsets != NULL)
            {
                for(int i = 0; i < numInBands; i++)
                {
                    delete bandOffsets[i];
                }
                delete[] bandOffsets;
            }
            
            if(inputDataUpper != NULL)
            {
                for(int i = 0; i < numInBands; i++)
                {
                    delete[] inputDataUpper[i];
                }
                delete[] inputDataUpper;
            }
            
            if(inputDataMain != NULL)
            {
                for(int i = 0; i < numInBands; i++)
                {
                    delete[] inputDataMain[i];
                }
                delete[] inputDataMain;
            }
            
            if(inputDataLower != NULL)
            {
                for(int i = 0; i < numInBands; i++)
                {
                    delete[] inputDataLower[i];
                }
                delete[] inputDataLower;
            }
            
            if(inDataBlock != NULL)
            {
                for(int i = 0; i < numInBands; i++)
                {
                    for(int j = 0; j < windowSize; j++)
                    {
                        delete[] inDataBlock[i][j];
                    }
                    delete[] inDataBlock[i];
                }
                delete[] inDataBlock;
            }
            
            if(outputData != NULL)
            {
                for(int i = 0; i < numOutBands; i++)
                {
                    delete outputData[i];
                }
                delete outputData;
            }
            
            if(outDataColumn != NULL)
            {
                delete[] outDataColumn;
            }
            
            throw e;
        }
        
        if(gdalTranslation != NULL)
        {
            delete[] gdalTranslation;
        }
        
        if(dsOffsets != NULL)
        {
            for(int i = 0; i < numDS; i++)
            {
                delete dsOffsets[i];
            }
            delete[] dsOffsets;
        }
        
        if(bandOffsets != NULL)
        {
            for(int i = 0; i < numInBands; i++)
            {
                delete bandOffsets[i];
            }
            delete[] bandOffsets;
        }
        
        if(inputDataUpper != NULL)
        {
            for(int i = 0; i < numInBands; i++)
            {
                delete[] inputDataUpper[i];
            }
            delete[] inputDataUpper;
        }
        
        if(inputDataMain != NULL)
        {
            for(int i = 0; i < numInBands; i++)
            {
                delete[] inputDataMain[i];
            }
            delete[] inputDataMain;
        }
        
        if(inputDataLower != NULL)
        {
            for(int i = 0; i < numInBands; i++)
            {
                delete[] inputDataLower[i];
            }
            delete[] inputDataLower;
        }
        
        if(inDataBlock != NULL)
        {
            for(int i = 0; i < numInBands; i++)
            {
                for(int j = 0; j < windowSize; j++)
                {
                    delete[] inDataBlock[i][j];
                }
                delete[] inDataBlock[i];
            }
            delete[] inDataBlock;
        }
        
        if(outputData != NULL)
        {
            for(int i = 0; i < numOutBands; i++)
            {
                delete outputData[i];
            }
            delete outputData;
        }
        
        if(outDataColumn != NULL)
        {
            delete[] outDataColumn;
        }
        
        GDALClose(outputImageDS);
    }
	
	void RSGISCalcImage::calcImageWithinPolygon(GDALDataset **datasets, int numDS, std::string outputImage, geos::geom::Envelope *env, geos::geom::Polygon *poly, float nodata, pixelInPolyOption pixelPolyOption, std::string gdalFormat,  GDALDataType gdalDataType)
	{
		GDALAllRegister();
		RSGISImageUtils imgUtils;
		double *gdalTranslation = new double[6];
		int **dsOffsets = new int*[numDS];
		for(int i = 0; i < numDS; i++)
		{
			dsOffsets[i] = new int[2];
		}
		int **bandOffsets = NULL;
		int height = 0;
		int width = 0;
		int numInBands = 0;
		
		float **inputData = NULL;
		double **outputData = NULL;
		float *inDataColumn = NULL;
		double *outDataColumn = NULL;
		
		GDALDataset *outputImageDS = NULL;
		GDALRasterBand **inputRasterBands = NULL;
		GDALRasterBand **outputRasterBands = NULL;
		GDALDriver *gdalDriver = NULL;
		geos::geom::Envelope extent;
		geos::geom::Coordinate pxlCentre;
		const geos::geom::GeometryFactory *geomFactory = geos::geom::GeometryFactory::getDefaultInstance();
		double pxlTLX = 0;
		double pxlTLY = 0;
		double pxlWidth = 0;
		double pxlHeight = 0;
		
		try
		{
			// Find image overlap
			imgUtils.getImageOverlap(datasets, numDS, dsOffsets, &width, &height, gdalTranslation, env);
			
			// Count number of image bands
			for(int i = 0; i < numDS; i++)
			{
				numInBands += datasets[i]->GetRasterCount();
			}
			
			// Create new Image
			gdalDriver = GetGDALDriverManager()->GetDriverByName(gdalFormat.c_str());
			if(gdalDriver == NULL)
			{
				throw RSGISImageBandException("ENVI driver does not exists..");
			}

			outputImageDS = gdalDriver->Create(outputImage.c_str(), width, height, this->numOutBands, gdalDataType, NULL);
			
			if(outputImageDS == NULL)
			{
				throw RSGISImageBandException("Output image could not be created. Check filepath.");
			}
			outputImageDS->SetGeoTransform(gdalTranslation);
			if(useImageProj)
			{
				outputImageDS->SetProjection(datasets[0]->GetProjectionRef());
			}
			else
			{
				outputImageDS->SetProjection(proj.c_str());
			}
			
			pxlTLX = gdalTranslation[0];
			pxlTLY = gdalTranslation[3];
			pxlWidth = gdalTranslation[1];
			pxlHeight = gdalTranslation[1];
			
			// Get Image Input Bands
			bandOffsets = new int*[numInBands];
			inputRasterBands = new GDALRasterBand*[numInBands];
			int counter = 0;
			for(int i = 0; i < numDS; i++)
			{
				for(int j = 0; j < datasets[i]->GetRasterCount(); j++)
				{
					inputRasterBands[counter] = datasets[i]->GetRasterBand(j+1);
					bandOffsets[counter] = new int[2];
					bandOffsets[counter][0] = dsOffsets[i][0];
					bandOffsets[counter][1] = dsOffsets[i][1];
					counter++;
				}
			}
			
			//Get Image Output Bands
			outputRasterBands = new GDALRasterBand*[this->numOutBands];
			for(int i = 0; i < this->numOutBands; i++)
			{
				outputRasterBands[i] = outputImageDS->GetRasterBand(i+1);
			}
			
			// Allocate memory
			inputData = new float*[numInBands];
			for(int i = 0; i < numInBands; i++)
			{
				inputData[i] = (float *) CPLMalloc(sizeof(float)*width);
			}
			inDataColumn = new float[numInBands];
			
			outputData = new double*[this->numOutBands];
			for(int i = 0; i < this->numOutBands; i++)
			{
				outputData[i] = (double *) CPLMalloc(sizeof(double)*width);
			}
			outDataColumn = new double[this->numOutBands];
			
			int feedback = height/10;
			if (feedback == 0)
            {
                feedback = 1; // Set feedback to 1
            }
			int feedbackCounter = 0;
			if(height > 100)
			{
				std::cout << "Started " << std::flush;
			}
            // Loop images to process data
			for(int i = 0; i < height; i++)
			{				
				if((feedback != 0) && ((i % feedback) == 0))
				{
					std::cout << "." << feedbackCounter << "." << std::flush;
					feedbackCounter = feedbackCounter + 10;
				}
				
				for(int n = 0; n < numInBands; n++)
				{
					inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], (bandOffsets[n][1]+i), width, 1, inputData[n], width, 1, GDT_Float32, 0, 0);
				}
				
				for(int j = 0; j < width; j++)
				{
					for(int n = 0; n < numInBands; n++)
					{
						inDataColumn[n] = inputData[n][j];
					}
					
                    geos::geom::Coordinate pxlCentre;
					geos::geom::Point *pt = NULL;
					
					extent.init(pxlTLX, (pxlTLX+pxlWidth), pxlTLY, (pxlTLY-pxlHeight));
					extent.centre(pxlCentre);
					pt = geomFactory->createPoint(pxlCentre);
					
					if (pixelPolyOption == polyContainsPixelCenter) 
					{
						
						if(poly->contains(pt)) // If polygon contains pixel center
						{
							this->calc->calcImageValue(inDataColumn, numInBands, outDataColumn);
						}
						else
						{
							for(int n = 0; n < this->numOutBands; n++)
							{
								outDataColumn[n] = nodata;
							}
						}
						
					}
					else if (pixelPolyOption == pixelAreaInPoly) 
					{
						geos::geom::CoordinateSequence *coords = NULL;
						geos::geom::LinearRing *ring = NULL;
						geos::geom::Polygon *pixelGeosPoly = NULL;
						geos::geom::Geometry *intersectionGeom;
						
						coords = new geos::geom::CoordinateArraySequence();
						coords->add(geos::geom::Coordinate(pxlTLX, pxlTLY, 0));
						coords->add(geos::geom::Coordinate(pxlTLX + pxlWidth, pxlTLY, 0));
						coords->add(geos::geom::Coordinate(pxlTLX + pxlWidth, pxlTLY - pxlHeight, 0));
						coords->add(geos::geom::Coordinate(pxlTLX, pxlTLY - pxlHeight, 0));
						coords->add(geos::geom::Coordinate(pxlTLX, pxlTLY, 0));
						ring = geomFactory->createLinearRing(coords);
						pixelGeosPoly = geomFactory->createPolygon(ring, NULL);
						
						
						intersectionGeom = pixelGeosPoly->intersection(poly);
						double intersectionArea = intersectionGeom->getArea()  / pixelGeosPoly->getArea();
						
						if(intersectionArea > 0)
						{
							for(int n = 0; n < numInBands; n++)
							{
								this->calc->calcImageValue(inDataColumn, numInBands, outDataColumn);
							}
						}
						else
						{
							for(int n = 0; n < this->numOutBands; n++)
							{
								outDataColumn[n] = nodata;
							}
						}
					}
					else 
					{
						RSGISPixelInPoly *pixelInPoly;
						OGRLinearRing *ring;
						OGRPolygon *pixelPoly;
						OGRPolygon *ogrPoly;
						OGRGeometry *ogrGeom;
						
						pixelInPoly = new RSGISPixelInPoly(pixelPolyOption);
						
						ring = new OGRLinearRing();
						ring->addPoint(pxlTLX, pxlTLY, 0);
						ring->addPoint(pxlTLX + pxlWidth, pxlTLY, 0);
						ring->addPoint(pxlTLX + pxlWidth, pxlTLY - pxlHeight, 0);
						ring->addPoint(pxlTLX, pxlTLY - pxlHeight, 0);
						ring->addPoint(pxlTLX, pxlTLY, 0);
						
						pixelPoly = new OGRPolygon();
						pixelPoly->addRingDirectly(ring);
						
						ogrPoly = new OGRPolygon();
						const geos::geom::LineString *line = poly->getExteriorRing();
						OGRLinearRing *ogrRing = new OGRLinearRing();
						const geos::geom::CoordinateSequence *coords = line->getCoordinatesRO();
						int numCoords = coords->getSize();
						geos::geom::Coordinate coord;
						for(int i = 0; i < numCoords; i++)
						{
							coord = coords->getAt(i);
							ogrRing->addPoint(coord.x, coord.y, coord.z);
						}
						ogrPoly->addRing(ogrRing);
						ogrGeom = (OGRGeometry *) ogrPoly;
						
						// Check if the pixel should be classed as part of the polygon using the specified method
						if (pixelInPoly->findPixelInPoly(ogrGeom, pixelPoly)) 
						{
							this->calc->calcImageValue(inDataColumn, numInBands, outDataColumn);
						}
						else
						{
							for(int n = 0; n < this->numOutBands; n++)
							{
								outDataColumn[n] = nodata;
							}
						}
						
						
						// Tidy
						delete pixelInPoly;
						delete pixelPoly;
						delete ogrPoly;
					}
					
					delete pt;
					
					pxlTLX += pxlWidth;
					
					for(int n = 0; n < this->numOutBands; n++)
					{
						outputData[n][j] = outDataColumn[n];
					}
					
				}
				pxlTLY -= pxlHeight;
				pxlTLX = gdalTranslation[0];
				
				for(int n = 0; n < this->numOutBands; n++)
				{
					outputRasterBands[n]->RasterIO(GF_Write, 0, i, width, 1, outputData[n], width, 1, GDT_Float64, 0, 0);
				}
			}
			if (height > 100) 
			{
				std::cout << " Complete.\n";
			}
		}
		catch(RSGISImageCalcException& e)
		{
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					if(dsOffsets[i] != NULL)
					{
						delete[] dsOffsets[i];
					}
				} 
				delete[] dsOffsets;
			}
			
			if(bandOffsets != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(bandOffsets[i] != NULL)
					{
						delete[] bandOffsets[i];
					}
				}
				delete[] bandOffsets;
			}			
			
			if(inputData != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(inputData[i] != NULL)
					{
						delete[] inputData[i];
					}
				}
				delete[] inputData;
			}
			
			if(outputData != NULL)
			{
				for(int i = 0; i < this->numOutBands; i++)
				{
					if(outputData[i] != NULL)
					{
						delete[] outputData[i];
					}
				}
				delete[] outputData;
			}
			
			if(inDataColumn != NULL)
			{
				delete[] inDataColumn;
			}
			
			if(outDataColumn != NULL)
			{
				delete[] outDataColumn;
			}
			
			if(inputRasterBands != NULL)
			{
				delete[] inputRasterBands;
			}
			
			if(outputRasterBands != NULL)
			{
				delete[] outputRasterBands;
			}
			throw e;
		}
		catch(RSGISImageBandException& e)
		{
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					if(dsOffsets[i] != NULL)
					{
						delete[] dsOffsets[i];
					}
				} 
				delete[] dsOffsets;
			}
			
			if(bandOffsets != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(bandOffsets[i] != NULL)
					{
						delete[] bandOffsets[i];
					}
				}
				delete[] bandOffsets;
			}			
			
			if(inputData != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(inputData[i] != NULL)
					{
						delete[] inputData[i];
					}
				}
				delete[] inputData;
			}
			
			if(outputData != NULL)
			{
				for(int i = 0; i < this->numOutBands; i++)
				{
					if(outputData[i] != NULL)
					{
						delete[] outputData[i];
					}
				}
				delete[] outputData;
			}
			
			if(inDataColumn != NULL)
			{
				delete[] inDataColumn;
			}
			
			if(outDataColumn != NULL)
			{
				delete[] outDataColumn;
			}
			
			if(inputRasterBands != NULL)
			{
				delete[] inputRasterBands;
			}
			
			if(outputRasterBands != NULL)
			{
				delete[] outputRasterBands;
			}
			throw e;
		}
		
		GDALClose(outputImageDS);
		
		if(gdalTranslation != NULL)
		{
			delete[] gdalTranslation;
		}
		
		if(dsOffsets != NULL)
		{
			for(int i = 0; i < numDS; i++)
			{
				if(dsOffsets[i] != NULL)
				{
					delete[] dsOffsets[i];
				}
			} 
			delete[] dsOffsets;
		}
		
		if(bandOffsets != NULL)
		{
			for(int i = 0; i < numInBands; i++)
			{
				if(bandOffsets[i] != NULL)
				{
					delete[] bandOffsets[i];
				}
			}
			delete[] bandOffsets;
		}		
		
		if(inputData != NULL)
		{
			for(int i = 0; i < numInBands; i++)
			{
				if(inputData[i] != NULL)
				{
					delete[] inputData[i];
				}
			}
			delete[] inputData;
		}
		
		if(outputData != NULL)
		{
			for(int i = 0; i < this->numOutBands; i++)
			{
				if(outputData[i] != NULL)
				{
					delete[] outputData[i];
				}
			}
			delete[] outputData;
		}
		
		if(inDataColumn != NULL)
		{
			delete[] inDataColumn;
		}
		
		if(outDataColumn != NULL)
		{
			delete[] outDataColumn;
		}
		
		if(inputRasterBands != NULL)
		{
			delete[] inputRasterBands;
		}
		
		if(outputRasterBands != NULL)
		{
			delete[] outputRasterBands;
		}		
	}
	
	/* calcImageWithinPolygon - takes existing output image */
	void RSGISCalcImage::calcImageWithinPolygon(GDALDataset **datasets, int numDS, geos::geom::Envelope *env, geos::geom::Polygon *poly, pixelInPolyOption pixelPolyOption)
	{
		/* Input and output images as gdal datasets
		 * Stored as:
		 * Input DS
		 * Output DS
		 * numDS = numinput + 1 (output band)
		 */
		
		GDALAllRegister();
		RSGISImageUtils imgUtils;
		double *gdalTranslation = new double[6];
		int **dsOffsets = new int*[numDS];
		for(int i = 0; i < numDS; i++)
		{
			dsOffsets[i] = new int[2];
		}
		int **bandOffsets = NULL;
		int height = 0;
		int width = 0;
		int numInBands = 0;
		//int numOutBands = 0;
		
		float **inputData = NULL;
		double **outputData = NULL;
		float *inDataColumn = NULL;
		double *outDataColumn = NULL;
		
		GDALRasterBand **inputRasterBands = NULL;
		GDALRasterBand **outputRasterBands = NULL;
		geos::geom::Envelope extent;
		geos::geom::Coordinate pxlCentre;
		const geos::geom::GeometryFactory *geomFactory = geos::geom::GeometryFactory::getDefaultInstance();
		double pxlTLX = 0;
		double pxlTLY = 0;
		double pxlWidth = 0;
		double pxlHeight = 0;
				
		try
		{
			// Find image overlap
			imgUtils.getImageOverlap(datasets, numDS, dsOffsets, &width, &height, gdalTranslation, env);
			
			// Count number of input image bands
			for(int i = 0; i < numDS - 1; i++)
			{
				numInBands += datasets[i]->GetRasterCount();
			}
			// Count number of output image bands
			this->numOutBands = datasets[numDS-1]->GetRasterCount();
			
			// Get Pixel Size
			pxlTLX = gdalTranslation[0];
			pxlTLY = gdalTranslation[3];
			pxlWidth = gdalTranslation[1];
			pxlHeight = gdalTranslation[1];
			
			if(pxlHeight < 0) // Check resolution is positive (negative in Southern hemisphere).
			{
				pxlHeight = pxlHeight * (-1);
			}
			
			// Get Image Input Bands
			bandOffsets = new int*[numInBands+numOutBands];
			inputRasterBands = new GDALRasterBand*[numInBands];
			int counter = 0;
			for(int i = 0; i < numDS - 1; i++)
			{
				for(int j = 0; j < datasets[i]->GetRasterCount(); j++)
				{
					inputRasterBands[counter] = datasets[i]->GetRasterBand(j+1);
					bandOffsets[counter] = new int[2];
					bandOffsets[counter][0] = dsOffsets[i][0];
					bandOffsets[counter][1] = dsOffsets[i][1];
					counter++;
				}
			}
			
			// Get Output Input Bands
			outputRasterBands = new GDALRasterBand*[this->numOutBands];
			for(int j = 0; j < datasets[numDS-1]->GetRasterCount(); j++)
			{
				outputRasterBands[j] = datasets[numDS-1]->GetRasterBand(j+1);
				bandOffsets[counter] = new int[2];
				bandOffsets[counter][0] = dsOffsets[numDS-1][0];
				bandOffsets[counter][1] = dsOffsets[numDS-1][1];
				counter++;
			}
			
			// Allocate memory
			inputData = new float*[numInBands];
			for(int i = 0; i < numInBands; i++)
			{
				inputData[i] = (float *) CPLMalloc(sizeof(float)*width);
			}
			inDataColumn = new float[numInBands];
			
			outputData = new double*[this->numOutBands];
			for(int i = 0; i < this->numOutBands; i++)
			{
				outputData[i] = (double *) CPLMalloc(sizeof(double)*width);
			}
			outDataColumn = new double[this->numOutBands];
			
			int feedback = height/10;
			if (feedback == 0) {feedback = 1;} // Set feedback to 1
			int feedbackCounter = 0;
			if(height > 100)
			{
				std::cout << "\rStarted " << std::flush;
			}			
			// Loop images to process data
			for(int i = 0; i < height; i++)
			{				
				if((feedback != 0) && ((i % feedback) == 0))
				{
					std::cout << "." << feedbackCounter << "." << std::flush;
					feedbackCounter = feedbackCounter + 10;
				}
				
				for(int n = 0; n < numInBands; n++)
				{
					inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], (bandOffsets[n][1]+i), width, 1, inputData[n], width, 1, GDT_Float32, 0, 0);
				}
				for(int n = 0; n < this->numOutBands; n++)
				{
					outputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], (bandOffsets[n][1]+i), width, 1, outputData[n], width, 1, GDT_Float64, 0, 0);
				}
				
				
				for(int j = 0; j < width; j++)
				{
					for(int n = 0; n < numInBands; n++)
					{
						inDataColumn[n] = inputData[n][j];
					}
					
					geos::geom::Coordinate pxlCentre;
					geos::geom::Point *pt = NULL;
					
					extent.init(pxlTLX, (pxlTLX+pxlWidth), pxlTLY, (pxlTLY-pxlHeight));
					extent.centre(pxlCentre);
					pt = geomFactory->createPoint(pxlCentre);
					
					if (pixelPolyOption == polyContainsPixelCenter) 
					{
						if(poly->contains(pt)) // If polygon contains pixel center
						{
							this->calc->calcImageValue(inDataColumn, numInBands, outDataColumn);
						}
						else 
						{
							for (int n = 0; n < this->numOutBands; n++) 
							{
								outDataColumn[n] = outputData[n][j];
							}
						}

					}
					else if (pixelPolyOption == pixelAreaInPoly) 
					{
						geos::geom::CoordinateSequence *coords = NULL;
						geos::geom::LinearRing *ring = NULL;
						geos::geom::Polygon *pixelGeosPoly = NULL;
						geos::geom::Geometry *intersectionGeom;
						
						coords = new geos::geom::CoordinateArraySequence();
						coords->add(geos::geom::Coordinate(pxlTLX, pxlTLY, 0));
						coords->add(geos::geom::Coordinate(pxlTLX + pxlWidth, pxlTLY, 0));
						coords->add(geos::geom::Coordinate(pxlTLX + pxlWidth, pxlTLY - pxlHeight, 0));
						coords->add(geos::geom::Coordinate(pxlTLX, pxlTLY - pxlHeight, 0));
						coords->add(geos::geom::Coordinate(pxlTLX, pxlTLY, 0));
						ring = geomFactory->createLinearRing(coords);
						pixelGeosPoly = geomFactory->createPolygon(ring, NULL);
						
						
						intersectionGeom = pixelGeosPoly->intersection(poly);
						double intersectionArea = intersectionGeom->getArea()  / pixelGeosPoly->getArea();
						
						if(intersectionArea > 0)
						{
							for(int n = 0; n < numInBands; n++)
							{
								this->calc->calcImageValue(inDataColumn, numInBands, outDataColumn);
							}
						}
						else 
						{
							for (int n = 0; n < this->numOutBands; n++) 
							{
								outDataColumn[n] = outputData[n][j];
							}
						}
					}
					else 
					{
						RSGISPixelInPoly *pixelInPoly;
						OGRLinearRing *ring;
						OGRPolygon *pixelPoly;
						OGRPolygon *ogrPoly;
						OGRGeometry *ogrGeom;
						
						pixelInPoly = new RSGISPixelInPoly(pixelPolyOption);
						
						ring = new OGRLinearRing();
						ring->addPoint(pxlTLX, pxlTLY, 0);
						ring->addPoint(pxlTLX + pxlWidth, pxlTLY, 0);
						ring->addPoint(pxlTLX + pxlWidth, pxlTLY - pxlHeight, 0);
						ring->addPoint(pxlTLX, pxlTLY - pxlHeight, 0);
						ring->addPoint(pxlTLX, pxlTLY, 0);
						
						pixelPoly = new OGRPolygon();
						pixelPoly->addRingDirectly(ring);
						
						ogrPoly = new OGRPolygon();
						const geos::geom::LineString *line = poly->getExteriorRing();
						OGRLinearRing *ogrRing = new OGRLinearRing();
						const geos::geom::CoordinateSequence *coords = line->getCoordinatesRO();
						int numCoords = coords->getSize();
						geos::geom::Coordinate coord;
						for(int i = 0; i < numCoords; i++)
						{
							coord = coords->getAt(i);
							ogrRing->addPoint(coord.x, coord.y, coord.z);
						}
						ogrPoly->addRing(ogrRing);
						ogrGeom = (OGRGeometry *) ogrPoly;
						
						// Check if the pixel should be classed as part of the polygon using the specified method
						if (pixelInPoly->findPixelInPoly(ogrGeom, pixelPoly)) 
						{
							this->calc->calcImageValue(inDataColumn, numInBands, outDataColumn);
						}
						else 
						{
							for (int n = 0; n < this->numOutBands; n++) 
							{
								outDataColumn[n] = outputData[n][j];
							}
						}
						
						// Tidy
						delete pixelInPoly;
						delete pixelPoly;
						delete ogrPoly;
					}
					
					delete pt;
					
					pxlTLX += pxlWidth;
					
					for(int n = 0; n < this->numOutBands; n++)
					{
						outputData[n][j] = outDataColumn[n];
					}
					
				}
				pxlTLY -= pxlHeight;
				pxlTLX = gdalTranslation[0];
				
				
				for(int n = 0; n < this->numOutBands; n++)
				{
					outputRasterBands[n]->RasterIO(GF_Write, bandOffsets[n][0], (bandOffsets[n][1]+i), width, 1, outputData[n], width, 1, GDT_Float64, 0, 0);
				}
			}
			if (height > 100) 
			{
				std::cout << "Complete\r" << std::flush;
				std::cout << "\r                                                                                                                            \r" << std::flush;
			}
		}
		catch(RSGISImageCalcException& e)
		{
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					if(dsOffsets[i] != NULL)
					{
						delete[] dsOffsets[i];
					}
				} 
				delete[] dsOffsets;
			}
			
			if(bandOffsets != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(bandOffsets[i] != NULL)
					{
						delete[] bandOffsets[i];
					}
				}
				delete[] bandOffsets;
			}			
			
			if(inputData != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(inputData[i] != NULL)
					{
						delete[] inputData[i];
					}
				}
				delete[] inputData;
			}
			
			if(outputData != NULL)
			{
				for(int i = 0; i < this->numOutBands; i++)
				{
					if(outputData[i] != NULL)
					{
						delete[] outputData[i];
					}
				}
				delete[] outputData;
			}
			
			if(inDataColumn != NULL)
			{
				delete[] inDataColumn;
			}
			
			if(outDataColumn != NULL)
			{
				delete[] outDataColumn;
			}
			
			if(inputRasterBands != NULL)
			{
				delete[] inputRasterBands;
			}
			
			if(outputRasterBands != NULL)
			{
				delete[] outputRasterBands;
			}
			throw e;
		}
		catch(RSGISImageBandException& e)
		{
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					if(dsOffsets[i] != NULL)
					{
						delete[] dsOffsets[i];
					}
				} 
				delete[] dsOffsets;
			}
			
			if(bandOffsets != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(bandOffsets[i] != NULL)
					{
						delete[] bandOffsets[i];
					}
				}
				delete[] bandOffsets;
			}			
			
			if(inputData != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(inputData[i] != NULL)
					{
						delete[] inputData[i];
					}
				}
				delete[] inputData;
			}
			
			if(outputData != NULL)
			{
				for(int i = 0; i < this->numOutBands; i++)
				{
					if(outputData[i] != NULL)
					{
						delete[] outputData[i];
					}
				}
				delete[] outputData;
			}
			
			if(inDataColumn != NULL)
			{
				delete[] inDataColumn;
			}
			
			if(outDataColumn != NULL)
			{
				delete[] outDataColumn;
			}
			
			if(inputRasterBands != NULL)
			{
				delete[] inputRasterBands;
			}
			
			if(outputRasterBands != NULL)
			{
				delete[] outputRasterBands;
			}
			throw e;
		}
				
		if(gdalTranslation != NULL)
		{
			delete[] gdalTranslation;
		}
		
		if(dsOffsets != NULL)
		{
			for(int i = 0; i < numDS; i++)
			{
				if(dsOffsets[i] != NULL)
				{
					delete[] dsOffsets[i];
				}
			} 
			delete[] dsOffsets;
		}
		
		if(bandOffsets != NULL)
		{
			for(int i = 0; i < numInBands; i++)
			{
				if(bandOffsets[i] != NULL)
				{
					delete[] bandOffsets[i];
				}
			}
			delete[] bandOffsets;
		}		
		
		if(inputData != NULL)
		{
			for(int i = 0; i < numInBands; i++)
			{
				if(inputData[i] != NULL)
				{
					delete[] inputData[i];
				}
			}
			delete[] inputData;
		}
		
		if(outputData != NULL)
		{
			for(int i = 0; i < this->numOutBands; i++)
			{
				if(outputData[i] != NULL)
				{
					delete[] outputData[i];
				}
			}
			delete[] outputData;
		}
		
		if(inDataColumn != NULL)
		{
			delete[] inDataColumn;
		}
		
		if(outDataColumn != NULL)
		{
			delete[] outDataColumn;
		}
		
		if(inputRasterBands != NULL)
		{
			delete[] inputRasterBands;
		}
		
		if(outputRasterBands != NULL)
		{
			delete[] outputRasterBands;
		}		
	}
    
    /* calcImageWithinPolygon - Does not use an output image */
	void RSGISCalcImage::calcImageWithinPolygonExtent(GDALDataset **datasets, int numDS, geos::geom::Envelope *env, geos::geom::Polygon *poly, pixelInPolyOption pixelPolyOption)
	{
		GDALAllRegister();
		RSGISImageUtils imgUtils;
		double *gdalTranslation = new double[6];
		int **dsOffsets = new int*[numDS];
		for(int i = 0; i < numDS; i++)
		{
			dsOffsets[i] = new int[2];
		}
		int **bandOffsets = NULL;
		int height = 0;
		int width = 0;
		int numInBands = 0;
		
		float **inputData = NULL;
		float *inDataColumn = NULL;
		
		GDALRasterBand **inputRasterBands = NULL;
		geos::geom::Envelope extent;
		geos::geom::Coordinate pxlCentre;
		const geos::geom::GeometryFactory *geomFactory = geos::geom::GeometryFactory::getDefaultInstance();
		double pxlTLX = 0;
		double pxlTLY = 0;
		double pxlWidth = 0;
		double pxlHeight = 0;
        
		try
		{
			// CHECK ENVELOPE IS AT LEAST 1 x 1 Pixel
			/* For small polygons the the envelope can be smaller than a pixel, which will cause problems.
			 * To avoid errors a buffer is applied to the envelope and this buffered envelope is used for 'getImageOverlap'
			 * The buffered envelope is created and destroyed in this class and does not effect the passed in envelope
			 */
			bool buffer = false;
			
			double *transformations = new double[6];
			datasets[0]->GetGeoTransform(transformations);
			
			// Get pixel size
			pxlWidth = transformations[1];
			pxlHeight = transformations[5];
			
			if(pxlHeight < 0) // Check resolution is positive (negative in Southern hemisphere).
			{
				pxlHeight = pxlHeight * (-1);
			}
			
			delete[] transformations;
			
			geos::geom::Envelope *bufferedEnvelope = NULL;
			
			if ((env->getWidth() < pxlWidth) | (env->getHeight() < pxlHeight))
			{
				buffer = true;
				bufferedEnvelope = new geos::geom::Envelope(env->getMinX() - pxlWidth / 2, env->getMaxX() + pxlWidth / 2, env->getMinY() - pxlHeight / 2, env->getMaxY() + pxlHeight / 2);
			}
			
			// Find image overlap
			gdalTranslation = new double[6];
			
			if (buffer) // Use buffered envelope.
			{
				imgUtils.getImageOverlap(datasets, numDS, dsOffsets, &width, &height, gdalTranslation, bufferedEnvelope);
			}
			else // Use envelope passed in.
			{
				imgUtils.getImageOverlap(datasets, numDS, dsOffsets, &width, &height, gdalTranslation, env);
			}
			
			// Count number of input image bands
			for(int i = 0; i < numDS; i++)
			{
				numInBands += datasets[i]->GetRasterCount();
			}
            
			// Get Pixel Size
			pxlTLX = gdalTranslation[0];
			pxlTLY = gdalTranslation[3];
			pxlWidth = gdalTranslation[1];
			pxlHeight = gdalTranslation[1];
			
			if(pxlHeight < 0) // Check resolution is positive (negative in Southern hemisphere).
			{
				pxlHeight = pxlHeight * (-1);
			}
			
			// Get Image Input Bands
			bandOffsets = new int*[numInBands];
			inputRasterBands = new GDALRasterBand*[numInBands];
			int counter = 0;
			for(int i = 0; i < numDS; i++)
			{
				for(int j = 0; j < datasets[i]->GetRasterCount(); j++)
				{
					inputRasterBands[counter] = datasets[i]->GetRasterBand(j+1);
					bandOffsets[counter] = new int[2];
					bandOffsets[counter][0] = dsOffsets[i][0];
					bandOffsets[counter][1] = dsOffsets[i][1];
					counter++;
				}
			}
			
			// Allocate memory
			inputData = new float*[numInBands];
			for(int i = 0; i < numInBands; i++)
			{
				inputData[i] = (float *) CPLMalloc(sizeof(float)*width);
			}
			inDataColumn = new float[numInBands];
            
			// Loop images to process data
			for(int i = 0; i < height; i++)
			{				
				for(int n = 0; n < numInBands; n++)
				{
					inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], (bandOffsets[n][1]+i), width, 1, inputData[n], width, 1, GDT_Float32, 0, 0);
				}
				
				for(int j = 0; j < width; j++)
				{
					for(int n = 0; n < numInBands; n++)
					{
						inDataColumn[n] = inputData[n][j];
					}
					
					geos::geom::Coordinate pxlCentre;
					geos::geom::Point *pt = NULL;
					
					extent.init(pxlTLX, (pxlTLX+pxlWidth), pxlTLY, (pxlTLY-pxlHeight));
					extent.centre(pxlCentre);
					pt = geomFactory->createPoint(pxlCentre);
					
					if (pixelPolyOption == polyContainsPixelCenter) 
					{
						if(poly->contains(pt)) // If polygon contains pixel center
						{
							this->calc->calcImageValue(inDataColumn, numInBands, extent);
						}
					}
					else if (pixelPolyOption == pixelAreaInPoly) 
					{
						geos::geom::CoordinateSequence *coords = NULL;
						geos::geom::LinearRing *ring = NULL;
						geos::geom::Polygon *pixelGeosPoly = NULL;
						geos::geom::Geometry *intersectionGeom;
						
						coords = new geos::geom::CoordinateArraySequence();
						coords->add(geos::geom::Coordinate(pxlTLX, pxlTLY, 0));
						coords->add(geos::geom::Coordinate(pxlTLX + pxlWidth, pxlTLY, 0));
						coords->add(geos::geom::Coordinate(pxlTLX + pxlWidth, pxlTLY - pxlHeight, 0));
						coords->add(geos::geom::Coordinate(pxlTLX, pxlTLY - pxlHeight, 0));
						coords->add(geos::geom::Coordinate(pxlTLX, pxlTLY, 0));
						ring = geomFactory->createLinearRing(coords);
						pixelGeosPoly = geomFactory->createPolygon(ring, NULL);
						
						
						intersectionGeom = pixelGeosPoly->intersection(poly);
						double intersectionArea = intersectionGeom->getArea()  / pixelGeosPoly->getArea();
						
						if(intersectionArea > 0)
						{
							this->calc->calcImageValue(inDataColumn, numInBands, extent);
						}
					}
					else 
					{
						RSGISPixelInPoly *pixelInPoly;
						OGRLinearRing *ring;
						OGRPolygon *pixelPoly;
						OGRPolygon *ogrPoly;
						OGRGeometry *ogrGeom;
						
						pixelInPoly = new RSGISPixelInPoly(pixelPolyOption);
						
						ring = new OGRLinearRing();
						ring->addPoint(pxlTLX, pxlTLY, 0);
						ring->addPoint(pxlTLX + pxlWidth, pxlTLY, 0);
						ring->addPoint(pxlTLX + pxlWidth, pxlTLY - pxlHeight, 0);
						ring->addPoint(pxlTLX, pxlTLY - pxlHeight, 0);
						ring->addPoint(pxlTLX, pxlTLY, 0);
						
						pixelPoly = new OGRPolygon();
						pixelPoly->addRingDirectly(ring);
						
						ogrPoly = new OGRPolygon();
						const geos::geom::LineString *line = poly->getExteriorRing();
						OGRLinearRing *ogrRing = new OGRLinearRing();
						const geos::geom::CoordinateSequence *coords = line->getCoordinatesRO();
						int numCoords = coords->getSize();
						geos::geom::Coordinate coord;
						for(int i = 0; i < numCoords; i++)
						{
							coord = coords->getAt(i);
							ogrRing->addPoint(coord.x, coord.y, coord.z);
						}
						ogrPoly->addRing(ogrRing);
						ogrGeom = (OGRGeometry *) ogrPoly;
						
						// Check if the pixel should be classed as part of the polygon using the specified method
						if (pixelInPoly->findPixelInPoly(ogrGeom, pixelPoly)) 
						{
							this->calc->calcImageValue(inDataColumn, numInBands, extent);
						}
						
						// Tidy
						delete pixelInPoly;
						delete pixelPoly;
						delete ogrPoly;
					}
					
					delete pt;
					
					pxlTLX += pxlWidth;
				}
				pxlTLY -= pxlHeight;
				pxlTLX = gdalTranslation[0];
			}
		}
		catch(RSGISImageCalcException& e)
		{
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					if(dsOffsets[i] != NULL)
					{
						delete[] dsOffsets[i];
					}
				} 
				delete[] dsOffsets;
			}
			
			if(bandOffsets != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(bandOffsets[i] != NULL)
					{
						delete[] bandOffsets[i];
					}
				}
				delete[] bandOffsets;
			}			
			
			if(inputData != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(inputData[i] != NULL)
					{
						delete[] inputData[i];
					}
				}
				delete[] inputData;
			}
			
			if(inDataColumn != NULL)
			{
				delete[] inDataColumn;
			}
			
			if(inputRasterBands != NULL)
			{
				delete[] inputRasterBands;
			}
            
			throw e;
		}
		catch(RSGISImageBandException& e)
		{
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					if(dsOffsets[i] != NULL)
					{
						delete[] dsOffsets[i];
					}
				} 
				delete[] dsOffsets;
			}
			
			if(bandOffsets != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(bandOffsets[i] != NULL)
					{
						delete[] bandOffsets[i];
					}
				}
				delete[] bandOffsets;
			}			
			
			if(inputData != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(inputData[i] != NULL)
					{
						delete[] inputData[i];
					}
				}
				delete[] inputData;
			}
			
			if(inDataColumn != NULL)
			{
				delete[] inDataColumn;
			}
			
			if(inputRasterBands != NULL)
			{
				delete[] inputRasterBands;
			}
            
			throw e;
		}
        
		if(gdalTranslation != NULL)
		{
			delete[] gdalTranslation;
		}
		
		if(dsOffsets != NULL)
		{
			for(int i = 0; i < numDS; i++)
			{
				if(dsOffsets[i] != NULL)
				{
					delete[] dsOffsets[i];
				}
			} 
			delete[] dsOffsets;
		}
		
		if(bandOffsets != NULL)
		{
			for(int i = 0; i < numInBands; i++)
			{
				if(bandOffsets[i] != NULL)
				{
					delete[] bandOffsets[i];
				}
			}
			delete[] bandOffsets;
		}		
		
		if(inputData != NULL)
		{
			for(int i = 0; i < numInBands; i++)
			{
				if(inputData[i] != NULL)
				{
					delete[] inputData[i];
				}
			}
			delete[] inputData;
		}
		
		if(inDataColumn != NULL)
		{
			delete[] inDataColumn;
		}
		
		if(inputRasterBands != NULL)
		{
			delete[] inputRasterBands;
		}		
	}

    void RSGISCalcImage::calcImageWithinPolygonExtentInMem(GDALDataset **datasets, int numDS, geos::geom::Envelope *env, geos::geom::Polygon *poly, pixelInPolyOption pixelPolyOption)
    {
        GDALAllRegister();
        RSGISImageUtils imgUtils;
        double *gdalTranslation = new double[6];
        int **dsOffsets = new int*[numDS];
        for(int i = 0; i < numDS; i++)
        {
            dsOffsets[i] = new int[2];
        }
        int **bandOffsets = NULL;
        int height = 0;
        int width = 0;
        int numInBands = 0;

        float **inputData = NULL;
        float *inDataColumn = NULL;

        GDALRasterBand **inputRasterBands = NULL;
        geos::geom::Envelope extent;
        geos::geom::Coordinate pxlCentre;
        const geos::geom::GeometryFactory *geomFactory = geos::geom::GeometryFactory::getDefaultInstance();
        double pxlTLX = 0;
        double pxlTLY = 0;
        double pxlWidth = 0;
        double pxlHeight = 0;

        try
        {
            // CHECK ENVELOPE IS AT LEAST 1 x 1 Pixel
            /* For small polygons the the envelope can be smaller than a pixel, which will cause problems.
             * To avoid errors a buffer is applied to the envelope and this buffered envelope is used for 'getImageOverlap'
             * The buffered envelope is created and destroyed in this class and does not effect the passed in envelope
             */
            bool buffer = false;

            double *transformations = new double[6];
            datasets[0]->GetGeoTransform(transformations);

            // Get pixel size
            pxlWidth = transformations[1];
            pxlHeight = transformations[5];

            if(pxlHeight < 0)
            {
                pxlHeight = pxlHeight * (-1);
            }
            delete[] transformations;

            geos::geom::Envelope *bufferedEnvelope = NULL;

            if ((env->getWidth() < pxlWidth) | (env->getHeight() < pxlHeight))
            {
                buffer = true;
                bufferedEnvelope = new geos::geom::Envelope(env->getMinX() - pxlWidth, env->getMaxX() + pxlWidth, env->getMinY() - pxlHeight, env->getMaxY() + pxlHeight);
            }

            // Find image overlap
            gdalTranslation = new double[6];

            // Image block size.
            int xBlockSize = 0;
            int yBlockSize = 0;

            if (buffer) // Use buffered envelope.
            {
                //imgUtils.getImageOverlap(datasets, numDS, dsOffsets, &width, &height, gdalTranslation, bufferedEnvelope);
                imgUtils.getImageOverlapCut2Env(datasets, numDS, dsOffsets, &width, &height, gdalTranslation, bufferedEnvelope, &xBlockSize, &yBlockSize);
            }
            else // Use envelope passed in.
            {
                //imgUtils.getImageOverlap(datasets, numDS, dsOffsets, &width, &height, gdalTranslation, env);
                imgUtils.getImageOverlapCut2Env(datasets, numDS, dsOffsets, &width, &height, gdalTranslation, env, &xBlockSize, &yBlockSize);
            }

            // Count number of input image bands
            for(int i = 0; i < numDS; i++)
            {
                numInBands += datasets[i]->GetRasterCount();
            }

            // Get Pixel Size
            pxlTLX = gdalTranslation[0];
            pxlTLY = gdalTranslation[3];
            pxlWidth = gdalTranslation[1];
            pxlHeight = gdalTranslation[1];

            if(pxlHeight < 0) // Check resolution is positive (negative in Southern hemisphere).
            {
                pxlHeight = pxlHeight * (-1);
            }

            // Get Image Input Bands
            bandOffsets = new int*[numInBands];
            inputRasterBands = new GDALRasterBand*[numInBands];
            int counter = 0;
            for(int i = 0; i < numDS; i++)
            {
                for(int j = 0; j < datasets[i]->GetRasterCount(); j++)
                {
                    inputRasterBands[counter] = datasets[i]->GetRasterBand(j+1);
                    bandOffsets[counter] = new int[2];
                    bandOffsets[counter][0] = dsOffsets[i][0];
                    bandOffsets[counter][1] = dsOffsets[i][1];
                    counter++;
                }
            }
            // Allocate memory
            inputData = new float*[numInBands];
            for(int i = 0; i < numInBands; i++)
            {
                inputData[i] = (float *) CPLMalloc(sizeof(float)*width*height);
            }
            inDataColumn = new float[numInBands];

            bool readSuccess = true;
            for(int n = 0; n < numInBands; n++)
            {
                readSuccess = inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[n][0], (bandOffsets[n][1]), width, height, inputData[n], width, height, GDT_Float32, 0, 0);
            }

            // Loop images to process data
            for(int i = 0; i < height; i++)
            {
                for(int j = 0; j < width; j++)
                {
                    for(int n = 0; n < numInBands; n++)
                    {
                        inDataColumn[n] = inputData[n][(i*width)+j];
                    }

                    geos::geom::Coordinate pxlCentre;
                    geos::geom::Point *pt = NULL;

                    extent.init(pxlTLX, (pxlTLX+pxlWidth), pxlTLY, (pxlTLY-pxlHeight));
                    extent.centre(pxlCentre);
                    pt = geomFactory->createPoint(pxlCentre);

                    if (pixelPolyOption == polyContainsPixelCenter)
                    {
                        if(poly->contains(pt)) // If polygon contains pixel center
                        {
                            this->calc->calcImageValue(inDataColumn, numInBands, extent);
                        }
                    }
                    else if (pixelPolyOption == pixelAreaInPoly)
                    {
                        geos::geom::CoordinateSequence *coords = NULL;
                        geos::geom::LinearRing *ring = NULL;
                        geos::geom::Polygon *pixelGeosPoly = NULL;
                        geos::geom::Geometry *intersectionGeom;

                        coords = new geos::geom::CoordinateArraySequence();
                        coords->add(geos::geom::Coordinate(pxlTLX, pxlTLY, 0));
                        coords->add(geos::geom::Coordinate(pxlTLX + pxlWidth, pxlTLY, 0));
                        coords->add(geos::geom::Coordinate(pxlTLX + pxlWidth, pxlTLY - pxlHeight, 0));
                        coords->add(geos::geom::Coordinate(pxlTLX, pxlTLY - pxlHeight, 0));
                        coords->add(geos::geom::Coordinate(pxlTLX, pxlTLY, 0));
                        ring = geomFactory->createLinearRing(coords);
                        pixelGeosPoly = geomFactory->createPolygon(ring, NULL);


                        intersectionGeom = pixelGeosPoly->intersection(poly);
                        double intersectionArea = intersectionGeom->getArea()  / pixelGeosPoly->getArea();

                        if(intersectionArea > 0)
                        {
                            this->calc->calcImageValue(inDataColumn, numInBands, extent);
                        }
                    }
                    else
                    {
                        RSGISPixelInPoly *pixelInPoly;
                        OGRLinearRing *ring;
                        OGRPolygon *pixelPoly;
                        OGRPolygon *ogrPoly;
                        OGRGeometry *ogrGeom;

                        pixelInPoly = new RSGISPixelInPoly(pixelPolyOption);

                        ring = new OGRLinearRing();
                        ring->addPoint(pxlTLX, pxlTLY, 0);
                        ring->addPoint(pxlTLX + pxlWidth, pxlTLY, 0);
                        ring->addPoint(pxlTLX + pxlWidth, pxlTLY - pxlHeight, 0);
                        ring->addPoint(pxlTLX, pxlTLY - pxlHeight, 0);
                        ring->addPoint(pxlTLX, pxlTLY, 0);

                        pixelPoly = new OGRPolygon();
                        pixelPoly->addRingDirectly(ring);

                        ogrPoly = new OGRPolygon();
                        const geos::geom::LineString *line = poly->getExteriorRing();
                        OGRLinearRing *ogrRing = new OGRLinearRing();
                        const geos::geom::CoordinateSequence *coords = line->getCoordinatesRO();
                        int numCoords = coords->getSize();
                        geos::geom::Coordinate coord;
                        for(int i = 0; i < numCoords; i++)
                        {
                            coord = coords->getAt(i);
                            ogrRing->addPoint(coord.x, coord.y, coord.z);
                        }
                        ogrPoly->addRing(ogrRing);
                        ogrGeom = (OGRGeometry *) ogrPoly;

                        // Check if the pixel should be classed as part of the polygon using the specified method
                        if (pixelInPoly->findPixelInPoly(ogrGeom, pixelPoly))
                        {
                            this->calc->calcImageValue(inDataColumn, numInBands, extent);
                        }

                        // Tidy
                        delete pixelInPoly;
                        delete pixelPoly;
                        delete ogrPoly;
                    }

                    delete pt;

                    pxlTLX += pxlWidth;
                }
                pxlTLY -= pxlHeight;
                pxlTLX = gdalTranslation[0];
            }
        }
        catch(RSGISImageCalcException& e)
        {
            if(gdalTranslation != NULL)
            {
                delete[] gdalTranslation;
            }

            if(dsOffsets != NULL)
            {
                for(int i = 0; i < numDS; i++)
                {
                    if(dsOffsets[i] != NULL)
                    {
                        delete[] dsOffsets[i];
                    }
                }
                delete[] dsOffsets;
            }

            if(bandOffsets != NULL)
            {
                for(int i = 0; i < numInBands; i++)
                {
                    if(bandOffsets[i] != NULL)
                    {
                        delete[] bandOffsets[i];
                    }
                }
                delete[] bandOffsets;
            }

            if(inputData != NULL)
            {
                for(int i = 0; i < numInBands; i++)
                {
                    if(inputData[i] != NULL)
                    {
                        delete[] inputData[i];
                    }
                }
                delete[] inputData;
            }

            if(inDataColumn != NULL)
            {
                delete[] inDataColumn;
            }

            if(inputRasterBands != NULL)
            {
                delete[] inputRasterBands;
            }

            throw e;
        }
        catch(RSGISImageBandException& e)
        {
            if(gdalTranslation != NULL)
            {
                delete[] gdalTranslation;
            }

            if(dsOffsets != NULL)
            {
                for(int i = 0; i < numDS; i++)
                {
                    if(dsOffsets[i] != NULL)
                    {
                        delete[] dsOffsets[i];
                    }
                }
                delete[] dsOffsets;
            }

            if(bandOffsets != NULL)
            {
                for(int i = 0; i < numInBands; i++)
                {
                    if(bandOffsets[i] != NULL)
                    {
                        delete[] bandOffsets[i];
                    }
                }
                delete[] bandOffsets;
            }

            if(inputData != NULL)
            {
                for(int i = 0; i < numInBands; i++)
                {
                    if(inputData[i] != NULL)
                    {
                        delete[] inputData[i];
                    }
                }
                delete[] inputData;
            }

            if(inDataColumn != NULL)
            {
                delete[] inDataColumn;
            }

            if(inputRasterBands != NULL)
            {
                delete[] inputRasterBands;
            }

            throw e;
        }

        if(gdalTranslation != NULL)
        {
            delete[] gdalTranslation;
        }

        if(dsOffsets != NULL)
        {
            for(int i = 0; i < numDS; i++)
            {
                if(dsOffsets[i] != NULL)
                {
                    delete[] dsOffsets[i];
                }
            }
            delete[] dsOffsets;
        }

        if(bandOffsets != NULL)
        {
            for(int i = 0; i < numInBands; i++)
            {
                if(bandOffsets[i] != NULL)
                {
                    delete[] bandOffsets[i];
                }
            }
            delete[] bandOffsets;
        }

        if(inputData != NULL)
        {
            for(int i = 0; i < numInBands; i++)
            {
                if(inputData[i] != NULL)
                {
                    delete[] inputData[i];
                }
            }
            delete[] inputData;
        }

        if(inDataColumn != NULL)
        {
            delete[] inDataColumn;
        }

        if(inputRasterBands != NULL)
        {
            delete[] inputRasterBands;
        }
    }
	
	/* calcImageWithinRasterPolygon - takes existing output image */
	void RSGISCalcImage::calcImageWithinRasterPolygon(GDALDataset **datasets, int numDS, geos::geom::Envelope *env, long fid)
	{
		/** Input and output images as gdal datasets
		 * Stored as:
		 * 1) RasterisedVector DS
		 * 2) Input DS
		 * 3) Output DS
		 * numDS = numinput + 2 (mask + output band)
		 */
		
		GDALAllRegister();
		RSGISImageUtils imgUtils;
		double *gdalTranslation = new double[6];
		int **dsOffsets = new int*[numDS];
		for(int i = 0; i < numDS; i++)
		{
			dsOffsets[i] = new int[2];
		}
		int **bandOffsets = NULL;
		int height = 0;
		int width = 0;
		int numInBands = 0;
		
		float **inputMask = NULL;
		float **inputData = NULL;
		double **outputData = NULL;
		float *inDataColumn = NULL;
		double *outDataColumn = NULL;
		
		GDALRasterBand **inputRasterBands = NULL;
		GDALRasterBand **outputRasterBands = NULL;
		GDALRasterBand **inputMaskBand = NULL;

		geos::geom::Envelope extent;
		geos::geom::Coordinate pxlCentre;
		double pxlTLX = 0;
		double pxlTLY = 0;
		double pxlWidth = 0;
		double pxlHeight = 0;
		
		try
		{
			// Find image overlap
			imgUtils.getImageOverlap(datasets, numDS, dsOffsets, &width, &height, gdalTranslation, env);
			
			// Count number of input image bands
			for(int i = 1; i < numDS - 1; i++)
			{
				numInBands += datasets[i]->GetRasterCount();
			}
			
			// Count number of output image bands
			this->numOutBands = datasets[numDS-1]->GetRasterCount();
			
			// Get Pixel Size
			pxlTLX = gdalTranslation[0];
			pxlTLY = gdalTranslation[3];
			pxlWidth = gdalTranslation[1];
			pxlHeight = gdalTranslation[1];
			
			if(pxlHeight < 0) // Check resolution is positive (negative in Southern hemisphere).
			{
				pxlHeight = pxlHeight * (-1);
			}
			
			int counter = 0;
			bandOffsets = new int*[numInBands+numOutBands+1];
			
			// Get Mask Band
			inputMaskBand = new GDALRasterBand*[1];
			inputMaskBand[counter] = datasets[0]->GetRasterBand(1);
			bandOffsets[counter] = new int[2];
			bandOffsets[counter][0] = dsOffsets[0][0];
			bandOffsets[counter][1] = dsOffsets[0][1];
			counter++;
			
			// Get Image Input Bands
			inputRasterBands = new GDALRasterBand*[numInBands];
			for(int i = 1; i < numDS - 1; i++)
			{
				for(int j = 0; j < datasets[i]->GetRasterCount(); j++)
				{
					bandOffsets[counter] = new int[2];
					inputRasterBands[j] = datasets[i]->GetRasterBand(j+1);
					bandOffsets[counter][0] = dsOffsets[i][0];
					bandOffsets[counter][1] = dsOffsets[i][1];
					counter++;
				}
			}
			
			// Get Image Output Bands
			outputRasterBands = new GDALRasterBand*[this->numOutBands];
			for(int j = 0; j < datasets[numDS-1]->GetRasterCount(); j++)
			{
				bandOffsets[counter] = new int[2];
				outputRasterBands[j] = datasets[numDS-1]->GetRasterBand(j+1);
				bandOffsets[counter][0] = dsOffsets[numDS-1][0];
				bandOffsets[counter][1] = dsOffsets[numDS-1][1];
				counter++;
			}
			
			// Allocate memory
			inputMask = new float*[1];
			inputMask[0] = (float *) CPLMalloc(sizeof(float)*width);
			
			inputData = new float*[numInBands];
			for(int i = 0; i < numInBands; i++)
			{
				inputData[i] = (float *) CPLMalloc(sizeof(float)*width);
			}
			inDataColumn = new float[numInBands];
			
			outputData = new double*[this->numOutBands];
			for(int i = 0; i < this->numOutBands; i++)
			{
				outputData[i] = (double *) CPLMalloc(sizeof(double)*width);
			}
			outDataColumn = new double[this->numOutBands];
			
			int feedback = height/10;
			if (feedback == 0) {feedback = 1;} // Set feedback to 1
			int feedbackCounter = 0;
			if(height > 100)
			{
				std::cout << "\rStarted (Object:" << fid << ")..";
			}	
			// Loop images to process data
			for(int i = 0; i < height; i++)
			{				
				if((feedback != 0) && ((i % feedback) == 0))
				{
					std::cout << "." << feedbackCounter << "." << std::flush;
					feedbackCounter = feedbackCounter + 10;
				}
				counter = 0;
				inputMaskBand[0]->RasterIO(GF_Read, bandOffsets[counter][0], (bandOffsets[counter][1]+i), width, 1, inputMask[0], width, 1, GDT_Float32, 0, 0);
				counter++;
				for(int n = 0; n < numInBands; n++)
				{
					inputRasterBands[n]->RasterIO(GF_Read, bandOffsets[counter][0], (bandOffsets[counter][1]+i), width, 1, inputData[n], width, 1, GDT_Float32, 0, 0);
					counter++;
				}
				for(int n = 0; n < this->numOutBands; n++)
				{
					outputRasterBands[n]->RasterIO(GF_Read, bandOffsets[counter][0], (bandOffsets[counter][1]+i), width, 1, outputData[n], width, 1, GDT_Float32, 0, 0);
					counter++;
				}
				
				for(int j = 0; j < width; j++)
				{
					for(int n = 0; n < numInBands; n++)
					{
						inDataColumn[n] = inputData[n][j];
					}
					
					if (inputMask[0][j] == fid) 
					{
						this->calc->calcImageValue(inDataColumn, numInBands, outDataColumn);
					}
					else 
					{
						for (int n = 0; n < this->numOutBands; n++) 
						{
							outDataColumn[n] = outputData[n][j];
						}
					}
					
					for(int n = 0; n < this->numOutBands; n++)
					{
						outputData[n][j] = outDataColumn[n];
					}
					
					pxlTLX += pxlWidth;
					
				}
				pxlTLY -= pxlHeight;
				pxlTLX = gdalTranslation[0];
				
				for(int n = 0; n < this->numOutBands; n++)
				{
					outputRasterBands[n]->RasterIO(GF_Write, bandOffsets[numInBands+1+n][0], (bandOffsets[numInBands+1+n][1]+i), width, 1, outputData[n], width, 1, GDT_Float32, 0, 0);
				}
			}
			if (height > 100) 
			{
				std::cout << "Complete" << std::flush;
				std::cout << "\r                                                                                                                            \r" << std::flush;
			}
		}
		catch(RSGISImageCalcException& e)
		{
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					if(dsOffsets[i] != NULL)
					{
						delete[] dsOffsets[i];
					}
				} 
				delete[] dsOffsets;
			}
			
			if(bandOffsets != NULL)
			{
				for(int i = 0; i < numInBands+numOutBands+1; i++)
				{
					if(bandOffsets[i] != NULL)
					{
						delete[] bandOffsets[i];
					}
				}
				delete[] bandOffsets;
			}			
			
			if(inputData != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(inputData[i] != NULL)
					{
						delete[] inputData[i];
					}
				}
				delete[] inputData;
			}
			
			if(outputData != NULL)
			{
				for(int i = 0; i < this->numOutBands; i++)
				{
					if(outputData[i] != NULL)
					{
						delete[] outputData[i];
					}
				}
				delete[] outputData;
			}
			
			if(inputMask != NULL)
			{
				delete[] inputMask[0];
				delete[] inputMask;
			}
			
			if(inDataColumn != NULL)
			{
				delete[] inDataColumn;
			}
			
			if(outDataColumn != NULL)
			{
				delete[] outDataColumn;
			}
			
			if(inputMaskBand != NULL)
			{
				delete[] inputMaskBand;
			}
			
			if(inputRasterBands != NULL)
			{
				delete[] inputRasterBands;
			}
			
			if(outputRasterBands != NULL)
			{
				delete[] outputRasterBands;
			}
			throw e;
		}
		catch(RSGISImageBandException& e)
		{
			if(gdalTranslation != NULL)
			{
				delete[] gdalTranslation;
			}
			
			if(dsOffsets != NULL)
			{
				for(int i = 0; i < numDS; i++)
				{
					if(dsOffsets[i] != NULL)
					{
						delete[] dsOffsets[i];
					}
				} 
				delete[] dsOffsets;
			}
			
			if(bandOffsets != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(bandOffsets[i] != NULL)
					{
						delete[] bandOffsets[i];
					}
				}
				delete[] bandOffsets;
			}			
			
			if(inputData != NULL)
			{
				for(int i = 0; i < numInBands; i++)
				{
					if(inputData[i] != NULL)
					{
						delete[] inputData[i];
					}
				}
				delete[] inputData;
			}
			
			if(outputData != NULL)
			{
				for(int i = 0; i < this->numOutBands; i++)
				{
					if(outputData[i] != NULL)
					{
						delete[] outputData[i];
					}
				}
				delete[] outputData;
			}
			
			if(inDataColumn != NULL)
			{
				delete[] inDataColumn;
			}
			
			if(outDataColumn != NULL)
			{
				delete[] outDataColumn;
			}
			
			if(inputRasterBands != NULL)
			{
				delete[] inputRasterBands;
			}
			
			if(outputRasterBands != NULL)
			{
				delete[] outputRasterBands;
			}
			throw e;
		}
		
		if(gdalTranslation != NULL)
		{
			delete[] gdalTranslation;
		}
		
		if(dsOffsets != NULL)
		{
			for(int i = 0; i < numDS; i++)
			{
				if(dsOffsets[i] != NULL)
				{
					delete[] dsOffsets[i];
				}
			} 
			delete[] dsOffsets;
		}
		
		if(bandOffsets != NULL)
		{
			for(int i = 0; i < numInBands; i++)
			{
				if(bandOffsets[i] != NULL)
				{
					delete[] bandOffsets[i];
				}
			}
			delete[] bandOffsets;
		}		
		
		if(inputData != NULL)
		{
			for(int i = 0; i < numInBands; i++)
			{
				if(inputData[i] != NULL)
				{
					delete[] inputData[i];
				}
			}
			delete[] inputData;
		}
		
		if(outputData != NULL)
		{
			for(int i = 0; i < this->numOutBands; i++)
			{
				if(outputData[i] != NULL)
				{
					delete[] outputData[i];
				}
			}
			delete[] outputData;
		}
		
		if(inDataColumn != NULL)
		{
			delete[] inDataColumn;
		}
		
		if(outDataColumn != NULL)
		{
			delete[] outDataColumn;
		}
		
		if(inputRasterBands != NULL)
		{
			delete[] inputRasterBands;
		}
		
		if(outputRasterBands != NULL)
		{
			delete[] outputRasterBands;
		}		
	}
	
    void RSGISCalcImage::calcImageBorderPixels(GDALDataset *dataset, bool returnInt)
    {
        GDALAllRegister();
        
        try
        {
            unsigned int imgWidth = dataset->GetRasterXSize();
            unsigned int imgHeight = dataset->GetRasterYSize();
            unsigned int numBands = dataset->GetRasterCount();
            
            GDALRasterBand **gdalBands = new GDALRasterBand*[numBands];
            for(unsigned int i = 0; i < numBands; ++i)
            {
                gdalBands[i] = dataset->GetRasterBand(i+1);
            }
            
            unsigned int numfloatVals = 0;
            float *pxlFloatVals = NULL;
            unsigned int numIntVals = 0;
            long *pxlIntVals = NULL;
            if(returnInt)
            {
                numIntVals = numBands;
                pxlIntVals = new long[numIntVals];
            }
            else
            {
                numfloatVals = numBands;
                pxlFloatVals = new float[numfloatVals];
            }
            
            int tmpVal = 0;
            
            std::cout << "Processing Top and Bottom Pixels\n";
            // Top and Bottom pixels
            for(unsigned int x = 0; x < imgWidth; ++x)
            {
                // Top
                for(unsigned int b = 0; b < numBands; ++b)
                {
                    if(returnInt)
                    {
                        gdalBands[b]->RasterIO(GF_Read, x, 0, 1, 1, &tmpVal, 1, 1, GDT_Int32, 0, 0);
                        pxlIntVals[b] = tmpVal;
                    }
                    else
                    {
                        gdalBands[b]->RasterIO(GF_Read, x, 0, 1, 1, &pxlFloatVals[b], 1, 1, GDT_Float32, 0, 0);
                    }
                }
                this->calc->calcImageValue(pxlIntVals, numIntVals, pxlFloatVals, numfloatVals);
                
                // Bottom
                for(unsigned int b = 0; b < numBands; ++b)
                {
                    if(returnInt)
                    {
                        gdalBands[b]->RasterIO(GF_Read, x, (imgHeight-1), 1, 1, &tmpVal, 1, 1, GDT_Int32, 0, 0);
                        pxlIntVals[b] = tmpVal;
                    }
                    else
                    {
                        gdalBands[b]->RasterIO(GF_Read, x, (imgHeight-1), 1, 1, &pxlFloatVals[b], 1, 1, GDT_Float32, 0, 0);
                    }
                }
                this->calc->calcImageValue(pxlIntVals, numIntVals, pxlFloatVals, numfloatVals);
            }
            
            std::cout << "Processing Left and Right Pixels\n";
            // Left and right pixels
            for(unsigned int y = 0; y < imgHeight; ++y)
            {
                // Left
                for(unsigned int b = 0; b < numBands; ++b)
                {
                    if(returnInt)
                    {
                        gdalBands[b]->RasterIO(GF_Read, 0, y, 1, 1, &pxlIntVals[b], 1, 1, GDT_Int32, 0, 0);
                    }
                    else
                    {
                        gdalBands[b]->RasterIO(GF_Read, 0, y, 1, 1, &pxlFloatVals[b], 1, 1, GDT_Float32, 0, 0);
                    }
                }
                this->calc->calcImageValue(pxlIntVals, numIntVals, pxlFloatVals, numfloatVals);
                
                // Right
                for(unsigned int b = 0; b < numBands; ++b)
                {
                    if(returnInt)
                    {
                        gdalBands[b]->RasterIO(GF_Read, (imgWidth-1), y, 1, 1, &pxlIntVals[b], 1, 1, GDT_Int32, 0, 0);
                    }
                    else
                    {
                        gdalBands[b]->RasterIO(GF_Read, (imgWidth-1), y, 1, 1, &pxlFloatVals[b], 1, 1, GDT_Float32, 0, 0);
                    }
                }
                this->calc->calcImageValue(pxlIntVals, numIntVals, pxlFloatVals, numfloatVals);
            }
            
            delete[] gdalBands;
            delete[] pxlFloatVals;
            delete[] pxlIntVals;
        }
        catch (RSGISImageCalcException &e)
        {
            throw e;
        }
        catch (RSGISImageBandException &e)
        {
            throw e;
        }
        catch (RSGISException &e)
        {
            throw RSGISImageCalcException(e.what());
        }
    }
    
	RSGISCalcImage::~RSGISCalcImage()
	{
		
	}
    
    
    
    RSGISCalcImageMultiImgRes::RSGISCalcImageMultiImgRes(RSGISCalcValuesFromMultiResInputs *valueCalcSum)
    {
        this->valueCalcSum = valueCalcSum;
    }
    
    void RSGISCalcImageMultiImgRes::calcImageHighResForLowRegions(GDALDataset *refDataset, GDALDataset *statsDataset, unsigned int statsImgBand, std::string outputImage, std::string gdalFormat, GDALDataType gdalDataType, bool useNoDataVal, unsigned int xIOGrid, unsigned int yIOGrid, bool setOutNames, std::string *bandNames)
    {
        try
        {
            if( (statsImgBand == 0) || (statsImgBand > statsDataset->GetRasterCount()) )
            {
                throw RSGISImageException("The image band specified for the stats image is not within the image. Don't forget, band numbering starts at 1.");
            }
            
            double *refImgTrans = new double[6];
            refDataset->GetGeoTransform(refImgTrans);
            double *statsImgTrans = new double[6];
            statsDataset->GetGeoTransform(statsImgTrans);
            
            // Check stats image has a resolution which is a multiple of the ref image resolution.
            if(refImgTrans[1] <= statsImgTrans[1])
            {
                throw RSGISImageException("Reference image is less than or equal resolution to stats image in X axis.");
            }
            else if(fmod(refImgTrans[1], statsImgTrans[1]) != 0)
            {
                throw RSGISImageException("Stats image resolution is not a multiple  less than or equal resolution to stats image in X axis.");
            }
            double xRefRes = refImgTrans[1];
            double xStatsRes = statsImgTrans[1];
            unsigned int nXPxls = ceil(xRefRes/xStatsRes);
            
            refImgTrans[5] = std::fabs(refImgTrans[5]);
            statsImgTrans[5] = std::fabs(statsImgTrans[5]);
            
            if(refImgTrans[5] <= statsImgTrans[5])
            {
                throw RSGISImageException("Reference image is less than or equal resolution to stats image in Y axis.");
            }
            else if(fmod(refImgTrans[5], statsImgTrans[5]) != 0)
            {
                throw RSGISImageException("Stats image resolution is not a multiple  less than or equal resolution to stats image in Y axis.");
            }
            double yRefRes = refImgTrans[5];
            double yStatsRes = statsImgTrans[5];
            unsigned int nYPxls = ceil(yRefRes/yStatsRes);
            
            std::cout << "There are " << nXPxls << " by " << nYPxls << " pixels within a single reference pixel\n";
            
            // Check rotation
            if(refImgTrans[2] != statsImgTrans[2])
            {
                throw RSGISImageException("X rotation is not equilvent between the two images.");
            }
            
            if(refImgTrans[4] != statsImgTrans[4])
            {
                throw RSGISImageException("Y rotation is not equilvent between the two images.");
            }
            
            // Get image size in pixels.
            unsigned int refImgXPxls = refDataset->GetRasterXSize();
            unsigned int refImgYPxls = refDataset->GetRasterYSize();
            
            unsigned int statsImgXPxls = statsDataset->GetRasterXSize();
            unsigned int statsImgYPxls = statsDataset->GetRasterYSize();
            
            // Get image bounds.
            double refImgXMin = refImgTrans[0];
            double refImgXMax = refImgTrans[0] + (xRefRes * refImgXPxls);
            double refImgYMin = refImgTrans[3] - (yRefRes * refImgYPxls);
            double refImgYMax = refImgTrans[3];
            
            double statsImgXMin = statsImgTrans[0];
            double statsImgXMax = statsImgTrans[0] + (xStatsRes * statsImgXPxls);
            double statsImgYMin = statsImgTrans[3] - (yStatsRes * statsImgYPxls);
            double statsImgYMax = statsImgTrans[3];
            
            // Check whether images overlap.
            if((refImgXMin > statsImgXMax) | (statsImgXMin > refImgXMax))
            {
                throw RSGISImageException("Images do not overlap in the X axis.");
            }
            
            if((refImgYMin > statsImgYMax) | (statsImgYMin > refImgYMax))
            {
                throw RSGISImageException("Images do not overlap in the Y axis.");
            }
            
            // Find overlap starting points and pixel widths.
            double xMinOverlap = refImgXMin;
            double xMaxOverlap = refImgXMax;
            double yMinOverlap = refImgYMin;
            double yMaxOverlap = refImgYMax;
            
            if(statsImgXMin > xMinOverlap)
            {
                double diff = ceil((statsImgXMin - xMinOverlap)/xRefRes)*xRefRes;
                xMinOverlap = xMinOverlap + diff;
            }
            
            if(statsImgXMax < xMaxOverlap)
            {
                double diff = ceil((xMaxOverlap - statsImgXMax)/xRefRes)*xRefRes;
                xMaxOverlap = xMaxOverlap - diff;
            }
            
            if(statsImgYMin > yMinOverlap)
            {
                double diff = ceil((statsImgYMin - yMinOverlap)/yRefRes)*yRefRes;
                yMinOverlap = yMinOverlap + diff;
            }
            
            if(statsImgYMax < yMaxOverlap)
            {
                double diff = ceil((yMaxOverlap - statsImgYMax)/yRefRes)*yRefRes;
                yMaxOverlap = yMaxOverlap - diff;
            }
            
            long refPxlWidth = floor((xMaxOverlap - xMinOverlap)/xRefRes);
            long refPxlHeight = floor((yMaxOverlap - yMinOverlap)/yRefRes);
            
            // Define the number of blocks in the X and Y needed to tranverse the image.
            long nXBlocks = refPxlWidth / xIOGrid;
            long remainCols = refPxlWidth - (nXBlocks * xIOGrid);
            long remainColsStats = (remainCols * nXPxls);
            
            long nYBlocks = refPxlHeight / yIOGrid;
            long remainRows = refPxlHeight - (nYBlocks * yIOGrid);
            long remainRowsStats = (remainRows * nYPxls);
            
            // Get Input Stats image band.
            GDALRasterBand *statsBand = statsDataset->GetRasterBand(statsImgBand);
            double noDataVal = 0.0;
            if(useNoDataVal)
            {
                noDataVal = statsBand->GetNoDataValue();
            }
            
            // Create the output image file
            int numOutImgBands = this->valueCalcSum->getNumOutBands();
            GDALDriver *gdalDriver = GetGDALDriverManager()->GetDriverByName(gdalFormat.c_str());
            if(gdalDriver == NULL)
            {
                throw RSGISImageException("Requested GDAL driver does not exists..");
            }
            std::cout << "New image width = " << refPxlWidth << " height = " << refPxlHeight << " bands = " << numOutImgBands << std::endl;
            
            GDALDataset *outputImageDS = gdalDriver->Create(outputImage.c_str(), refPxlWidth, refPxlHeight, numOutImgBands, gdalDataType, NULL);
            
            if(outputImageDS == NULL)
            {
                throw RSGISImageException("Output image could not be created. Check filepath.");
            }
            double *outImgTrans = new double[6];
            outImgTrans[0] = xMinOverlap;
            outImgTrans[1] = refImgTrans[1];
            outImgTrans[2] = refImgTrans[2];
            outImgTrans[3] = yMaxOverlap;
            outImgTrans[4] = refImgTrans[4];
            if(refImgTrans[5] > 0)
            {
                refImgTrans[5] = refImgTrans[5]*(-1);
            }
            outImgTrans[5] = refImgTrans[5];
            outputImageDS->SetGeoTransform(outImgTrans);
            outputImageDS->SetProjection(refDataset->GetProjectionRef());
            
            // Block width in stats image pixels
            long xIOGridStats = nXPxls * xIOGrid;
            long yIOGridStats = nYPxls * yIOGrid;
            long nStatsPixelsInRefPxl = nXPxls * nYPxls;
            
            // Create Data Arrays and open output bands
            unsigned long numRefPxlsInBlock = xIOGrid * yIOGrid;
            double **refDataArrOuts = new double*[numOutImgBands];
            GDALRasterBand **outBands = new GDALRasterBand*[numOutImgBands];
            for(unsigned int i = 0; i < numOutImgBands; ++i)
            {
                outBands[i] = outputImageDS->GetRasterBand(i+1);
                refDataArrOuts[i] = (double *) CPLMalloc(sizeof(double)*numRefPxlsInBlock);
            }
            
            unsigned long numStatsPxlsInBlock = xIOGridStats * yIOGridStats;
            float *statsDataArr = (float *) CPLMalloc(sizeof(float)*numStatsPxlsInBlock);
            
            float *statsPxlsInRefPxl = new float[nStatsPixelsInRefPxl];
            double *outImgBandVals = new double[numOutImgBands];
            
            // Sort out stats for user feedback
            long nBlocks = nXBlocks * nYBlocks;
            if(remainCols > 0)
            {
                nBlocks += nYBlocks;
            }
            if(remainRows > 0)
            {
                nBlocks += nXBlocks;
                if(remainCols > 0)
                {
                    nBlocks += 1;
                }
            }
            
            int rowOffsetRef = 0;
            int colOffsetRef = 0;
            int rowOffsetStats = 0;
            int colOffsetStats = 0;
            
            int ib_rowOffStats = 0;
            int ib_colOffStats = 0;
            
            long feedback = nBlocks/10;
            long blockCounter = 0;
            int feedbackCounter = 0;
            std::cout << "Started " << std::flush;
            for(long i = 0; i < nYBlocks; i++)
            {
                colOffsetStats = 0;
                colOffsetRef = 0;
                for(long j = 0; j < nXBlocks; j++)
                {
                    if((feedback != 0) && ((blockCounter % feedback) == 0))
                    {
                        std::cout << "." << feedbackCounter << "." << std::flush;
                        feedbackCounter = feedbackCounter + 10;
                    }
                    
                    // Read Block
                    if(statsBand->RasterIO(GF_Read, colOffsetStats, rowOffsetStats, xIOGridStats, yIOGridStats, statsDataArr, xIOGridStats, yIOGridStats, GDT_Float32, 0, 0))
                    {
                        throw RSGISImageException("Failed to read image data from stats band.");
                    }
                    
                    // Process Block
                    ib_rowOffStats = 0;
                    ib_colOffStats = 0;
                    for(int n = 0; n < yIOGrid; ++n)
                    {
                        ib_colOffStats = 0;
                        for(int m = 0; m < xIOGrid; ++m)
                        {
                            for(int y = 0; y < nYPxls; ++y)
                            {
                                for(int x = 0; x < nXPxls; ++x)
                                {
                                    statsPxlsInRefPxl[(y*nYPxls)+x] = statsDataArr[(ib_colOffStats+x)+(ib_rowOffStats+(yIOGridStats*y))];
                                }
                            }
                            
                            valueCalcSum->calcImageValue(statsPxlsInRefPxl, nStatsPixelsInRefPxl, useNoDataVal, noDataVal, outImgBandVals);
                            for(unsigned int b = 0; b < numOutImgBands; ++b)
                            {
                                refDataArrOuts[b][((n*xIOGrid)+m)] = outImgBandVals[b];
                            }
                            ib_colOffStats += nXPxls;
                        }
                        ib_rowOffStats += (nYPxls * (xIOGrid * nXPxls));
                    }
                    
                    // Write Block
                    for(unsigned int n = 0; n < numOutImgBands; ++n)
                    {
                        if(outBands[n]->RasterIO(GF_Write, colOffsetRef, rowOffsetRef, xIOGrid, yIOGrid, refDataArrOuts[n], xIOGrid, yIOGrid, GDT_Float64, 0, 0))
                        {
                            throw RSGISImageException("Failed to write image data to output image.");
                        }
                    }
                    
                    ++blockCounter;
                    colOffsetStats += xIOGridStats;
                    colOffsetRef += xIOGrid;
                }
                
                if(remainCols > 0)
                {
                    if((feedback != 0) && ((blockCounter % feedback) == 0))
                    {
                        std::cout << "." << feedbackCounter << "." << std::flush;
                        feedbackCounter = feedbackCounter + 10;
                    }
                    
                    // Read Block
                    if(statsBand->RasterIO(GF_Read, colOffsetStats, rowOffsetStats, remainColsStats, yIOGridStats, statsDataArr, remainColsStats, yIOGridStats, GDT_Float32, 0, 0))
                    {
                        throw RSGISImageException("Failed to read image data from stats band.");
                    }
                    
                    // Process Block
                    ib_rowOffStats = 0;
                    ib_colOffStats = 0;
                    for(int n = 0; n < yIOGrid; ++n)
                    {
                        ib_colOffStats = 0;
                        for(int m = 0; m < remainCols; ++m)
                        {
                            for(int y = 0; y < nYPxls; ++y)
                            {
                                for(int x = 0; x < nXPxls; ++x)
                                {
                                    statsPxlsInRefPxl[(y*nYPxls)+x] = statsDataArr[(ib_colOffStats+x)+(ib_rowOffStats+(yIOGridStats*y))];
                                }
                            }
                            
                            valueCalcSum->calcImageValue(statsPxlsInRefPxl, nStatsPixelsInRefPxl, useNoDataVal, noDataVal, outImgBandVals);
                            for(unsigned int b = 0; b < numOutImgBands; ++b)
                            {
                                refDataArrOuts[b][((n*remainCols)+m)] = outImgBandVals[b];
                            }
                            ib_colOffStats += nXPxls;
                        }
                        ib_rowOffStats += (nYPxls * (remainCols * nXPxls));
                    }
                    
                    // Write Block
                    for(unsigned int n = 0; n < numOutImgBands; ++n)
                    {
                        if(outBands[n]->RasterIO(GF_Write, colOffsetRef, rowOffsetRef, remainCols, yIOGrid, refDataArrOuts[n], remainCols, yIOGrid, GDT_Float64, 0, 0))
                        {
                            throw RSGISImageException("Failed to write image data to output image.");
                        }
                    }
                    
                    ++blockCounter;
                    colOffsetStats += remainColsStats;
                    colOffsetRef += remainCols;
                }
                
                rowOffsetStats += yIOGridStats;
                rowOffsetRef += yIOGrid;
            }
            
            if(remainRows > 0)
            {
                colOffsetStats = 0;
                colOffsetRef = 0;
                for(long j = 0; j < nXBlocks; j++)
                {
                    if((feedback != 0) && ((blockCounter % feedback) == 0))
                    {
                        std::cout << "." << feedbackCounter << "." << std::flush;
                        feedbackCounter = feedbackCounter + 10;
                    }
                    
                    // Read Block
                    if(statsBand->RasterIO(GF_Read, colOffsetStats, rowOffsetStats, xIOGridStats, remainRowsStats, statsDataArr, xIOGridStats, remainRowsStats, GDT_Float32, 0, 0))
                    {
                        throw RSGISImageException("Failed to read image data from stats band.");
                    }
                    
                    // Process Block
                    ib_rowOffStats = 0;
                    ib_colOffStats = 0;
                    for(int n = 0; n < remainRows; ++n)
                    {
                        ib_colOffStats = 0;
                        for(int m = 0; m < xIOGrid; ++m)
                        {
                            for(int y = 0; y < nYPxls; ++y)
                            {
                                for(int x = 0; x < nXPxls; ++x)
                                {
                                    statsPxlsInRefPxl[(y*nYPxls)+x] = statsDataArr[(ib_colOffStats+x)+(ib_rowOffStats+((remainRows*nYPxls)*y))];
                                }
                            }
                            
                            valueCalcSum->calcImageValue(statsPxlsInRefPxl, nStatsPixelsInRefPxl, useNoDataVal, noDataVal, outImgBandVals);
                            for(unsigned int b = 0; b < numOutImgBands; ++b)
                            {
                                refDataArrOuts[b][((n*xIOGrid)+m)] = outImgBandVals[b];
                            }
                            ib_colOffStats += nXPxls;
                        }
                        ib_rowOffStats += (nYPxls * (xIOGrid * nXPxls));
                    }
                    
                    // Write Block
                    for(unsigned int n = 0; n < numOutImgBands; ++n)
                    {
                        if(outBands[n]->RasterIO(GF_Write, colOffsetRef, rowOffsetRef, xIOGrid, remainRows, refDataArrOuts[n], xIOGrid, remainRows, GDT_Float64, 0, 0))
                        {
                            throw RSGISImageException("Failed to write image data to output image.");
                        }
                    }
                    
                    ++blockCounter;
                    colOffsetStats += xIOGridStats;
                    colOffsetRef += xIOGrid;
                }
                
                if(remainCols > 0)
                {
                    if((feedback != 0) && ((blockCounter % feedback) == 0))
                    {
                        std::cout << "." << feedbackCounter << "." << std::flush;
                        feedbackCounter = feedbackCounter + 10;
                    }
                    
                    // Read Block
                    if(statsBand->RasterIO(GF_Read, colOffsetStats, rowOffsetStats, remainColsStats, remainRowsStats, statsDataArr, remainColsStats, remainRowsStats, GDT_Float32, 0, 0))
                    {
                        throw RSGISImageException("Failed to read image data from stats band.");
                    }
                    
                    // Process Block
                    ib_rowOffStats = 0;
                    ib_colOffStats = 0;
                    for(int n = 0; n < remainRows; ++n)
                    {
                        ib_colOffStats = 0;
                        for(int m = 0; m < remainCols; ++m)
                        {
                            for(int y = 0; y < nYPxls; ++y)
                            {
                                for(int x = 0; x < nXPxls; ++x)
                                {
                                    statsPxlsInRefPxl[(y*nYPxls)+x] = statsDataArr[(ib_colOffStats+x)+(ib_rowOffStats+((remainRows*nYPxls)*y))];
                                }
                            }
                            
                            valueCalcSum->calcImageValue(statsPxlsInRefPxl, nStatsPixelsInRefPxl, useNoDataVal, noDataVal, outImgBandVals);
                            for(unsigned int b = 0; b < numOutImgBands; ++b)
                            {
                                refDataArrOuts[b][((n*remainCols)+m)] = outImgBandVals[b];
                            }
                            ib_colOffStats += nXPxls;
                        }
                        ib_rowOffStats += (nYPxls * (remainCols * nXPxls));
                    }
                    
                    // Write Block
                    for(unsigned int n = 0; n < numOutImgBands; ++n)
                    {
                        if(outBands[n]->RasterIO(GF_Write, colOffsetRef, rowOffsetRef, remainCols, remainRows, refDataArrOuts[n], remainCols, remainRows, GDT_Float64, 0, 0))
                        {
                            throw RSGISImageException("Failed to write image data to output image.");
                        }
                    }
                    
                    ++blockCounter;
                    colOffsetStats += remainColsStats;
                    colOffsetRef += remainCols;
                }
                rowOffsetStats += remainRowsStats;
                rowOffsetRef += remainRows;
            }
            std::cout << " Complete.\n";
            
            GDALClose(outputImageDS);
            
            for(unsigned int i = 0; i < numOutImgBands; ++i)
            {
                delete[] refDataArrOuts[i];
            }
            delete[] refDataArrOuts;
            delete[] outBands;
            delete[] statsDataArr;
            delete[] statsPxlsInRefPxl;
            delete[] outImgBandVals;
            
            delete[] refImgTrans;
            delete[] statsImgTrans;
            delete[] outImgTrans;
        }
        catch (RSGISImageException &e)
        {
            throw e;
        }
        catch (RSGISException &e)
        {
            throw RSGISImageException(e.what());
        }
        catch (std::exception &e)
        {
            throw RSGISImageException(e.what());
        }
    }
    
    RSGISCalcImageMultiImgRes::~RSGISCalcImageMultiImgRes()
    {
        
    }

    
	
}} //rsgis::img


/*
 *  RSGISEliminateSmallClumps.h
 *  RSGIS_LIB
 *
 *  Created by Pete Bunting on 17/01/2012.
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

#ifndef RSGISEliminateSmallClumps_h
#define RSGISEliminateSmallClumps_h

#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <deque>
#include <list>
#include <math.h>

#include "gdal_priv.h"

#include "common/RSGISAttributeTableException.h"
#include "common/RSGISFileException.h"

#include "utils/RSGISTextUtils.h"

#include "img/RSGISImageUtils.h"
#include "img/RSGISImageCalcException.h"
#include "img/RSGISCalcImageValue.h"
#include "img/RSGISCalcImage.h"
#include "img/RSGISStretchImage.h"

//#include "rastergis/RSGISAttributeTable.h"
//#include "rastergis/RSGISPopulateAttributeTable.h"
//#include "rastergis/RSGISFindClumpNeighbours.h"

namespace rsgis{namespace segment{
    
    class RSGISEliminateSmallClumps
    {
    public:
        RSGISEliminateSmallClumps();
        void eliminateSmallClumps(GDALDataset *spectral, GDALDataset *clumps, unsigned int minClumpSize, float specThreshold) throw(rsgis::img::RSGISImageCalcException);
        void stepwiseEliminateSmallClumps(GDALDataset *spectral, GDALDataset *clumps, unsigned int minClumpSize, float specThreshold, std::vector<rsgis::img::BandSpecThresholdStats> *bandStretchStats, bool bandStatsAvail) throw(rsgis::img::RSGISImageCalcException);
        void stepwiseIterativeEliminateSmallClumps(GDALDataset *spectral, GDALDataset *clumps, unsigned int minClumpSize, float specThreshold, std::vector<rsgis::img::BandSpecThresholdStats> *bandStretchStats, bool bandStatsAvail) throw(rsgis::img::RSGISImageCalcException);
        void stepwiseEliminateSmallClumpsNoMean(GDALDataset *spectral, GDALDataset *clumps, unsigned int minClumpSize, float specThreshold, std::vector<rsgis::img::BandSpecThresholdStats> *bandStretchStats, bool bandStatsAvail) throw(rsgis::img::RSGISImageCalcException);
        //void stepwiseEliminateSmallClumpsWithAtt(GDALDataset *spectral, GDALDataset *clumps, std::string outputImageFile, std::string imageFormat, bool useImageProj, std::string proj, rsgis::rastergis::RSGISAttributeTable *attTable, unsigned int minClumpSize, float specThreshold, bool outputWithConsecutiveFIDs, std::vector<rsgis::img::BandSpecThresholdStats> *bandStretchStats, bool bandStatsAvail) throw(rsgis::img::RSGISImageCalcException);
        ~RSGISEliminateSmallClumps();
        //protected:
        //void defineOutputFID(rsgis::rastergis::RSGISAttributeTable *attTable, rsgis::rastergis::RSGISFeature *feat, unsigned int eliminatedFieldIdx, unsigned int mergedToFIDIdx, unsigned int outFIDIdx, unsigned int outFIDSetFieldIdx) throw(rsgis::RSGISAttributeTableException);
        //void performElimination(rsgis::rastergis::RSGISAttributeTable *attTable, std::vector<std::pair<unsigned long, unsigned long> > *eliminationVectors, unsigned int eliminatedFieldIdx, unsigned int mergedToFIDIdx, unsigned int pxlCountIdx, std::vector<rsgis::rastergis::RSGISBandAttStats*> *bandStats) throw(rsgis::RSGISAttributeTableException);
        //rsgis::rastergis::RSGISFeature* getEliminatedNeighbour(rsgis::rastergis::RSGISFeature *feat, rsgis::rastergis::RSGISAttributeTable *attTable, unsigned int eliminatedFieldIdx, unsigned int mergedToFIDIdx)throw(rsgis::RSGISAttributeTableException);
    };
    /*
    class RSGISEliminateFeature : public rsgis::rastergis::RSGISProcessFeature
    {
    public:
        RSGISEliminateFeature(unsigned int eliminatedFieldIdx, unsigned int mergedToFIDIdx, float specThreshold, unsigned int pxlCountIdx, std::vector<rsgis::rastergis::RSGISBandAttStats*> *bandStats, std::vector<std::pair<unsigned long, unsigned long> > *eliminationPairs, bool bandStatsAvail, double *stretch2reflOffs, double *stretch2reflGains);
        void processFeature(rsgis::rastergis::RSGISFeature *feat, rsgis::rastergis::RSGISAttributeTable *attTable)throw(rsgis::RSGISAttributeTableException);
        ~RSGISEliminateFeature();
    protected:
        double calcDistance(rsgis::rastergis::RSGISFeature *feat1, rsgis::rastergis::RSGISFeature *feat2, std::vector<rsgis::rastergis::RSGISBandAttStats*> *bandStats)throw(rsgis::RSGISAttributeTableException);
        double calcDistanceGainAndOff(rsgis::rastergis::RSGISFeature *feat1, rsgis::rastergis::RSGISFeature *feat2, std::vector<rsgis::rastergis::RSGISBandAttStats*> *bandStats)throw(rsgis::RSGISAttributeTableException);
        rsgis::rastergis::RSGISFeature* getEliminatedNeighbour(rsgis::rastergis::RSGISFeature *feat, rsgis::rastergis::RSGISAttributeTable *attTable)throw(rsgis::RSGISAttributeTableException); 
        unsigned int eliminatedFieldIdx;
        unsigned int mergedToFIDIdx;
        float specThreshold;
        unsigned int pxlCountIdx;
        std::vector<rsgis::rastergis::RSGISBandAttStats*> *bandStats;
        std::vector<std::pair<unsigned long, unsigned long> > *eliminationPairs;
        bool bandStatsAvail;
        double *stretch2reflOffs;
        double *stretch2reflGains;
    };
    */
    
    /*
    class RSGISApplyOutputFIDs : public rsgis::img::RSGISCalcImageValue
    {
    public:
        RSGISApplyOutputFIDs(rsgis::rastergis::RSGISAttributeTable *attTable, unsigned int outFIDIdx, unsigned int outFIDSetFieldIdx);
        void calcImageValue(float *bandValues, int numBands, float *output) throw(rsgis::img::RSGISImageCalcException);
        void calcImageValue(float *bandValues, int numBands) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented.");};
        void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
        void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals, float *output) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
        void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals, geos::geom::Envelope extent)throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
        void calcImageValue(float *bandValues, int numBands, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented.");};
        void calcImageValue(float *bandValues, int numBands, float *output, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented.");};
        void calcImageValue(float ***dataBlock, int numBands, int winSize, float *output) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented.");};
        void calcImageValue(float ***dataBlock, int numBands, int winSize, float *output, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented.");};
        bool calcImageValueCondition(float ***dataBlock, int numBands, int winSize, float *output) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented.");};
        ~RSGISApplyOutputFIDs();
    protected:
        rsgis::rastergis::RSGISAttributeTable *attTable;
        unsigned int outFIDIdx;
        unsigned int outFIDSetFieldIdx;
    };
    */
    
    class RSGISPopulateMeansPxlLocs : public rsgis::img::RSGISCalcImageValue
    {
    public:
        RSGISPopulateMeansPxlLocs(std::vector<rsgis::img::ImgClump*> *clumpTable, unsigned int numSpecBands);
        void calcImageValue(float *bandValues, int numBands, float *output) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented.");};
        void calcImageValue(float *bandValues, int numBands) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented.");};
        void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
        void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals, float *output) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
        void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals, geos::geom::Envelope extent)throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
        void calcImageValue(float *bandValues, int numBands, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException);
        void calcImageValue(float *bandValues, int numBands, float *output, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented.");};
        void calcImageValue(float ***dataBlock, int numBands, int winSize, float *output) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented.");};
        void calcImageValue(float ***dataBlock, int numBands, int winSize, float *output, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented.");};
        bool calcImageValueCondition(float ***dataBlock, int numBands, int winSize, float *output) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented.");};
        ~RSGISPopulateMeansPxlLocs();
    protected:
        std::vector<rsgis::img::ImgClump*> *clumpTable;
        unsigned int numSpecBands;
    };
    
    

    class RSGISRemoveClumpsBelowThreshold : public rsgis::img::RSGISCalcImageValue
    {
    public:
        RSGISRemoveClumpsBelowThreshold(float threshold, int *clumpHisto, size_t numHistVals);
        void calcImageValue(float *bandValues, int numBands, float *output) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
        void calcImageValue(float *bandValues, int numBands) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented.");};
        void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
        void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals, float *output) throw(rsgis::img::RSGISImageCalcException);
        void calcImageValue(long *intBandValues, unsigned int numIntVals, float *floatBandValues, unsigned int numfloatVals, geos::geom::Envelope extent)throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented");};
        void calcImageValue(float *bandValues, int numBands, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented.");};
        void calcImageValue(float *bandValues, int numBands, float *output, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented.");};
        void calcImageValue(float ***dataBlock, int numBands, int winSize, float *output) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented.");};
        void calcImageValue(float ***dataBlock, int numBands, int winSize, float *output, geos::geom::Envelope extent) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented.");};
        bool calcImageValueCondition(float ***dataBlock, int numBands, int winSize, float *output) throw(rsgis::img::RSGISImageCalcException){throw rsgis::img::RSGISImageCalcException("Not implemented.");};
        ~RSGISRemoveClumpsBelowThreshold();
    protected:
        float threshold;
        int *clumpHisto;
        size_t numHistVals;
    };
    
}}

#endif

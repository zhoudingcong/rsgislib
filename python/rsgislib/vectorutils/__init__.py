"""
The vector utils module performs geometry / attribute table operations on vectors.
"""

# import the C++ extension into this level
from ._vectorutils import *

import os.path
import os
import shutil
import subprocess

import osgeo.gdal as gdal
import osgeo.osr as osr
import osgeo.ogr as ogr

# Import the RSGISLib module
import rsgislib

# Import the RSGISLib Image Utils module
from rsgislib import imageutils

# Import the RSGISLib RasterGIS module
from rsgislib import rastergis

def rasterise2Image(inputVec, inputImage, outImage, gdalformat="KEA", burnVal=1, shpAtt=None, shpExt=False):
    """ 
A utillity to rasterise a shapefile into an image covering the same region and at the same resolution as the input image. 

Where:

* inputVec is a string specifying the input vector (shapefile) file
* inputImage is a string specifying the input image defining the grid, pixel resolution and area for the rasterisation (if None and shpExt is False them assumes output image already exists and just uses it as is burning vector into it)
* outImage is a string specifying the output image for the rasterised shapefile
* gdalformat is the output image format (Default: KEA).
* burnVal is the value for the output image pixels if no attribute is provided.
* shpAtt is a string specifying the attribute to be rasterised, value of None creates a binary mask and \"FID\" creates a temp shapefile with a "FID" column and rasterises that column.
* shpExt is a boolean specifying that the output image should be cut to the same extent as the input shapefile (Default is False and therefore output image will be the same as the input).

Example::

    from rsgislib import vectorutils
    
    inputVector = 'crowns.shp'
    inputImage = 'injune_p142_casi_sub_utm.kea'
    outputImage = 'psu142_crowns.kea'  
    vectorutils.rasterise2Image(inputVector, inputImage, outputImage, 'KEA', shpAtt='FID')

"""
    try:
        gdal.UseExceptions()
        
        if shpExt:
            print("Creating output image from shapefile extent")
            imageutils.createCopyImageVecExtent(inputImage, inputVec, outImage, 1, 0, gdalformat, rsgislib.TYPE_32UINT)
        elif inputImage is None:
            print("Assuming output image is already created so just using.")
        else:
            print("Creating output image using input image")
            imageutils.createCopyImage(inputImage, outImage, 1, 0, gdalformat, rsgislib.TYPE_32UINT)
        
        if shpAtt == "FID":   
            tmpVector = os.path.splitext(inputVec)[0] + "_tmpFIDFile.shp"
            print("Added FID Column...")
            addFIDColumn(inputVec, tmpVector, True)
        else:
            tmpVector = inputVec
        
        print("Running Rasterise now...")
        outRasterDS = gdal.Open(outImage, gdal.GA_Update)
        
        inVectorDS = ogr.Open(tmpVector)
        inVectorLayer = inVectorDS.GetLayer(0)
        
        # Run the algorithm.
        err = 0
        if shpAtt is None:
            err = gdal.RasterizeLayer(outRasterDS, [1], inVectorLayer, burn_values=[burnVal])
        else:
            err = gdal.RasterizeLayer(outRasterDS, [1], inVectorLayer, options=["ATTRIBUTE="+shpAtt])
        if err != 0:
            raise Exception("Rasterisation Error: " + str(err))
        
        outRasterDS = None
        inVectorDS = None
        
        if shpAtt == "FID":
            driver = ogr.GetDriverByName("ESRI Shapefile")
            if os.path.exists(tmpVector):
                driver.DeleteDataSource(tmpVector)
        
        print("Adding Colour Table")
        rastergis.populateStats(clumps=outImage, addclrtab=True, calcpyramids=True, ignorezero=True)
        print("Completed")
    except Exception as e:
        raise e



def copyShapefile2RAT(inputVec, inputImage, outputImage):
    """ 
A utillity to create raster copy of a shapefile. The output image is a KEA file and the attribute table has the attributes from the shapefile. 
    
Where:

* inputVec is a string specifying the input vector (shapefile) file
* inputImage is a string specifying the input image defining the grid, pixel resolution and area for the rasterisation
* outputImage is a string specifying the output KEA image for the rasterised shapefile

Example::

    from rsgislib import vectorutils
     
    inputVector = 'crowns.shp'
    inputImage = 'injune_p142_casi_sub_utm.kea'
    outputImage = 'psu142_crowns.kea'
        
    vectorutils.copyShapefile2RAT(inputVector, inputImage, outputImage)

"""
    try:
        rasterise2Image(inputVec, inputImage, outputImage, "KEA", shpAtt="FID")
        rsgislib.rastergis.importVecAtts(outputImage, inputVec, None)
    except Exception as e:
        raise e


def polygoniseRaster(inputImg, outShp, imgBandNo=1, maskImg=None, imgMaskBandNo=1 ):
    """ 
A utillity to polygonise a raster to a ESRI Shapefile. 
    
Where:

* inputImg is a string specifying the input image file to be polygonised
* outShp is a string specifying the output shapefile path. If it exists it will be deleted and overwritten.
* imgBandNo is an int specifying the image band to be polygonised. (default = 1)
* maskImg is an optional string mask file specifying a no data mask (default = None)
* imgMaskBandNo is an int specifying the image band to be used the mask (default = 1)

Example::

    from rsgislib import vectorutils
     
    inputVector = 'crowns.shp'
    inputImage = 'injune_p142_casi_sub_utm.kea'
    outputImage = 'psu142_crowns.kea'
        
    vectorutils.copyShapefile2RAT(inputVector, inputImage, outputImage)

"""
    gdal.UseExceptions()
    
    gdalImgData = gdal.Open(inputImg)
    imgBand = gdalImgData.GetRasterBand(imgBandNo)
    imgsrs = osr.SpatialReference()
    imgsrs.ImportFromWkt(gdalImgData.GetProjectionRef())
    
    gdalImgMaskData = None
    imgMaskBand = None
    if maskImg is not None:
        print("Using mask")
        gdalImgMaskData = gdal.Open(maskImg)
        imgMaskBand = gdalImgData.GetRasterBand(imgMaskBandNo)

    
    driver = ogr.GetDriverByName("ESRI Shapefile")
    if os.path.exists(outShp):
        driver.DeleteDataSource(outShp)
    outDatasource = driver.CreateDataSource(outShp)
    
    layerName = os.path.splitext(os.path.basename(outShp))[0]
    outLayer = outDatasource.CreateLayer(layerName, srs=imgsrs)
    
    newField = ogr.FieldDefn('PXLVAL', ogr.OFTInteger)
    outLayer.CreateField(newField)
    dstFieldIdx = outLayer.GetLayerDefn().GetFieldIndex('PXLVAL')
    
    print("Polygonising...")
    gdal.Polygonize(imgBand, imgMaskBand, outLayer, dstFieldIdx, [], callback=gdal.TermProgress )
    print("Completed")
    outDatasource.Destroy()
    gdalImgData = None
    if maskImg is not None:
        gdalImgMaskData = None


def writeVecColumn(vectorFile, vectorLayer, colName, colDataType, colData):
    """
A function which will write a column to a vector file

Where:

* vectorFile - The file / path to the vector data 'file'.
* vectorLayer - The layer to which the data is to be added.
* colName - Name of the output column
* colDataType - ogr data type (e.g., ogr.OFTString, ogr.OFTInteger, ogr.OFTReal)
* colData - A list of the same length as the number of features in vector file.

Example::

    from rsgislib import vectorutils
    import rsgislib
    import osgeo.ogr as ogr
    
    rsgisUtils = rsgislib.RSGISPyUtils()
    requiredScenes = rsgisUtils.readTextFile2List("GMW_JERS-1_ScenesRequired.txt")
    requiredScenesShp = "JERS-1_Scenes_Requred_shp"
    vectorutils.writeVecColumn(requiredScenesShp+'.shp', requiredScenesShp, 'ScnName', ogr.OFTString, requiredScenes)

"""
    gdal.UseExceptions()
    
    ds = gdal.OpenEx(vectorFile, gdal.OF_UPDATE )
    if ds is None:
        raise Exception("Could not open '" + vectorFile + "'")
    
    lyr = ds.GetLayerByName( vectorLayer )
    
    if lyr is None:
        raise Exception("Could not find layer '" + vectorLayer + "'")
    
    numFeats = lyr.GetFeatureCount()
    
    if not len(colData) == numFeats:
        print("Number of Features: " + str(numFeats))
        print("Length of Data: " + str(len(colData)))
        raise Exception( "The number of features and size of the input data is not equal." )

    colExists = False
    lyrDefn = lyr.GetLayerDefn()
    for i in range( lyrDefn.GetFieldCount() ):
        if lyrDefn.GetFieldDefn(i).GetName() == colName:
            colExists = True
            break
    
    if not colExists:
        field_defn = ogr.FieldDefn( colName, colDataType )
        if lyr.CreateField ( field_defn ) != 0:
            raise Exception("Creating '" + colName + "' field failed.\n")

    lyr.ResetReading()
    i = 0
    for feat in lyr:
        feat.SetField(colName, colData[i] )
        lyr.SetFeature(feat)
        i = i + 1
    lyr.SyncToDisk()
    ds = None


def readVecColumn(vectorFile, vectorLayer, colName):
    """
A function which will reads a column from a vector file

Where:

* vectorFile - The file / path to the vector data 'file'.
* vectorLayer - The layer to which the data is to be read from.
* colName - Name of the input column

Example::

    from rsgislib import vectorutils
    import rsgislib
    
    rsgisUtils = rsgislib.RSGISPyUtils()
    requiredScenes = rsgisUtils.readTextFile2List("GMW_JERS-1_ScenesRequired.txt")
    requiredScenesShp = "JERS-1_Scenes_Requred_shp"
    vectorutils.writeVecColumn(requiredScenesShp+'.shp', requiredScenesShp, 'ScnName', ogr.OFTString, requiredScenes)

"""
    gdal.UseExceptions()
    
    ds = gdal.OpenEx(vectorFile, gdal.OF_UPDATE )
    if ds is None:
        raise Exception("Could not open '" + vectorFile + "'")
    
    lyr = ds.GetLayerByName( vectorLayer )
    
    if lyr is None:
        raise Exception("Could not find layer '" + vectorLayer + "'")
    

    colExists = False
    lyrDefn = lyr.GetLayerDefn()
    for i in range( lyrDefn.GetFieldCount() ):
        if lyrDefn.GetFieldDefn(i).GetName() == colName:
            colExists = True
            break
    
    if not colExists:
        raise Exception("The specified column does not exist in the input layer.")
    
    outVal = list()
    lyr.ResetReading()
    for feat in lyr:
        outVal.append(feat.GetField(colName))

    lyr.SyncToDisk()
    ds = None
    
    return outVal


def extractImageFootprint(inputImg, outVec, tmpDIR='./tmp', rePrjTo=None):
    """
A function to extract an image footprint as a vector.

* inputImg - the input image file for which the footprint will be extracted.
* outVec - output shapefile path and name.
* tmpDIR - temp directory which will be used during processing. It will be created and deleted once processing complete.
* rePrjTo - optional command 

"""
    gdal.UseExceptions()

    rsgisUtils = rsgislib.RSGISPyUtils()
    
    uidStr = rsgisUtils.uidGenerator()
    
    createdTmp = False
    if not os.path.exists(tmpDIR):
        os.makedirs(tmpDIR)
        createdTmp = True
    
    inImgBase = os.path.splitext(os.path.basename(inputImg))[0]
    
    validOutImg = os.path.join(tmpDIR, inImgBase+'_'+uidStr+'_validimg.kea')
    inImgNoData = rsgisUtils.getImageNoDataValue(inputImg)
    rsgislib.imageutils.genValidMask(inimages=inputImg, outimage=validOutImg, gdalformat='KEA', nodata=inImgNoData)
    
    outVecTmpFile = outVec
    if not (rePrjTo is None):
        outVecTmpFile = os.path.join(tmpDIR, inImgBase+'_'+uidStr+'_initVecOut.shp')
    
    rsgislib.vectorutils.polygoniseRaster(validOutImg, outVecTmpFile, imgBandNo=1, maskImg=validOutImg, imgMaskBandNo=1)
    vecLayerName = os.path.splitext(os.path.basename(outVecTmpFile))[0]
    ds = gdal.OpenEx(outVecTmpFile, gdal.OF_READONLY )
    if ds is None:
        raise Exception("Could not open '" + vectorFile + "'")
    
    lyr = ds.GetLayerByName( vecLayerName )
    if lyr is None:
        raise Exception("Could not find layer '" + vecLayerName + "'")
    numFeats = lyr.GetFeatureCount()
    lyr = None
    ds = None
    
    fileName = []
    for i in range(numFeats):
        fileName.append(os.path.basename(inputImg))
    rsgislib.vectorutils.writeVecColumn(outVecTmpFile, vecLayerName, 'FileName', ogr.OFTString, fileName)
    
    if not (rePrjTo is None):
        if os.path.exists(outVec):
            driver = ogr.GetDriverByName('ESRI Shapefile')
            driver.DeleteDataSource(outVec)
    
        cmd = 'ogr2ogr -f "ESRI Shapefile" -t_srs ' + rePrjTo + ' ' + outVec + ' ' + outVecTmpFile
        print(cmd)
        try:
            subprocess.call(cmd, shell=True)
        except OSError as e:
            raise Exception('Could not re-projection shapefile: ' + cmd)
    
    if createdTmp:
        shutil.rmtree(tmpDIR)
    else:
        if not (rePrjTo is None):
            driver = ogr.GetDriverByName('ESRI Shapefile')
            driver.DeleteDataSource(outVecTmpFile)

def getVecFeatCount(inVec, layerName=None, computeCount=True):
    """
Get a count of the number of features in the vector layers.

* inVec - is a string with the input vector file name and path.
* layerName - is the layer for which extent is to be calculated (Default: None). if None assume there is only one layer and that will be read.
* computeCount - is a boolean which specifies whether the layer extent 
                 should be calculated (rather than estimated from header)
                 even if that operation is computationally expensive.

return: nfeats

"""
    gdal.UseExceptions()
    inDataSource = gdal.OpenEx(inVec, gdal.OF_VECTOR )
    if layerName is not None:
        inLayer = inDataSource.GetLayer(layerName)
    else:
        inLayer = inDataSource.GetLayer()
    nFeats = inLayer.GetFeatureCount(computeCount)
    return nFeats


def mergeShapefiles(inFileList, outVecFile):
    """
Function which will merge a list of shapefiles into an single shapefile using ogr2ogr.

Where:

* inFileList - is a list of input files.
* outVecFile - is the output shapefile

"""
    if os.path.exists(outVecFile):
        driver = ogr.GetDriverByName('ESRI Shapefile')
        driver.DeleteDataSource(outVecFile)
    first = True
    for inFile in inFileList:
        nFeat = getVecFeatCount(inFile)
        print("Processing: " + inFile + " has " + str(nFeat) + " features.")
        if nFeat > 0:
            if first:
                cmd = 'ogr2ogr -f "ESRI Shapefile"  "' + outVecFile + '" "' + inFile + '"'
                try:
                    subprocess.call(cmd, shell=True)
                except OSError as e:
                    raise Exception('Error running ogr2ogr: ' + cmd)
                first = False
            else:
                cmd = 'ogr2ogr -update -append -f "ESRI Shapefile" "' + outVecFile + '" "' + inFile + '"'
                try:
                    subprocess.call(cmd, shell=True)
                except OSError as e:
                    raise Exception('Error running ogr2ogr: ' + cmd)

def mergeVectors2SQLiteDB(inFileList, outDBFile, lyrName, exists):
    """
Function which will merge a list of vector files into an single output SQLite database using ogr2ogr.

Where:

* inFileList - is a list of input files.
* outDBFile - is the output SQLite database (\*.sqlite)
* lyrName - is the layer name in the output database (i.e., you can merge layers into single layer or write a number of layers to the same database).
* exists - boolean which specifies whether the database file exists or not.

"""
    first = True
    for inFile in inFileList:
        nFeat = getVecFeatCount(inFile)
        print("Processing: " + inFile + " has " + str(nFeat) + " features.")
        if nFeat > 0:
            if first:
                if not exists:
                    cmd = 'ogr2ogr -f "SQLite" -lco COMPRESS_GEOM=YES -lco SPATIAL_INDEX=YES -nln '+lyrName+' "' + outDBFile + '" "' + inFile + '"'
                    try:
                        subprocess.call(cmd, shell=True)
                    except OSError as e:
                        raise Exception('Error running ogr2ogr: ' + cmd)
                else:
                    cmd = 'ogr2ogr -update -f "SQLite" -lco COMPRESS_GEOM=YES -lco SPATIAL_INDEX=YES -nln '+lyrName+' "' + outDBFile + '" "' + inFile + '"'
                    try:
                        subprocess.call(cmd, shell=True)
                    except OSError as e:
                        raise Exception('Error running ogr2ogr: ' + cmd)
                first = False
            else:
                cmd = 'ogr2ogr -update -append -f "SQLite" -nln '+lyrName+' "' + outDBFile + '" "' + inFile + '"'
                try:
                    subprocess.call(cmd, shell=True)
                except OSError as e:
                    raise Exception('Error running ogr2ogr: ' + cmd)


def createPolySHP4LstBBOXs(csvFile, outSHP, espgCode, minXCol=0, maxXCol=1, minYCol=2, maxYCol=3, ignoreRows=0, force=False):
    """
This function takes a CSV file of bounding boxes (1 per line) and creates a polygon shapefile.
    
* csvFile - input CSV file.
* outSHP - output ESRI shapefile
* espgCode - ESPG code specifying the projection of the data (4326 is WSG84 Lat/Long).
* minXCol - The index (starting at 0) for the column within the CSV file for the minimum X coordinate.
* maxXCol - The index (starting at 0) for the column within the CSV file for the maximum X coordinate.
* minYCol - The index (starting at 0) for the column within the CSV file for the minimum Y coordinate.
* maxYCol - The index (starting at 0) for the column within the CSV file for the maximum Y coordinate.
* ignoreRows - The number of rows to ignore from the start of the CSV file (i.e., column headings)
* force - If the output file already exists delete it before proceeding.

"""
    gdal.UseExceptions()
    try:
        if os.path.exists(outSHP):
            if force:
                driver = ogr.GetDriverByName('ESRI Shapefile')
                driver.DeleteDataSource(outSHP)
            else:
                raise Exception("Output file already exists")
        # Create the output Driver
        outDriver = ogr.GetDriverByName('ESRI Shapefile')
        # create the spatial reference, WGS84
        srs = osr.SpatialReference()
        srs.ImportFromEPSG(int(espgCode))
        # Create the output Shapefile
        outDataSource = outDriver.CreateDataSource(outSHP)
        outLayer = outDataSource.CreateLayer(os.path.splitext(os.path.basename(outSHP))[0], srs, geom_type=ogr.wkbPolygon )
        # Get the output Layer's Feature Definition
        featureDefn = outLayer.GetLayerDefn()
    
        dataFile = open(csvFile, 'r')
        rowCount = 0
        for line in dataFile:
            if rowCount >= ignoreRows:
                line = line.strip()
                if line != "":
                    comps = line.split(',')
                    # Get values from CSV file.
                    minX = float(comps[minXCol])
                    maxX = float(comps[maxXCol])
                    minY = float(comps[minYCol])
                    maxY = float(comps[maxYCol])
                    # Create Linear Ring
                    ring = ogr.Geometry(ogr.wkbLinearRing)
                    ring.AddPoint(minX, maxY)
                    ring.AddPoint(maxX, maxY)
                    ring.AddPoint(maxX, minY)
                    ring.AddPoint(minX, minY)
                    ring.AddPoint(minX, maxY)
                    # Create polygon.
                    poly = ogr.Geometry(ogr.wkbPolygon)
                    poly.AddGeometry(ring)
                    # Add to output shapefile.
                    outFeature = ogr.Feature(featureDefn)
                    outFeature.SetGeometry(poly)
                    outLayer.CreateFeature(outFeature)
                    outFeature = None
            rowCount = rowCount + 1
        dataFile.close()
        outDataSource = None
    except Exception as e:
        raise e



def getVecLayerExtent(inVec, layerName=None, computeIfExp=True):
    """
Get the extent of the vector layer.

* inVec - is a string with the input vector file name and path.
* layerName - is the layer for which extent is to be calculated (Default: None)
*             if None assume there is only one layer and that will be read.
* computeIfExp - is a boolean which specifies whether the layer extent 
                 should be calculated (rather than estimated from header)
                 even if that operation is computationally expensive.

return: boundary box is returned (MinX, MaxX, MinY, MaxY)

"""
    gdal.UseExceptions()
    # Get a Layer's Extent
    inDataSource = gdal.OpenEx(inVec, gdal.OF_VECTOR )
    if layerName is not None:
        inLayer = inDataSource.GetLayer(layerName)
    else:
        inLayer = inDataSource.GetLayer()
    extent = inLayer.GetExtent(computeIfExp)
    return extent

def getProjWKTFromVec(inVec):
    """
* inVec - is a string with the input vector file name and path.

return: WKT representation of projection

"""
    gdal.UseExceptions()
    # Get shapefile projection as WKT
    dataset = gdal.OpenEx(inVec, gdal.OF_VECTOR )
    layer = dataset.GetLayer()
    spatialRef = layer.GetSpatialRef()
    return spatialRef.ExportToWkt()


def reProjVectorLayer(inputVec, outputVec, outProjWKT, outDriverName='ESRI Shapefile', outLyrName=None, inLyrName=None, inProjWKT=None, force=False):
    """
A function which reprojects a vector layer. 
    
* inputVec is a string with name and path to input vector file.
* outputVec is a string with name and path to output vector file.
* outProjWKT is a string with the WKT string for the output vector file.
* outDriverName is the output vector file format. Default is ESRI Shapefile.
* outLyrName is a string for the output layer name. If None then ignored and 
             assume there is just a single layer in the vector and layer name
             is the same as the file name.
* inLyrName is a string for the input layer name. If None then ignored and 
            assume there is just a single layer in the vector.
* inProjWKT is a string with the WKT string for the input shapefile 
            (Optional; taken from input file if not specified).

"""
    ## This code has been editted from https://pcjericks.github.io/py-gdalogr-cookbook/projection.html#reproject-a-layer
    ## Updated for GDAL 2.0
    gdal.UseExceptions()
        
    # get the input layer
    inDataSet = gdal.OpenEx(inputVec, gdal.OF_VECTOR )
    if inDataSet is None:
        raise("Failed to open input shapefile\n") 
    if inLyrName is None:   
        inLayer = inDataSet.GetLayer()
    else:
        inLayer = inDataSet.GetLayer(inLyrName)
    
    # input SpatialReference
    inSpatialRef = osr.SpatialReference()
    if inProjWKT is not None:
        inSpatialRef.ImportFromWkt(inProjWKT)
    else:
        inSpatialRef = inLayer.GetSpatialRef()
    
    # output SpatialReference
    outSpatialRef = osr.SpatialReference()
    outSpatialRef.ImportFromWkt(outProjWKT)
    
    # create the CoordinateTransformation
    coordTrans = osr.CoordinateTransformation(inSpatialRef, outSpatialRef)
    
    # Create shapefile driver
    driver = gdal.GetDriverByName( outDriverName )
    
    # create the output layer
    if os.path.exists(outputVec):
        if (outDriverName == 'ESRI Shapefile'):
            if force:
                driver.DeleteDataSource(outputVec)
            else:
                raise Exception('Output shapefile already exists - stopping.')
            outDataSet = driver.Create(outputVec, 0, 0, 0, gdal.GDT_Unknown )
        else:
            outDataSet = gdal.OpenEx(outputVec, gdal.OF_UPDATE )
    else:
        outDataSet = driver.Create(outputVec, 0, 0, 0, gdal.GDT_Unknown )
    
    if outLyrName is None:
        outLyrName = os.path.splitext(os.path.basename(outputVec))[0]
    outLayer = outDataSet.CreateLayer(outLyrName, outSpatialRef, inLayer.GetGeomType() )
    
    # add fields
    inLayerDefn = inLayer.GetLayerDefn()
    for i in range(0, inLayerDefn.GetFieldCount()):
        fieldDefn = inLayerDefn.GetFieldDefn(i)
        outLayer.CreateField(fieldDefn)
    
    # get the output layer's feature definition
    outLayerDefn = outLayer.GetLayerDefn()
    
    # loop through the input features
    inFeature = inLayer.GetNextFeature()
    while inFeature:
        # get the input geometry
        geom = inFeature.GetGeometryRef()
        if geom is not None:
            # reproject the geometry
            geom.Transform(coordTrans)
            # create a new feature
            outFeature = ogr.Feature(outLayerDefn)
            # set the geometry and attribute
            outFeature.SetGeometry(geom)
            for i in range(0, outLayerDefn.GetFieldCount()):
                outFeature.SetField(outLayerDefn.GetFieldDefn(i).GetNameRef(), inFeature.GetField(i))
            # add the feature to the shapefile
            outLayer.CreateFeature(outFeature)
        # dereference the features and get the next input feature
        outFeature = None
        inFeature = inLayer.GetNextFeature()
    
    # Save and close the shapefiles
    inDataSet = None
    outDataSet = None


def getAttLstSelectFeats(vecFile, vecLyr, attName, selVecFile, selVecLyr):
    """
Function to get a list of attribute values from features which intersect
with the select layer.
* vecFile - vector layer from which the attribute data comes from.
* vecLyr - the layer name from which the attribute data comes from.
* attName - name of the attribute to be outputted.
* selVecFile - the vector file which will be intersected within the vector file.
* selVecLyr - the layer name which will be intersected within the vector file.
"""
    gdal.UseExceptions()
    att_vals = []
    try:
        dsVecFile = gdal.OpenEx(vecFile, gdal.OF_READONLY )
        if dsVecFile is None:
            raise Exception("Could not open '" + vecFile + "'")
        
        lyrVecObj = dsVecFile.GetLayerByName( vecLyr )
        if lyrVecObj is None:
            raise Exception("Could not find layer '" + vecLyr + "'")
            
        dsSelVecFile = gdal.OpenEx(selVecFile, gdal.OF_READONLY )
        if dsSelVecFile is None:
            raise Exception("Could not open '" + selVecFile + "'")
        
        lyrSelVecObj = dsSelVecFile.GetLayerByName( selVecLyr )
        if lyrSelVecObj is None:
            raise Exception("Could not find layer '" + selVecLyr + "'")
        
        lyrDefn = lyrVecObj.GetLayerDefn()
        feat_idx = 0
        found_att = False
        for i in range(lyrDefn.GetFieldCount()):
            if lyrDefn.GetFieldDefn(i).GetName() == attName:
                feat_idx = i
                feat_type = lyrDefn.GetFieldDefn(feat_idx).GetType()
                found_att = True
         
        if not found_att:
            dsSelVecFile = None            
            dsVecFile = None
            raise Exception("Could not find the attribute specified within the vector layer.")
            
        mem_driver = ogr.GetDriverByName('MEMORY')
        
        mem_sel_ds = mem_driver.CreateDataSource('MemSelData')
        mem_sel_lyr = mem_sel_ds.CopyLayer(lyrSelVecObj, selVecLyr, ['OVERWRITE=YES'])
        
        mem_result_ds = mem_driver.CreateDataSource('MemResultData')
        mem_result_lyr = mem_result_ds.CreateLayer("MemResultLyr", geom_type=lyrVecObj.GetGeomType())
        
        result_field = ogr.FieldDefn(attName, lyrDefn.GetFieldDefn(feat_idx).GetType())
        mem_result_lyr.CreateField(result_field)
        
        lyrVecObj.Intersection(mem_sel_lyr, mem_result_lyr)
        
        # loop through the input features
        inFeat = mem_result_lyr.GetNextFeature()
        while inFeat:
            if feat_type == ogr.OFTString:
                att_vals.append(inFeat.GetFieldAsString(0))
            elif feat_type == ogr.OFTReal:
                att_vals.append(inFeat.GetFieldAsDouble(0))
            elif feat_type == ogr.OFTInteger:
                att_vals.append(inFeat.GetFieldAsInteger(0))
            
            inFeat = mem_result_lyr.GetNextFeature()
        
        dsSelVecFile = None        
        dsVecFile = None
        mem_sel_ds = None
        mem_result_ds = None
    except Exception as e:
        raise e
    return att_vals


def createPolyVecBBOXs(vectorFile, vectorLyr, vecDriver, espgCode, bboxs, atts=None, attTypes=None):
    """
This function creates a set of polygons for a set of bounding boxes. 

When creating an attribute the available data types are ogr.OFTString, ogr.OFTInteger, ogr.OFTReal
    
* vectorFile - output vector file/path
* vectorLyr - output vector layer
* vecDriver - the output vector layer type.
* espgCode - ESPG code specifying the projection of the data (e.g., 4326 is WSG84 Lat/Long).
* bboxs - is a list of bounding boxes ([xMin, xMax, yMin, yMax]) to be saved to the output vector.
* atts - is a dict of lists of attributes with the same length as the bboxs list. The dict should be named the same as the attTypes['names'] list.
* attTypes - is a dict with a list of attribute names (attTypes['names']) and types (attTypes['types']). The list must be the same length as one another and the number of atts.
"""
    try:
        gdal.UseExceptions()
        # Create the output Driver
        outDriver = ogr.GetDriverByName(vecDriver)
        # create the spatial reference, WGS84
        srs = osr.SpatialReference()
        srs.ImportFromEPSG(int(espgCode))
        # Create the output Shapefile
        outDataSource = outDriver.CreateDataSource(vectorFile)
        outLayer = outDataSource.CreateLayer(vectorLyr, srs, geom_type=ogr.wkbPolygon )
        
        
        addAtts = False
        if (atts is not None) and (attTypes is not None):
            nAtts = 0
            if not 'names' in attTypes:
                raise Exception('attTypes must include a list for "names"')
            nAtts = len(attTypes['names'])
            if not 'types' in attTypes:
                raise Exception('attTypes must include a list for "types"')
            if nAtts != len(attTypes['types']):
                raise Exception('attTypes "names" and "types" lists must be the same length.')
            for i in range(nAtts):
                if attTypes['names'][i] not in atts:
                    raise Exception('"{}" is not within atts'.format(attTypes['names'][i]))
                if len(atts[attTypes['names'][i]]) != len(bboxs):
                    raise Exception('"{}" in atts does not have the same len as bboxs'.format(attTypes['names'][i]))
                    
            for i in range(nAtts):       
                field_defn = ogr.FieldDefn( attTypes['names'][i], attTypes['types'][i] )
                if outLayer.CreateField ( field_defn ) != 0:
                    raise Exception("Creating '" + attTypes['names'][i] + "' field failed.\n")
            addAtts = True
        elif not ((atts is None) and (attTypes is None)): 
            raise Exception('If atts or attTypes is not None then the other should also not be none and equalivent in length.')
            
        # Get the output Layer's Feature Definition
        featureDefn = outLayer.GetLayerDefn()
        
        for n in range(len(bboxs)):
            bbox = bboxs[n]
            # Create Linear Ring
            ring = ogr.Geometry(ogr.wkbLinearRing)
            ring.AddPoint(bbox[0], bbox[3])
            ring.AddPoint(bbox[1], bbox[3])
            ring.AddPoint(bbox[1], bbox[2])
            ring.AddPoint(bbox[0], bbox[2])
            ring.AddPoint(bbox[0], bbox[3])
            # Create polygon.
            poly = ogr.Geometry(ogr.wkbPolygon)
            poly.AddGeometry(ring)
            # Add to output shapefile.
            outFeature = ogr.Feature(featureDefn)
            outFeature.SetGeometry(poly)
            if addAtts:
                # Add Attributes
                for i in range(nAtts):
                    outFeature.SetField(attTypes['names'][i], atts[attTypes['names'][i]][n])
            outLayer.CreateFeature(outFeature)
            outFeature = None
        outDataSource = None
    except Exception as e:
        raise e





def createImgExtentLUT(imgList, vectorFile, vectorLyr, vecDriver):
    """
Create a vector layer look up table (LUT) for a directory of images.

* imgList - list of input images for the LUT. All input images should be the same projection/coordinate system.
* vectorFile - output vector file/path
* vectorLyr - output vector layer
* vecDriver - the output vector layer type.

Example::

    import glob
    import rsgislib.vectorutils
    imgList = glob.glob('/Users/pete/Temp/GabonLandsat/Hansen*.kea')
    rsgislib.vectorutils.createImgExtentLUT(imgList, './ImgExtents.shp', 'ImgExtents', 'ESRI Shapefile')

"""
    gdal.UseExceptions()
    rsgisUtils = rsgislib.RSGISPyUtils()
    
    bboxs = []
    atts=dict()
    atts['filename'] = []
    atts['path'] = []
    
    attTypes = dict()
    attTypes['types'] = [ogr.OFTString, ogr.OFTString]
    attTypes['names'] = ['filename', 'path']
    
    epsgCode = 0
    
    first = True
    baseImg = ''
    for img in imgList:
        if first:
            epsgCode = int(rsgisUtils.getEPSGCode(img))
            baseImg = img
            first = False
        else:
            epsgCodeTmp = int(rsgisUtils.getEPSGCode(img))
            if epsgCodeTmp != epsgCode:
                raise Exception("The EPSG codes ({0} & {1}) do not match. (Base: '{2}', Img: '{3}')".format(epsgCode, epsgCodeTmp, baseImg, img))
        
        bboxs.append(rsgisUtils.getImageBBOX(img))
        baseName = os.path.basename(img)
        filePath = os.path.dirname(img)
        atts['filename'].append(baseName)
        atts['path'].append(filePath)
    # Create vector layer
    createPolyVecBBOXs(vectorFile, vectorLyr, vecDriver, epsgCode, bboxs, atts, attTypes)


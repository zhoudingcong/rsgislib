RSGISLib Image Calculations Module
===================================
.. automodule:: rsgislib.imagecalc
   :members:
   :undoc-members:

Band & Image Maths
---------------------

.. autofunction:: rsgislib.imagecalc.bandMath
.. autofunction:: rsgislib.imagecalc.imageMath
.. autofunction:: rsgislib.imagecalc.imageBandMath
.. autofunction:: rsgislib.imagecalc.allBandsEqualTo
.. autofunction:: rsgislib.imagecalc.replaceValuesLessThan

Clustering
-----------

.. autofunction:: rsgislib.imagecalc.kMeansClustering
.. autofunction:: rsgislib.imagecalc.isoDataClustering


Spectral Unmixing
------------------

.. autofunction:: rsgislib.imagecalc.unconLinearSpecUnmix
.. autofunction:: rsgislib.imagecalc.exhconLinearSpecUnmix
.. autofunction:: rsgislib.imagecalc.conSum1LinearSpecUnmix
.. autofunction:: rsgislib.imagecalc.nnConSum1LinearSpecUnmix



Statistics
-----------

.. autofunction:: rsgislib.imagecalc.getImageBandMinMax
.. autofunction:: rsgislib.imagecalc.imageStats
.. autofunction:: rsgislib.imagecalc.imageBandStats
.. autofunction:: rsgislib.imagecalc.meanVector
.. autofunction:: rsgislib.imagecalc.imagePixelLinearFit
.. autofunction:: rsgislib.imagecalc.pca
.. autofunction:: rsgislib.imagecalc.getPCAEigenVector
.. autofunction:: rsgislib.imagecalc.performImagePCA
.. autofunction:: rsgislib.imagecalc.calculateRMSE
.. autofunction:: rsgislib.imagecalc.bandPercentile
.. autofunction:: rsgislib.imagecalc.covariance
.. autofunction:: rsgislib.imagecalc.histogram
.. autofunction:: rsgislib.imagecalc.getHistogram
.. autofunction:: rsgislib.imagecalc.get2DImageHistogram
.. autofunction:: rsgislib.imagecalc.correlation
.. autofunction:: rsgislib.imagecalc.correlationWindow
.. autofunction:: rsgislib.imagecalc.getImageBandModeInEnv
.. autofunction:: rsgislib.imagecalc.getImageStatsInEnv
.. autofunction:: rsgislib.imagecalc.calcPropTrueExp
.. autofunction:: rsgislib.imagecalc.calcMultiImgBandStats
.. autofunction:: rsgislib.imagecalc.calcMaskImgPxlValProb
.. autofunction:: rsgislib.imagecalc.calcImageDifference
.. autofunction:: rsgislib.imagecalc.getImgIdxForStat
.. autofunction:: rsgislib.imagecalc.countPxlsOfVal
.. autofunction:: rsgislib.imagecalc.getImgSumStatsInPxl
.. autofunction:: rsgislib.imagecalc.calcImgMeanInMask
.. autofunction:: rsgislib.imagecalc.identifyMinPxlValueInWin


Normalise
----------

.. autofunction:: rsgislib.imagecalc.standardise
.. autofunction:: rsgislib.imagecalc.normalisation
.. autofunction:: rsgislib.imagecalc.rescaleImgPxlVals


Geometry
---------

.. autofunction:: rsgislib.imagecalc.dist2Geoms
.. autofunction:: rsgislib.imagecalc.imageDist2Geoms
.. autofunction:: rsgislib.imagecalc.mahalanobisDistFilter
.. autofunction:: rsgislib.imagecalc.mahalanobisDist2ImgFilter
.. autofunction:: rsgislib.imagecalc.imageCalcDistance
.. autofunction:: rsgislib.imagecalc.calcDist2ImgVals
.. autofunction:: rsgislib.imagecalc.calcDist2ImgValsTiled

Image Indices
-------------
.. autofunction:: rsgislib.imagecalc.calcindices.calcNDVI
.. autofunction:: rsgislib.imagecalc.calcindices.calcWBI
.. autofunction:: rsgislib.imagecalc.calcindices.calcNDWI
.. autofunction:: rsgislib.imagecalc.calcindices.calcGNDWI
.. autofunction:: rsgislib.imagecalc.calcindices.calcGMNDWI
.. autofunction:: rsgislib.imagecalc.calcindices.calcWhiteness
.. autofunction:: rsgislib.imagecalc.calcindices.calcBrightness
.. autofunction:: rsgislib.imagecalc.calcindices.calcBrightnessScaled
.. autofunction:: rsgislib.imagecalc.calcindices.calcCTVI

Other
------

.. autofunction:: rsgislib.imagecalc.countValsInCols
.. autofunction:: rsgislib.imagecalc.imagePixelColumnSummary
.. autofunction:: rsgislib.imagecalc.movementSpeed
.. autofunction:: rsgislib.imagecalc.unitArea
.. autofunction:: rsgislib.imagecalc.leastcostpath.performLeastCostPathCalc


* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`


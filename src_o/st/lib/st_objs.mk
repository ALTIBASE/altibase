#
# $Id$
#

STD_SRCS = $(ST_DIR)/std/stdUtils.cpp \
           $(ST_DIR)/std/stdPrimitive.cpp \
           $(ST_DIR)/std/stdParsing.cpp \
           $(ST_DIR)/std/stdMethod.cpp \
           $(ST_DIR)/std/stdGpc.cpp \
           $(ST_DIR)/std/stdGeometry.cpp \
           $(ST_DIR)/std/stdPolyClip.cpp

STV_SRCS = $(ST_DIR)/stv/stvNull2Geometry.cpp \
           $(ST_DIR)/stv/stvBinary2Geometry.cpp \
           $(ST_DIR)/stv/stvGeometry2Binary.cpp \
           $(ST_DIR)/stv/stvUndef2All.cpp 

STK_SRCS = $(ST_DIR)/stk/stk.cpp

STU_SRCS = $(ST_DIR)/stu/stuProperty.cpp

STI_SRCS = $(ST_DIR)/sti/sti.cpp

STI_STUB_SRCS = $(ST_DIR)/sti/stistub.cpp

STIX_SRCS = $(ST_DIR)/sti/stix.cpp

STF_SRCS = $(ST_DIR)/stf/stfSt_Geometry.cpp \
           $(ST_DIR)/stf/stfModules.cpp \
           $(ST_DIR)/stf/stfAnalysis.cpp \
           $(ST_DIR)/stf/stfBasic.cpp \
           $(ST_DIR)/stf/stfFunctions.cpp \
           $(ST_DIR)/stf/stfRelation.cpp \
           $(ST_DIR)/stf/stfWKT.cpp \
           $(ST_DIR)/stf/stfWKB.cpp \
           $(ST_DIR)/stf/stfAsBinary.cpp \
           $(ST_DIR)/stf/stfAsText.cpp \
           $(ST_DIR)/stf/stfBoundary.cpp \
           $(ST_DIR)/stf/stfBuffer.cpp \
           $(ST_DIR)/stf/stfContains.cpp \
           $(ST_DIR)/stf/stfConvexHull.cpp \
           $(ST_DIR)/stf/stfCoordX.cpp \
           $(ST_DIR)/stf/stfCoordY.cpp \
           $(ST_DIR)/stf/stfCrosses.cpp \
           $(ST_DIR)/stf/stfDifference.cpp \
           $(ST_DIR)/stf/stfDimension.cpp \
           $(ST_DIR)/stf/stfDisjoint.cpp \
           $(ST_DIR)/stf/stfDistance.cpp \
           $(ST_DIR)/stf/stfEnvelope.cpp \
           $(ST_DIR)/stf/stfEquals.cpp \
           $(ST_DIR)/stf/stfGeometryType.cpp \
           $(ST_DIR)/stf/stfIntersection.cpp \
           $(ST_DIR)/stf/stfIntersects.cpp \
           $(ST_DIR)/stf/stfIsEmpty.cpp \
           $(ST_DIR)/stf/stfIsSimple.cpp \
           $(ST_DIR)/stf/stfNotContains.cpp \
           $(ST_DIR)/stf/stfNotCrosses.cpp \
           $(ST_DIR)/stf/stfNotEquals.cpp \
           $(ST_DIR)/stf/stfNotOverlaps.cpp \
           $(ST_DIR)/stf/stfNotRelate.cpp \
           $(ST_DIR)/stf/stfNotTouches.cpp \
           $(ST_DIR)/stf/stfNotWithin.cpp \
           $(ST_DIR)/stf/stfOverlaps.cpp \
           $(ST_DIR)/stf/stfRelate.cpp \
           $(ST_DIR)/stf/stfSymDifference.cpp \
           $(ST_DIR)/stf/stfTouches.cpp \
           $(ST_DIR)/stf/stfUnion.cpp \
           $(ST_DIR)/stf/stfWithin.cpp \
           $(ST_DIR)/stf/stfStartPoint.cpp \
           $(ST_DIR)/stf/stfEndPoint.cpp \
           $(ST_DIR)/stf/stfIsRing.cpp \
           $(ST_DIR)/stf/stfIsClosed.cpp \
           $(ST_DIR)/stf/stfNumPoints.cpp \
           $(ST_DIR)/stf/stfPointN.cpp \
           $(ST_DIR)/stf/stfCentroid.cpp \
           $(ST_DIR)/stf/stfPointOnSurface.cpp \
           $(ST_DIR)/stf/stfArea.cpp \
           $(ST_DIR)/stf/stfExteriorRing.cpp \
           $(ST_DIR)/stf/stfNumInteriorRing.cpp \
           $(ST_DIR)/stf/stfInteriorRingN.cpp \
           $(ST_DIR)/stf/stfNumGeometries.cpp \
           $(ST_DIR)/stf/stfGeometryN.cpp \
           $(ST_DIR)/stf/stfLength.cpp \
           $(ST_DIR)/stf/stfGeomFromText.cpp \
           $(ST_DIR)/stf/stfPointFromText.cpp \
           $(ST_DIR)/stf/stfLineFromText.cpp \
           $(ST_DIR)/stf/stfPolyFromText.cpp \
           $(ST_DIR)/stf/stfRectFromText.cpp \
           $(ST_DIR)/stf/stfMPointFromText.cpp \
           $(ST_DIR)/stf/stfMLineFromText.cpp \
           $(ST_DIR)/stf/stfMPolyFromText.cpp \
           $(ST_DIR)/stf/stfGeoCollectionFromText.cpp \
           $(ST_DIR)/stf/stfGeomFromWKB.cpp \
           $(ST_DIR)/stf/stfPointFromWKB.cpp \
           $(ST_DIR)/stf/stfLineFromWKB.cpp \
           $(ST_DIR)/stf/stfLinestringFromWKB.cpp \
           $(ST_DIR)/stf/stfPolyFromWKB.cpp \
           $(ST_DIR)/stf/stfRectFromWKB.cpp \
           $(ST_DIR)/stf/stfMPointFromWKB.cpp \
           $(ST_DIR)/stf/stfMLineFromWKB.cpp \
           $(ST_DIR)/stf/stfMPolyFromWKB.cpp \
           $(ST_DIR)/stf/stfGeoCollectionFromWKB.cpp \
           $(ST_DIR)/stf/stfIsValid.cpp \
		   $(ST_DIR)/stf/stfIsValidHeader.cpp \
           $(ST_DIR)/stf/stfMinX.cpp \
           $(ST_DIR)/stf/stfMinY.cpp \
           $(ST_DIR)/stf/stfMaxX.cpp \
           $(ST_DIR)/stf/stfMaxY.cpp \
           $(ST_DIR)/stf/stfIsMBRIntersects.cpp  \
           $(ST_DIR)/stf/stfIsMBRContains.cpp  \
           $(ST_DIR)/stf/stfIsMBRWithin.cpp \
           $(ST_DIR)/stf/stfReverse.cpp \
           $(ST_DIR)/stf/stfMakeEnvelope.cpp \
           $(ST_DIR)/stf/stfEWKT.cpp \
           $(ST_DIR)/stf/stfAsEWKT.cpp \
           $(ST_DIR)/stf/stfAsEWKB.cpp \
           $(ST_DIR)/stf/stfGeomFromEWKT.cpp \
           $(ST_DIR)/stf/stfSRID.cpp \
           $(ST_DIR)/stf/stfSetSRID.cpp \
           $(ST_DIR)/stf/stfMakePoint.cpp \
           $(ST_DIR)/stf/stfMakeLine.cpp \
           $(ST_DIR)/stf/stfProj4.cpp \
           $(ST_DIR)/stf/stfTransform.cpp \
           $(ST_DIR)/stf/stfMakePolygon.cpp \
           $(ST_DIR)/stf/stfPolygon.cpp \
           $(ST_DIR)/stf/stfCollect.cpp \
           $(ST_DIR)/stf/stfIsCollection.cpp 

STM_SRCS = $(ST_DIR)/stm/stm.cpp \
           $(ST_DIR)/stm/stmFixedTable.cpp 

STN_SRCS = $(ST_DIR)/stn/stnIndexTypes.cpp

STIM_SRCS = $(ST_DIR)/sti/stim.cpp

STNMR_SRCS = $(ST_DIR)/stn/stnmr/stnmrModule.cpp    \
             $(ST_DIR)/stn/stnmr/stnmrFT.cpp

STNDR_SRCS = $(ST_DIR)/stn/stndr/stndrModule.cpp	\
             $(ST_DIR)/stn/stndr/stndrUpdate.cpp	\
             $(ST_DIR)/stn/stndr/stndrTDBuild.cpp	\
             $(ST_DIR)/stn/stndr/stndrFT.cpp	    \
             $(ST_DIR)/stn/stndr/stndrStackMgr.cpp	\
             $(ST_DIR)/stn/stndr/stndrBUBuild.cpp

ST_SRCS =  $(STD_SRCS) $(STV_SRCS) $(STU_SRCS) $(STI_SRCS) $(STF_SRCS) $(STM_SRCS) $(STN_SRCS) $(STNMR_SRCS) $(STK_SRCS) $(STIX_SRCS) $(STNDR_SRCS)

ST_OBJS        = $(ST_SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))
ST_SHOBJS      = $(ST_SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%_shobj.$(OBJEXT))

ST_STUB_OBJS   = $(STI_STUB_SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))
ST_STUB_SHOBJS = $(STI_STUB_SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%_shobj.$(OBJEXT))

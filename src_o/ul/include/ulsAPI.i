
/*----------------------------------------------------------------*
 *  Environment Handle Management
 *----------------------------------------------------------------*/

/* Alloc Environment */
ACSRETURN ACSAllocEnv( ACSHENV * aHandle );

/* Free Environment */
ACSRETURN ACSFreeEnv( ACSHENV aHandle );

/* Get Error Information */
ACSRETURN ACSError( ACSHENV       aHandle,
                    SQLUINTEGER * aErrorCode,
                    SQLSCHAR   ** aErrorMessage,
                    SQLSMALLINT * aErrorMessageLength );

/*----------------------------------------------------------------*
 *  Create Objects
 *----------------------------------------------------------------*/

/* Create 2D Point Object */
ACSRETURN ACSCreatePoint2D( ACSHENV                   aHandle,
                            stdGeometryType         * aBuffer,
                            SQLLEN                    aBufferSize,
                            stdPoint2D              * aPoint,
                            SQLINTEGER                aSRID,
                            SQLLEN                  * aObjLength );

/* Create 2D LineString Object */
ACSRETURN ACSCreateLineString2D( ACSHENV             aHandle,
                                 stdGeometryType   * aBuffer,
                                 SQLLEN              aBufferSize,
                                 SQLUINTEGER         aNumPoints,
                                 stdPoint2D        * aPoints,
                                 SQLINTEGER          aSRID,
                                 SQLLEN            * aObjLength );

/* Create 2D LinearRing Object */
ACSRETURN ACSCreateLinearRing2D(  ACSHENV               aHandle,
                                  stdLinearRing2D     * aBuffer,
                                  SQLLEN                aBufferSize,
                                  SQLUINTEGER           aNumPoints,
                                  stdPoint2D          * aPoints,
                                  SQLLEN              * aObjLength );

/* Create 2D Polygon Object */
ACSRETURN ACSCreatePolygon2D( ACSHENV                  aHandle,
                              stdGeometryType        * aBuffer,
                              SQLLEN                   aBufferSize,
                              SQLUINTEGER              aNumRings,
                              stdLinearRing2D       ** aRings,
                              SQLINTEGER               aSRID,
                              SQLLEN                 * aObjLength );

/* Create 2D MultiPoint Object */
ACSRETURN ACSCreateMultiPoint2D( ACSHENV                   aHandle,
                                 stdGeometryType         * aBuffer,
                                 SQLLEN                    aBufferSize,
                                 SQLUINTEGER               aNumPoints,
                                 stdPoint2DType        ** aPoints,
                                 SQLLEN                 * aObjLength );

/* Create 2D MultiLineString Object */
ACSRETURN ACSCreateMultiLineString2D( ACSHENV                   aHandle,
                                      stdGeometryType         * aBuffer,
                                      SQLLEN                    aBufferSize,
                                      SQLUINTEGER               aNumLineStrings,
                                      stdLineString2DType    ** aLineStrings,
                                      SQLLEN                  * aObjLength );

/* Create 2D MultiPolygon Object */
ACSRETURN ACSCreateMultiPolygon2D( ACSHENV                   aHandle,
                                   stdGeometryType         * aBuffer,
                                   SQLLEN                    aBufferSize,
                                   SQLUINTEGER               aNumPolygons,
                                   stdPolygon2DType       ** aPolygons,
                                   SQLLEN                  * aObjLength );

/* Create 2D GeometryCollection Object */
ACSRETURN ACSCreateGeomCollection2D( ACSHENV                   aHandle,
                                     stdGeometryType         * aBuffer,
                                     SQLLEN                    aBufferSize,
                                     SQLUINTEGER               aNumGeometries,
                                     stdGeometryType        ** aGeometries,
                                     SQLLEN                  * aObjLength );

/*----------------------------------------------------------------*
 *  Geometry Infomation Search Function
 *----------------------------------------------------------------*/

/*----------------------------------------------------------------*
 *  Geometry Infomation Search Function  : Geometry
 *----------------------------------------------------------------*/
/* Get GeometryType */
ACSRETURN ACSGetGeometryType( ACSHENV                   aHandle,
                              stdGeometryType         * aGeometry,
                              stdGeoTypes             * aGeoType );
/* Get Geometry Size */
ACSRETURN ACSGetGeometrySize( ACSHENV                  aHandle,
                              stdGeometryType        * aGeometry,
                              SQLLEN                 * aSize );

/* Get Geometry Size from WKB Object */
ACSRETURN ACSGetGeometrySizeFromWKB( ACSHENV       aHandle,
                                     SQLCHAR     * aWKB,
                                     SQLUINTEGER   aWKBLength,
                                     SQLLEN      * aGeometrySize );

/* Get Geometry SRID */
ACSRETURN ACSGetGeometrySRID( ACSHENV            aHandle,
                              stdGeometryType  * aGeometry,
                              SQLINTEGER       * aSRID );

/*----------------------------------------------------------------*
 *  Geometry Infomation Search Function  
 *    : 2D & 3D MultiPoint, MultiLineString, MultiGeometryCollection
 *----------------------------------------------------------------*/
/* Get Geometries Num */
ACSRETURN ACSGetNumGeometries( ACSHENV                   aHandle,
                               stdGeometryType         * aGeometry,
                               SQLUINTEGER             * aNumGeometries );

/* Get Nth Geometry */
ACSRETURN ACSGetGeometryN( ACSHENV                   aHandle,
                           stdGeometryType         * aGeometry,
                           SQLUINTEGER               aNth,
                           stdGeometryType        ** aSubGeometry );

/*----------------------------------------------------------------*
 *  Geometry Infomation Search Function  
 *    : 2D Polygon 
 *----------------------------------------------------------------*/
/* Get Exterior Ring : 2D Polygon */
ACSRETURN ACSGetExteriorRing2D( ACSHENV                    aHandle,
                                stdPolygon2DType         * aPolygon,
                                stdLinearRing2D         ** aLinearRing );

/* Get InteriorRing Num : 2D Polygon */
ACSRETURN ACSGetNumInteriorRing2D( ACSHENV                   aHandle,
                                   stdPolygon2DType        * aPolygon,
                                   SQLUINTEGER             * aNumInterinor );

/* Get Nth InteriorRing : 2D Polygon */
ACSRETURN ACSGetInteriorRingNPolygon2D( ACSHENV                aHandle,
                                        stdPolygon2DType     * aPolygon,
                                        SQLUINTEGER            aNth,
                                        stdLinearRing2D     ** aLinearRing );

/*----------------------------------------------------------------*
 *  Geometry Infomation Search Function  
 *    : 2D LineString
 *----------------------------------------------------------------*/
/* Get Points num : 2D LineString */
ACSRETURN ACSGetNumPointsLineString2D( ACSHENV                 aHandle,
                                       stdLineString2DType   * aLineString,
                                       SQLUINTEGER           * aNumPoints );

/* Get Nth Point : 2D LineString */
ACSRETURN ACSGetPointNLineString2D( ACSHENV                 aHandle,
                                    stdLineString2DType   * aLineString,
                                    SQLUINTEGER             aNth, 
                                    stdPoint2D            * aPoint );

/* Get Points pointer : 2D LineString */
ACSRETURN ACSGetPointsLineString2D( ACSHENV                 aHandle,
                                    stdLineString2DType   * aLineString,
                                    stdPoint2D           ** aPoints );


/*----------------------------------------------------------------*
 *  Geometry Infomation Search Function  
 *    : 2D LinearRing
 *----------------------------------------------------------------*/
/* Get Points num : 2D LinearRing */
ACSRETURN ACSGetNumPointsLinearRing2D( ACSHENV                 aHandle,
                                       stdLinearRing2D       * aLinearRing,
                                       SQLUINTEGER           * aNumPoints );

/* Get Nth Point : 2D LinearRing */
ACSRETURN ACSGetPointNLinearRing2D( ACSHENV                 aHandle,
                                    stdLinearRing2D       * aLinearRing,
                                    SQLUINTEGER             aNth, 
                                    stdPoint2D            * aPoint );

/* Get Points pointer : 2D LinearRing */
ACSRETURN ACSGetPointsLinearRing2D( ACSHENV                 aHandle,
                                    stdLinearRing2D       * aLinearRing,
                                    stdPoint2D           ** aPoints );

/*----------------------------------------------------------------*
 *  Byte Order Interface
 *----------------------------------------------------------------*/

/* Adjust Byte Order */
ACSRETURN ACSAdjustByteOrder( ACSHENV           aHandle,
                              stdGeometryType * aObject );

/* Change Byte Order */
ACSRETURN ACSEndian( ACSHENV             aHandle,
                     stdGeometryType   * aObject );







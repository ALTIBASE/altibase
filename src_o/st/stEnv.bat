echo config.status: creating st/stEnv.mk

echo include %DEV_DIR%\env.mk>%DEV_DIR%\st\stEnv.mk
echo include %ST_DIR%\src\lib\st_srcs.mk>>%DEV_DIR%\st\stEnv.mk

echo INCLUDES=/I%ST_DIR%\src\include /I%MT_DIR%\include $(INCLUDES) /I. >>%DEV_DIR%\st\stEnv.mk

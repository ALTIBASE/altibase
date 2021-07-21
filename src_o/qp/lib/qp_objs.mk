#
# $Id: qp_objs.mk 89988 2021-02-15 02:01:46Z khkwak $
#

QCI_SRCS = $(QP_DIR)/qci/qci.cpp  \
           $(QP_DIR)/qci/qciMisc.cpp

QCG_SRCS = $(QP_DIR)/qcg/qcg.cpp \
           $(QP_DIR)/qcg/qcgPlan.cpp

QCP_SRCS = $(QP_DIR)/qcp/qcply.cpp \
           $(QP_DIR)/qcp/qcpll.cpp \
           $(QP_DIR)/qcp/qcphl.cpp \
           $(QP_DIR)/qcp/qcphy.cpp \
           $(QP_DIR)/qcp/qcpUtil.cpp \
           $(QP_DIR)/qcp/qcpManager.cpp \
           $(QP_DIR)/qcp/qcphManager.cpp

QCU_SRCS = $(QP_DIR)/qcu/qcuProperty.cpp \
           $(QP_DIR)/qcu/qcuSqlSourceInfo.cpp \
           $(QP_DIR)/qcu/qcuTraceCode.cpp \
           $(QP_DIR)/qcu/qcuSessionObj.cpp \
           $(QP_DIR)/qcu/qcuTemporaryObj.cpp \
           $(QP_DIR)/qcu/qcuSessionPkg.cpp

QCC_SRCS = $(QP_DIR)/qcc/qcc.cpp

QCD_SRCS = $(QP_DIR)/qcd/qcd.cpp

QDB_SRCS = $(QP_DIR)/qdb/qdbCommon.cpp \
           $(QP_DIR)/qdb/qdbCreate.cpp \
           $(QP_DIR)/qdb/qdbAlter.cpp \
           $(QP_DIR)/qdb/qdbComment.cpp \
           $(QP_DIR)/qdb/qdbFlashback.cpp \
           $(QP_DIR)/qdb/qdbDisjoin.cpp \
           $(QP_DIR)/qdb/qdbJoin.cpp \
           $(QP_DIR)/qdb/qdbCopySwap.cpp

QDC_SRCS = $(QP_DIR)/qdc/qdc.cpp \
           $(QP_DIR)/qdc/qdcDir.cpp \
           $(QP_DIR)/qdc/qdcLibrary.cpp \
           $(QP_DIR)/qdc/qdcAudit.cpp \
           $(QP_DIR)/qdc/qdcJob.cpp

QDD_SRCS = $(QP_DIR)/qdd/qdd.cpp

QDR_SRCS = $(QP_DIR)/qdr/qdr.cpp

QDS_SRCS = $(QP_DIR)/qds/qds.cpp \
           $(QP_DIR)/qds/qdsSynonym.cpp

QDT_SRCS = $(QP_DIR)/qdt/qdtCommon.cpp \
           $(QP_DIR)/qdt/qdtCreate.cpp \
           $(QP_DIR)/qdt/qdtAlter.cpp \
           $(QP_DIR)/qdt/qdtDrop.cpp

QDX_SRCS = $(QP_DIR)/qdx/qdx.cpp

QDV_SRCS = $(QP_DIR)/qdv/qdv.cpp

QDM_SRCS = $(QP_DIR)/qdm/qdm.cpp

QMS_SRCS = $(QP_DIR)/qms/qmsParseTree.cpp \
           $(QP_DIR)/qms/qmsIndexTable.cpp \
           $(QP_DIR)/qms/qmsDefaultExpr.cpp \
           $(QP_DIR)/qms/qmsPreservedTable.cpp

QDK_SRCS = $(QP_DIR)/qdk/qdk.cpp

QMC_SRCS = $(QP_DIR)/qmc/qmc.cpp \
           $(QP_DIR)/qmc/qmcMemory.cpp \
           $(QP_DIR)/qmc/qmcTempTableMgr.cpp \
           $(QP_DIR)/qmc/qmcHashTempTable.cpp \
           $(QP_DIR)/qmc/qmcMemHashTempTable.cpp \
           $(QP_DIR)/qmc/qmcMemPartHashTempTable.cpp \
           $(QP_DIR)/qmc/qmcDiskHashTempTable.cpp \
           $(QP_DIR)/qmc/qmcSortTempTable.cpp \
           $(QP_DIR)/qmc/qmcMemSortTempTable.cpp \
           $(QP_DIR)/qmc/qmcDiskSortTempTable.cpp \
           $(QP_DIR)/qmc/qmcCursor.cpp \
           $(QP_DIR)/qmc/qmcInsertCursor.cpp \
           $(QP_DIR)/qmc/qmcThr.cpp

QMV_SRCS = $(QP_DIR)/qmv/qmv.cpp \
           $(QP_DIR)/qmv/qmvQuerySet.cpp \
           $(QP_DIR)/qmv/qmvOrderBy.cpp \
           $(QP_DIR)/qmv/qmvQTC.cpp \
           $(QP_DIR)/qmv/qmvGBGSTransform.cpp \
           $(QP_DIR)/qmv/qmvPivotTransform.cpp \
           $(QP_DIR)/qmv/qmvHierTransform.cpp \
           $(QP_DIR)/qmv/qmvAvgTransform.cpp \
           $(QP_DIR)/qmv/qmvTableFuncTransform.cpp \
           $(QP_DIR)/qmv/qmvWith.cpp \
           $(QP_DIR)/qmv/qmvShardTransform.cpp \
           $(QP_DIR)/qmv/qmvUnpivotTransform.cpp

QMO_SRCS = $(QP_DIR)/qmo/qmo.cpp              \
           $(QP_DIR)/qmo/qmoCost.cpp          \
           $(QP_DIR)/qmo/qmoNonCrtPathMgr.cpp \
           $(QP_DIR)/qmo/qmoCrtPathMgr.cpp    \
           $(QP_DIR)/qmo/qmoDnfMgr.cpp        \
           $(QP_DIR)/qmo/qmoCnfMgr.cpp        \
           $(QP_DIR)/qmo/qmoJoinMethod.cpp    \
           $(QP_DIR)/qmo/qmoOneNonPlan.cpp    \
           $(QP_DIR)/qmo/qmoOneMtrPlan.cpp    \
           $(QP_DIR)/qmo/qmoTwoNonPlan.cpp    \
           $(QP_DIR)/qmo/qmoTwoMtrPlan.cpp    \
           $(QP_DIR)/qmo/qmoMultiNonPlan.cpp  \
           $(QP_DIR)/qmo/qmoParallelPlan.cpp  \
           $(QP_DIR)/qmo/qmoDependency.cpp    \
           $(QP_DIR)/qmo/qmoNormalForm.cpp    \
           $(QP_DIR)/qmo/qmoPredicate.cpp     \
           $(QP_DIR)/qmo/qmoRidPred.cpp       \
           $(QP_DIR)/qmo/qmoSubquery.cpp      \
           $(QP_DIR)/qmo/qmoUnnesting.cpp     \
           $(QP_DIR)/qmo/qmoStatMgr.cpp       \
           $(QP_DIR)/qmo/qmoKeyRange.cpp      \
           $(QP_DIR)/qmo/qmoUtil.cpp          \
           $(QP_DIR)/qmo/qmoPartition.cpp     \
           $(QP_DIR)/qmo/qmoTransitivity.cpp  \
           $(QP_DIR)/qmo/qmoViewMerging.cpp   \
           $(QP_DIR)/qmo/qmoConstExpr.cpp     \
           $(QP_DIR)/qmo/qmoPushPred.cpp      \
           $(QP_DIR)/qmo/qmoOuterJoinOper.cpp \
           $(QP_DIR)/qmo/qmoAnsiJoinOrder.cpp \
           $(QP_DIR)/qmo/qmoOuterJoinElimination.cpp \
           $(QP_DIR)/qmo/qmoRownumPredToLimit.cpp \
           $(QP_DIR)/qmo/qmoListTransform.cpp \
           $(QP_DIR)/qmo/qmoCSETransform.cpp  \
           $(QP_DIR)/qmo/qmoCFSTransform.cpp  \
           $(QP_DIR)/qmo/qmoOBYETransform.cpp \
           $(QP_DIR)/qmo/qmoSelectivity.cpp   \
           $(QP_DIR)/qmo/qmoCheckViewColumnRef.cpp \
           $(QP_DIR)/qmo/qmoDistinctElimination.cpp \
           $(QP_DIR)/qmo/qmoInnerJoinPushDown.cpp

QMG_SRCS = $(QP_DIR)/qmg/qmg.cpp             \
           $(QP_DIR)/qmg/qmgSelection.cpp    \
           $(QP_DIR)/qmg/qmgProjection.cpp   \
           $(QP_DIR)/qmg/qmgDistinction.cpp  \
           $(QP_DIR)/qmg/qmgGrouping.cpp     \
           $(QP_DIR)/qmg/qmgSorting.cpp      \
           $(QP_DIR)/qmg/qmgJoin.cpp         \
           $(QP_DIR)/qmg/qmgSemiJoin.cpp     \
           $(QP_DIR)/qmg/qmgAntiJoin.cpp     \
           $(QP_DIR)/qmg/qmgLeftOuter.cpp    \
           $(QP_DIR)/qmg/qmgFullOuter.cpp    \
           $(QP_DIR)/qmg/qmgSet.cpp          \
           $(QP_DIR)/qmg/qmgHierarchy.cpp    \
           $(QP_DIR)/qmg/qmgDnf.cpp          \
           $(QP_DIR)/qmg/qmgPartition.cpp    \
           $(QP_DIR)/qmg/qmgCounting.cpp     \
           $(QP_DIR)/qmg/qmgWindowing.cpp    \
           $(QP_DIR)/qmg/qmgInsert.cpp       \
           $(QP_DIR)/qmg/qmgMultiInsert.cpp  \
           $(QP_DIR)/qmg/qmgUpdate.cpp       \
           $(QP_DIR)/qmg/qmgDelete.cpp       \
           $(QP_DIR)/qmg/qmgMove.cpp         \
           $(QP_DIR)/qmg/qmgMerge.cpp        \
           $(QP_DIR)/qmg/qmgShardSelect.cpp  \
           $(QP_DIR)/qmg/qmgShardDML.cpp     \
           $(QP_DIR)/qmg/qmgShardInsert.cpp  \
           $(QP_DIR)/qmg/qmgSetRecursive.cpp

QMN_SRCS = $(QP_DIR)/qmn/qmn.cpp \
           $(QP_DIR)/qmn/qmnScan.cpp \
           $(QP_DIR)/qmn/qmnProject.cpp \
           $(QP_DIR)/qmn/qmnView.cpp \
           $(QP_DIR)/qmn/qmnFilter.cpp \
           $(QP_DIR)/qmn/qmnSort.cpp \
           $(QP_DIR)/qmn/qmnLimitSort.cpp \
           $(QP_DIR)/qmn/qmnHash.cpp \
           $(QP_DIR)/qmn/qmnHashDist.cpp \
           $(QP_DIR)/qmn/qmnGroupBy.cpp \
           $(QP_DIR)/qmn/qmnGroupAgg.cpp \
           $(QP_DIR)/qmn/qmnCountAsterisk.cpp \
           $(QP_DIR)/qmn/qmnAggregation.cpp \
           $(QP_DIR)/qmn/qmnConcatenate.cpp \
           $(QP_DIR)/qmn/qmnJoin.cpp \
           $(QP_DIR)/qmn/qmnMergeJoin.cpp \
           $(QP_DIR)/qmn/qmnMultiBagUnion.cpp \
           $(QP_DIR)/qmn/qmnSetIntersect.cpp \
           $(QP_DIR)/qmn/qmnSetDifference.cpp \
           $(QP_DIR)/qmn/qmnLeftOuter.cpp \
           $(QP_DIR)/qmn/qmnAntiOuter.cpp \
           $(QP_DIR)/qmn/qmnFullOuter.cpp \
           $(QP_DIR)/qmn/qmnViewMaterialize.cpp \
           $(QP_DIR)/qmn/qmnViewScan.cpp \
           $(QP_DIR)/qmn/qmnPartitionCoord.cpp \
           $(QP_DIR)/qmn/qmnCounter.cpp \
           $(QP_DIR)/qmn/qmnWindowSort.cpp \
           $(QP_DIR)/qmn/qmnConnectBy.cpp \
           $(QP_DIR)/qmn/qmnConnectByMTR.cpp \
           $(QP_DIR)/qmn/qmnInsert.cpp \
           $(QP_DIR)/qmn/qmnMultiInsert.cpp \
           $(QP_DIR)/qmn/qmnUpdate.cpp \
           $(QP_DIR)/qmn/qmnDelete.cpp \
           $(QP_DIR)/qmn/qmnMove.cpp \
           $(QP_DIR)/qmn/qmnMerge.cpp \
           $(QP_DIR)/qmn/qmnRollup.cpp \
           $(QP_DIR)/qmn/qmnCube.cpp \
           $(QP_DIR)/qmn/qmnPRLQ.cpp \
           $(QP_DIR)/qmn/qmnPPCRD.cpp \
           $(QP_DIR)/qmn/qmnPSCRD.cpp \
           $(QP_DIR)/qmn/qmnSetRecursive.cpp \
           $(QP_DIR)/qmn/qmnDelay.cpp \
           $(QP_DIR)/qmn/qmnShardSelect.cpp \
           $(QP_DIR)/qmn/qmnShardDML.cpp \
           $(QP_DIR)/qmn/qmnShardInsert.cpp \

QMX_SRCS = $(QP_DIR)/qmx/qmx.cpp \
           $(QP_DIR)/qmx/qmxSessionCache.cpp \
           $(QP_DIR)/qmx/qmxSimple.cpp \
           $(QP_DIR)/qmx/qmxResultCache.cpp \
           $(QP_DIR)/qmx/qmxShard.cpp

QMR_SRCS = $(QP_DIR)/qmr/qmr.cpp

QDP_SRCS = $(QP_DIR)/qdp/qdpGrant.cpp                   \
           $(QP_DIR)/qdp/qdpRevoke.cpp                  \
           $(QP_DIR)/qdp/qdpDrop.cpp                    \
           $(QP_DIR)/qdp/qdpPrivilege.cpp               \
           $(QP_DIR)/qdp/qdpRole.cpp


QCM_SRCS = $(QP_DIR)/qcm/qcm.cpp \
           $(QP_DIR)/qcm/qcmFixedTable.cpp \
           $(QP_DIR)/qcm/qcmPerformanceView.cpp \
           $(QP_DIR)/qcm/qcmCreate.cpp \
           $(QP_DIR)/qcm/qcmCache.cpp \
           $(QP_DIR)/qcm/qcmUser.cpp \
           $(QP_DIR)/qcm/qcmDNUser.cpp \
           $(QP_DIR)/qcm/qcmProc.cpp \
           $(QP_DIR)/qcm/qcmPkg.cpp \
           $(QP_DIR)/qcm/qcmPriv.cpp \
           $(QP_DIR)/qcm/qcmTrigger.cpp \
           $(QP_DIR)/qcm/qcmDatabase.cpp \
           $(QP_DIR)/qcm/qcmView.cpp \
           $(QP_DIR)/qcm/qcmMView.cpp \
           $(QP_DIR)/qcm/qcmTableSpace.cpp \
           $(QP_DIR)/qcm/qcmXA.cpp \
           $(QP_DIR)/qcm/qcmSynonym.cpp \
           $(QP_DIR)/qcm/qcmDirectory.cpp \
           $(QP_DIR)/qcm/qcmDump.cpp \
           $(QP_DIR)/qcm/qcmTableInfoMgr.cpp \
           $(QP_DIR)/qcm/qcmPartition.cpp \
           $(QP_DIR)/qcm/qcmPassword.cpp \
           $(QP_DIR)/qcm/qcmLibrary.cpp \
           $(QP_DIR)/qcm/qcmDatabaseLinks.cpp \
           $(QP_DIR)/qcm/qcmAudit.cpp \
           $(QP_DIR)/qcm/qcmDictionary.cpp \
           $(QP_DIR)/qcm/qcmJob.cpp


QDN_SRCS = $(QP_DIR)/qdn/qdn.cpp            \
           $(QP_DIR)/qdn/qdnCheck.cpp       \
           $(QP_DIR)/qdn/qdnForeignKey.cpp  \
           $(QP_DIR)/qdn/qdnTrigger.cpp


QRC_SRCS = $(QP_DIR)/qrc/qrc.cpp

QSF_SRCS = $(QP_DIR)/qsf/qsf.cpp \
           $(QP_DIR)/qsf/qsfPrint.cpp \
           $(QP_DIR)/qsf/qsfFOpen.cpp \
           $(QP_DIR)/qsf/qsfFGetLine.cpp \
           $(QP_DIR)/qsf/qsfFPut.cpp \
           $(QP_DIR)/qsf/qsfFClose.cpp \
           $(QP_DIR)/qsf/qsfFFlush.cpp \
           $(QP_DIR)/qsf/qsfFRemove.cpp \
           $(QP_DIR)/qsf/qsfFRename.cpp \
           $(QP_DIR)/qsf/qsfFCopy.cpp \
           $(QP_DIR)/qsf/qsfFCloseAll.cpp \
           $(QP_DIR)/qsf/qsfSendmsg.cpp \
           $(QP_DIR)/qsf/qsfRaiseAppErr.cpp \
           $(QP_DIR)/qsf/qsfUserID.cpp \
           $(QP_DIR)/qsf/qsfSessionID.cpp \
           $(QP_DIR)/qsf/qsfUserName.cpp \
           $(QP_DIR)/qsf/qsfMCount.cpp \
           $(QP_DIR)/qsf/qsfMDelete.cpp \
           $(QP_DIR)/qsf/qsfMExists.cpp \
           $(QP_DIR)/qsf/qsfMFirst.cpp \
           $(QP_DIR)/qsf/qsfMLast.cpp \
           $(QP_DIR)/qsf/qsfMNext.cpp \
           $(QP_DIR)/qsf/qsfMPrior.cpp \
           $(QP_DIR)/qsf/qsfSleep.cpp \
           $(QP_DIR)/qsf/qsfRemoveXid.cpp \
           $(QP_DIR)/qsf/qsfMutexLock.cpp \
           $(QP_DIR)/qsf/qsfMutexUnlock.cpp \
           $(QP_DIR)/qsf/dbms_stats/qsfGatherSystemStats.cpp \
           $(QP_DIR)/qsf/dbms_stats/qsfGatherTableStats.cpp \
           $(QP_DIR)/qsf/dbms_stats/qsfGatherIndexStats.cpp \
           $(QP_DIR)/qsf/dbms_stats/qsfSetSystemStats.cpp \
           $(QP_DIR)/qsf/dbms_stats/qsfSetTableStats.cpp \
           $(QP_DIR)/qsf/dbms_stats/qsfSetIndexStats.cpp \
           $(QP_DIR)/qsf/dbms_stats/qsfSetColumnStats.cpp \
           $(QP_DIR)/qsf/dbms_stats/qsfGetSystemStats.cpp \
           $(QP_DIR)/qsf/dbms_stats/qsfGetTableStats.cpp \
           $(QP_DIR)/qsf/dbms_stats/qsfGetIndexStats.cpp \
           $(QP_DIR)/qsf/dbms_stats/qsfGetColumnStats.cpp \
           $(QP_DIR)/qsf/dbms_stats/qsfDeleteSystemStats.cpp \
           $(QP_DIR)/qsf/dbms_stats/qsfDeleteTableStats.cpp \
           $(QP_DIR)/qsf/dbms_stats/qsfDeleteIndexStats.cpp \
           $(QP_DIR)/qsf/dbms_stats/qsfDeleteColumnStats.cpp \
           $(QP_DIR)/qsf/qsfConnectByRoot.cpp \
           $(QP_DIR)/qsf/qsfSysConnectByPath.cpp \
           $(QP_DIR)/qsf/qsfSessionTZ.cpp \
           $(QP_DIR)/qsf/qsfDBTZ.cpp \
           $(QP_DIR)/qsf/qsfGrouping.cpp \
           $(QP_DIR)/qsf/qsfGroupingID.cpp \
           $(QP_DIR)/qsf/qsfLastModifiedProwid.cpp \
           $(QP_DIR)/qsf/qsfRowCount.cpp \
           $(QP_DIR)/qsf/qsfMemoryDump.cpp \
           $(QP_DIR)/qsf/dbms_concurrent_exec/qsfConcurrentRequest.cpp \
           $(QP_DIR)/qsf/dbms_concurrent_exec/qsfConcurrentWait.cpp \
           $(QP_DIR)/qsf/dbms_concurrent_exec/qsfConcurrentInitialize.cpp \
           $(QP_DIR)/qsf/dbms_concurrent_exec/qsfConcurrentFinalize.cpp \
           $(QP_DIR)/qsf/dbms_concurrent_exec/qsfConcurrentError.cpp \
           $(QP_DIR)/qsf/qsfSecureHashB64.cpp \
           $(QP_DIR)/qsf/qsfSecureHashStr.cpp \
           $(QP_DIR)/qsf/qsfCloseAllConnect.cpp \
           $(QP_DIR)/qsf/qsfCloseConnect.cpp \
           $(QP_DIR)/qsf/qsfWriteraw.cpp \
           $(QP_DIR)/qsf/qsfOpenConnect.cpp \
           $(QP_DIR)/qsf/qsfIsConnect.cpp \
           $(QP_DIR)/qsf/qsfTableFuncElement.cpp \
           $(QP_DIR)/qsf/qsfUserLockRequest.cpp \
           $(QP_DIR)/qsf/qsfUserLockRelease.cpp \
           $(QP_DIR)/qsf/qsfIsArrayBound.cpp \
           $(QP_DIR)/qsf/qsfIsFirstArrayBound.cpp \
           $(QP_DIR)/qsf/qsfIsLastArrayBound.cpp \
           $(QP_DIR)/qsf/qsfSetClientAppInfo.cpp \
           $(QP_DIR)/qsf/qsfSetModule.cpp \
           $(QP_DIR)/qsf/dbms_sql/qsfOpenCursor.cpp \
           $(QP_DIR)/qsf/dbms_sql/qsfIsOpen.cpp \
           $(QP_DIR)/qsf/dbms_sql/qsfParse.cpp \
           $(QP_DIR)/qsf/dbms_sql/qsfBindVariableVarchar.cpp \
           $(QP_DIR)/qsf/dbms_sql/qsfBindVariableChar.cpp \
           $(QP_DIR)/qsf/dbms_sql/qsfBindVariableInteger.cpp \
           $(QP_DIR)/qsf/dbms_sql/qsfBindVariableBigint.cpp \
           $(QP_DIR)/qsf/dbms_sql/qsfBindVariableSmallint.cpp \
           $(QP_DIR)/qsf/dbms_sql/qsfBindVariableDouble.cpp \
           $(QP_DIR)/qsf/dbms_sql/qsfBindVariableReal.cpp \
           $(QP_DIR)/qsf/dbms_sql/qsfBindVariableNumeric.cpp \
           $(QP_DIR)/qsf/dbms_sql/qsfBindVariableDate.cpp \
           $(QP_DIR)/qsf/dbms_sql/qsfExecuteCursor.cpp \
           $(QP_DIR)/qsf/dbms_sql/qsfFetchRows.cpp \
           $(QP_DIR)/qsf/dbms_sql/qsfColumnValueVarchar.cpp \
           $(QP_DIR)/qsf/dbms_sql/qsfColumnValueChar.cpp \
           $(QP_DIR)/qsf/dbms_sql/qsfColumnValueInteger.cpp \
           $(QP_DIR)/qsf/dbms_sql/qsfColumnValueBigint.cpp \
           $(QP_DIR)/qsf/dbms_sql/qsfColumnValueSmallint.cpp \
           $(QP_DIR)/qsf/dbms_sql/qsfColumnValueDouble.cpp \
           $(QP_DIR)/qsf/dbms_sql/qsfColumnValueReal.cpp \
           $(QP_DIR)/qsf/dbms_sql/qsfColumnValueNumeric.cpp \
           $(QP_DIR)/qsf/dbms_sql/qsfColumnValueDate.cpp \
           $(QP_DIR)/qsf/dbms_sql/qsfCloseCursor.cpp \
           $(QP_DIR)/qsf/dbms_sql/qsfLastErrorPosition.cpp \
           $(QP_DIR)/qsf/dbms_alert/qsfRegister.cpp \
           $(QP_DIR)/qsf/dbms_alert/qsfWaitOne.cpp \
           $(QP_DIR)/qsf/dbms_alert/qsfWaitAny.cpp \
           $(QP_DIR)/qsf/dbms_alert/qsfSignal.cpp \
           $(QP_DIR)/qsf/dbms_alert/qsfSetDefaults.cpp \
           $(QP_DIR)/qsf/dbms_alert/qsfRemoveAll.cpp \
           $(QP_DIR)/qsf/dbms_alert/qsfRemove.cpp \
           $(QP_DIR)/qsf/dbms_utility/qsfFormatCallStack.cpp \
           $(QP_DIR)/qsf/dbms_utility/qsfFormatErrorBacktrace.cpp \
           $(QP_DIR)/qsf/qsfRandom.cpp \
           $(QP_DIR)/qsf/qsfWriteRawValue.cpp \
           $(QP_DIR)/qsf/qsfSendText.cpp \
           $(QP_DIR)/qsf/qsfRecvText.cpp \
           $(QP_DIR)/qsf/qsfCheckConnectState.cpp \
           $(QP_DIR)/qsf/qsfCheckConnectReply.cpp \
           $(QP_DIR)/qsf/qsfSetPrevMetaVersion.cpp \
           $(QP_DIR)/qsf/dbms_sql_plan_cache/qsfKeepPlan.cpp \
           $(QP_DIR)/qsf/dbms_sql_plan_cache/qsfUnkeepPlan.cpp \
           $(QP_DIR)/qsf/qsfAddUserSrs.cpp \
           $(QP_DIR)/qsf/qsfDeleteUserSrs.cpp \
		   $(QP_DIR)/qsf/dbms_standard/qsfInserting.cpp \
		   $(QP_DIR)/qsf/dbms_standard/qsfUpdating.cpp \
		   $(QP_DIR)/qsf/dbms_standard/qsfDeleting.cpp \
           $(QP_DIR)/qsf/qsfInvokeUserID.cpp \
           $(QP_DIR)/qsf/qsfInvokeUserName.cpp \
           $(QP_DIR)/qsf/dbms_shard_get_diagnostics/qsfGetErrorCode.cpp \
           $(QP_DIR)/qsf/dbms_shard_get_diagnostics/qsfGetErrorMessage.cpp \
           $(QP_DIR)/qsf/dbms_shard_get_diagnostics/qsfGetErrorCount.cpp \
           $(QP_DIR)/qsf/dbms_shard_get_diagnostics/qsfGetErrorNodeID.cpp \
           $(QP_DIR)/qsf/dbms_shard_get_diagnostics/qsfGetErrorSeqNo.cpp

QSV_SRCS = $(QP_DIR)/qsv/qsv.cpp \
           $(QP_DIR)/qsv/qsvProcStmts.cpp \
           $(QP_DIR)/qsv/qsvPkgStmts.cpp\
           $(QP_DIR)/qsv/qsvEnv.cpp \
           $(QP_DIR)/qsv/qsvCursor.cpp \
           $(QP_DIR)/qsv/qsvProcVar.cpp \
           $(QP_DIR)/qsv/qsvProcType.cpp \
           $(QP_DIR)/qsv/qsvRefCursor.cpp

QSO_SRCS = $(QP_DIR)/qso/qso.cpp \
           $(QP_DIR)/qso/qsoProcStmts.cpp\
           $(QP_DIR)/qso/qsoPkgStmts.cpp

QDQ_SRCS = $(QP_DIR)/qdq/qdq.cpp

QSX_SRCS = $(QP_DIR)/qsx/qsx.cpp              \
           $(QP_DIR)/qsx/qsxProc.cpp          \
           $(QP_DIR)/qsx/qsxPkg.cpp           \
           $(QP_DIR)/qsx/qsxRelatedProc.cpp   \
           $(QP_DIR)/qsx/qsxTemplatePool.cpp  \
           $(QP_DIR)/qsx/qsxEnv.cpp           \
           $(QP_DIR)/qsx/qsxExecutor.cpp      \
           $(QP_DIR)/qsx/qsxUtil.cpp          \
           $(QP_DIR)/qsx/qsxCursor.cpp        \
           $(QP_DIR)/qsx/qsxArray.cpp         \
           $(QP_DIR)/qsx/qsxAvl.cpp           \
           $(QP_DIR)/qsx/qsxRefCursor.cpp     \
           $(QP_DIR)/qsx/qsxExtProc.cpp       \
           $(QP_DIR)/qsx/qsxTemplateCache.cpp \
           $(QP_DIR)/qsx/qsxLibrary.cpp

QSS_SRCS = $(QP_DIR)/qss/qss.cpp

QSC_SRCS = $(QP_DIR)/qsc/qsc.cpp

QTC_SRCS = $(QP_DIR)/qtc/qtc.cpp                        \
           $(QP_DIR)/qtc/qtcAssign.cpp                  \
           $(QP_DIR)/qtc/qtcColumn.cpp                  \
           $(QP_DIR)/qtc/qtcIndirect.cpp                \
           $(QP_DIR)/qtc/qtcSkip.cpp                    \
           $(QP_DIR)/qtc/qtcSubquery.cpp                \
           $(QP_DIR)/qtc/qtcValue.cpp                   \
           $(QP_DIR)/qtc/qtcPass.cpp                    \
           $(QP_DIR)/qtc/qtcRid.cpp                     \
           $(QP_DIR)/qtc/qtcConstantWrapper.cpp         \
           $(QP_DIR)/qtc/qtcSubqueryWrapper.cpp         \
           $(QP_DIR)/qtc/qtcSpFunctionCall.cpp          \
           $(QP_DIR)/qtc/qtcSpCursor.cpp                \
           $(QP_DIR)/qtc/qtcSpCursorFound.cpp           \
           $(QP_DIR)/qtc/qtcSpCursorNotFound.cpp        \
           $(QP_DIR)/qtc/qtcSpCursorIsOpen.cpp          \
           $(QP_DIR)/qtc/qtcSpCursorIsNotOpen.cpp       \
           $(QP_DIR)/qtc/qtcSpCursorRowCount.cpp        \
           $(QP_DIR)/qtc/qtcSpSqlCode.cpp               \
           $(QP_DIR)/qtc/qtcSpSqlErrm.cpp               \
           $(QP_DIR)/qtc/qtcSpRowType.cpp               \
           $(QP_DIR)/qtc/qtcSpArrType.cpp               \
           $(QP_DIR)/qtc/qtcSpRefCurType.cpp            \
           $(QP_DIR)/qtc/qtcHash.cpp                    \
           $(QP_DIR)/qtc/qtcCache.cpp

QCS_SRCS = $(QP_DIR)/qcs/qcs.cpp             \
           $(QP_DIR)/qcs/qcsModule.cpp       \
           $(QP_DIR)/qcs/qcsAltibase.cpp     \
           $(QP_DIR)/qcs/qcsDAmo.cpp         \
           $(QP_DIR)/qcs/qcsCubeOne.cpp

QDSD_SRCS = $(QP_DIR)/qdsd/qdsd.cpp

QP_SRCS = $(QCI_SRCS) $(QCG_SRCS) $(QCP_SRCS) $(QCC_SRCS) $(QCM_SRCS) $(QCU_SRCS) \
          $(QCD_SRCS) $(QMS_SRCS) $(QMR_SRCS) \
          $(QDB_SRCS) $(QDC_SRCS) $(QDD_SRCS) $(QDN_SRCS) $(QDR_SRCS) \
          $(QDS_SRCS) $(QDX_SRCS) $(QDV_SRCS) $(QDM_SRCS) $(QMB_SRCS) $(QMC_SRCS) \
          $(QMV_SRCS) $(QMO_SRCS) $(QMG_SRCS) $(QMN_SRCS) $(QMX_SRCS) \
          $(QSF_SRCS) $(QSV_SRCS) $(QSO_SRCS) $(QSX_SRCS) $(QSS_SRCS) $(QSC_SRCS) \
          $(QDP_SRCS) $(QDT_SRCS) $(QDK_SRCS) \
          $(QTC_SRCS) $(QRC_SRCS) $(QDQ_SRCS) $(QCS_SRCS) $(QDSD_SRCS)

QP_OBJS = $(QP_SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))
QP_SHOBJS = $(QP_SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%_shobj.$(OBJEXT))

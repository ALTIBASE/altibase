rm $ALTIBASE_HOME/conf/recovery.dat
( cd $ALTIDEV_HOME/src/sm; make gen_rec_list )
( cd $ALTIDEV_HOME/src/rp; make gen_rec_list )
( cd $ALTIDEV_HOME/src/qp; make gen_rec_list )
( cd $ALTIDEV_HOME/src/id; make gen_rec_list )
( cd $ALTIDEV_HOME/src/st; make gen_rec_list )
( cd $ALTIDEV_HOME/src/mm; make gen_rec_list )
( cd $ALTIDEV_HOME/src/cm; make gen_rec_list )
( cd $ALTIDEV_HOME/src/mt; make gen_rec_list )

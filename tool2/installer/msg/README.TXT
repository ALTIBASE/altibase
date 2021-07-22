 [ Quick Guide to Making Settings in ALTIBASE XDB ] 

 1. Set kernel variables using the root user account.
    Please refer to the installation manual 
    or $ALTIBASE_XDB_HOME/install/kernelSetGuide.txt. 
 
 2. Provide a license.
    Please rename and locate the license file as shown below.
    $ALTIBASE_XDB_HOME/conf/license

    If no license file has been issued, or if the license file has expired,
    ALTIBASE XDB services will not start.
    In this case, please visit http://support.altibase.com 

 3. Configure user environment variables (using the user account with which ALTIBASE XDB was installed).
    Run the '$ALTIBASE_XDB_HOME/install/post_install.sh' file 
    under the account with which ALTIBASE XDB was installed.

    This script performs necessary post-installation configuration. 

    1) Create the ALTIBASE XDB user environment file and apply it to the user profile.
       ($ALTIBASE_XDB_HOME/conf/xdbaltibase_user.env)

        shell> sh post_install.sh setenv
         
    2) Create a database.

         If you selected 'YES' in response to the question about whether to create 
         a database after installation by Package installer, at "ALTIBASE XDB Quick Setting Guide", 
         a database will be automatically created.

         If you selected 'NO' in response to this question,
         you need to create a database manually.

         shell> xdbserver create [DB Character Set] [National Character Set]

 4. Start up and shut down the server
     shell> xdbserver start
     shell> xdbserver stop

 5. Connect to the database using xdbiSQL
     shell> xdbisql -s 127.0.0.1 -u SYS -p MANAGER

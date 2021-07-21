### Altibase: Enterprise grade High performance Relational DBMS
- Enterprise grade: 17 years of experience in servicing over 600 global enterprise clients including Samsung, HP, E-Trade, China Mobile and more. (Ref: http://altibase.com/product/enterprise-grade-database/ )
- High performance: Accelerate the performance of your mission critical applications by over 10X with in-memory and on-disk hybrid architecture. (Ref: http://altibase.com/product/high-performance-database/ )
- Relational DBMS: Function and feature rich with all the tools and relational capabilities expected and required by enterprise grade applications. Enjoy the benefits without radically changing your applications. (Ref: http://altibase.com/product/relational-database/ )
- Low cost: Open source costs with enterprise grade quality. Lower your TCO with flexible subscription fees. (Ref: http://altibase.com/product/low-cost-database/ )

### Help
- http://altibase.com/resources/
- http://altibase.com/services/support/
- http://altibase.com/case-studies/
- http://altibase.com/faqs/product/
- http://support.altibase.com (Korean materials)

### Q&A and Bug Reports
- Q&A and bug reports regarding Altibase should be submitted at http://support.altibase.com
- The code for Altibase can be found at: https://github.com/Altibase

### License
- Altibase is an open source with GNU Affero General Public License version 3(GNU AGPLv3) for the Altibase server code and GNU Lesser General Public License version 3(GNU LGPLv3) for the Altibase client code. 
- The products that can be downloaded from this site are open source versions. Please contact Altibase for Altibase enterprise edition products.
- License information can be found in the COPYING files.
- Altibase includes GPC sources, so, if you want to use those sources for commertial use then you need to buy "General Polygon Clipper (GPC) License".

### Build and Run
<pre><code>
- OS: Red Hat or Ubuntu
- CPU: x86_64
- selinux disabling (optional)
  vi /etc/sysconfig/selinux 
  SELINUX=disabled
- ntsysv  (optional)
  IPTables = iptables, ip6tables *uncheck*
  vsftpd = *check* 
  /etc/init.d/iptable stop
  /etc/init.d/vsftpd start
- /etc/rc.local  (optional)
  echo 16147483648 > /proc/sys/kernel/shmmax
  echo 1024 32000 1024 1024 > /proc/sys/kernel/sem
- /etc/sysctl.conf  (optional)
  # Controls the default maxmimum size of a mesage queue
  kernel.msgmnb = 65536
  # Controls the maximum size of a message, in bytes
  kernel.msgmax = 65536
  # Controls the maximum shared segment size, in bytes
  kernel.shmmax = 68719476736000
  # Controls the maximum number of shared memory segments, in pages
  kernel.shmall = 4294967296
  fs.suid_dumpable = 1
  fs.aio-max-nr = 1048576
  fs.file-max = 6815744
  # semaphores: semmsl, semmns, semopm, semmni  
  kernel.sem = 1024 32000 1024 1024
  net.ipv4.ip_local_port_range = 32768 61000
  net.core.rmem_default = 4194304
  net.core.rmem_max = 4194304
  net.core.wmem_default = 262144
  net.core.wmem_max = 1048586
  # core filename pattern (core.execution_file_name.time)
  kernel.core_uses_pid = 0
  kernel.core_pattern = core.%e.%t
- Install following libraries  (optional)
  https://gmplib.org/ 
  http://www.mpfr.org/
  http://www.multiprecision.org/
  http://www.mr511.de/software/english.html
  Gmp => ./configure --enable-shared --enable-static --prefix=/usr/gmp
  Mpfr => ./configure --enable-shared --enable-static --prefix=/usr/mpfr --with-gmp=/usr/gmp
  Mpc => ./configure --enable-shared --enable-static --prefix=/usr/mpc --with-gmp=/usr/gmp --with-mpfr=/usr/mpfr
  Libelf => ./configure --enable-shared --enable-static --prefix=/usr/elf --with-gmp=/usr/gmp --with-mpfr=/usr/mpfr
  LD_LIBRARY_PATH=/usr/gmp/lib:/usr/mpfr/lib:/usr/mpc/lib:/usr/elf/lib:$LD_LIBRARY_PATH
  Install glibc-devel package(e.g. glibc-devel-2.*.el6.i686.rpm)
  https://gcc.gnu.org/
  ./configure --prefix=/usr/local/gcc-4.6.3 \
  --enable-shared \
  --enable-threads=posix \
  --enable-languages=c,c++ \
  --with-gmp=/usr/gmp \
  --with-mpfr=/usr/mpfr \
  --with-mpc=/usr/mpc \
  --with-libelf=/usr/elf \
  make; make install
- glibc 2.12 ~ 2.27 (ldd --version) (If system doesn't have right one, change OS version itself)
- gcc 4.6.3 ~ 7.3.0 (gcc --version)
  If you want to use gcc 7.x then refer this repostory Wiki page "changes for using gcc 7"
- Install both of Oracle Java JDK 1.5 and 1.7
  Go to https://www.oracle.com/technetwork/java/archive-139210.html
  JDK 1.5 : jdk-1_5_0_22-linux-amd64-rpm.bin or jdk-1_5_0_22-linux-amd64.bin
  JDK 1.7 : jdk-7u80-linux-x64.rpm or jdk-7u80-linux-x64.tar.gz
- Install OpenSSL
  Remove existing openssl (Ubuntu 18.04 example)
    sudo mv /usr/include/openssl /usr/include/openssl.original
    sudo mv /usr/include/x86_64-linux-gnu/openssl /usr/include/x86_64-linux-gnu/openssl.original
  Download : https://www.openssl.org/source/openssl-1.0.2o.tar.gz
  Make a symbolic link : sudo ln -s /usr/local/ssl/include/openssl /usr/include/openssl
- Install development tools
  sudo apt install autoconf autopoint help2man texinfo g++ gawk flex bison
  flex (2.5.35 version) (https://github.com/westes/flex/releases) (install method: https://github.com/westes/flex)
  bison (2.4.1 version) (http://ftp.gnu.org/gnu/bison/)
  sudo apt install libncurses5-dev binutils-dev ddd tkdiff manpages-dev libldap2-dev
- Modify /usr/include/sys/select.h (Ubuntu: /usr/include/x86_64-linux-gnu/sys/select.h)
  Following diff command's resulting line numbers can be different by various platforms and versions. So, search using keywords and approximate positions. 
  Following diff is executed on the Ubuntu 18.04.
  /usr/include/x86_64-linux-gnu/sys$ diff select.h.original select.h
  57a58,62
  > /* Maximum number of file descriptors in `fd_set'. */
  > #ifndef FD_SETSIZE
  > #define FD_SETSIZE __FD_SETSIZE
  > #endif
  > 
  64c69
  <     __fd_mask fds_bits[__FD_SETSIZE / __NFDBITS];
  ---
  >     __fd_mask fds_bits[FD_SETSIZE / __NFDBITS];
  67c72
  <     __fd_mask __fds_bits[__FD_SETSIZE / __NFDBITS];
  ---
  >     __fd_mask __fds_bits[FD_SETSIZE / __NFDBITS];

- re2c (re2c-0.13.5.tar.gz) (http://re2c.org/install/install.html)
- Nullify SVN setting(refer this repostory Wiki page "Fix build without Altibase internal SVN")
- Other environment variable setting
  export LANG=en_US.UTF-8
  export ALTIDEV_HOME=/path/to/source_code_directory
  export ALTIBASE_DEV=${ALTIDEV_HOME}
  export ALTIBASE_HOME=${ALTIDEV_HOME}/altibase_home
  export ALTIBASE_NLS_USE=UTF8
  export ALTIBASE_PORT_NO=17730
  export ADAPTER_JAVA_HOME=/path/to/jdk1.7
  export JAVA_HOME=/path/to/jdk1.5
  export PATH=.:${ALTIBASE_HOME}/bin:${JAVA_HOME}/bin:${PATH}
  export CLASSPATH=.:${JAVA_HOME}/lib:${JAVA_HOME}/jre/lib:${ALTIBASE_HOME}/lib/Altibase.jar:${CLASSPATH}
  export LD_LIBRARY_PATH=$ADAPTER_JAVA_HOME/jre/lib/amd64/server:${ALTIBASE_HOME}/lib:${LD_LIBRARY_PATH}

- Build Altibase
  ./configure --with-build_mode=release ## default build mode is debug mode
  make clean
  make build

- Prepare Altibase configuration file
  cp $ALTIBASE_HOME/conf/altibase.properties.release $ALTIBASE_HOME/conf/altibase.properties 
- Create Altibase database files
  $ALTIBASE_HOME/bin/server create UTF8 UTF8
- Start Altibase server deamon
  $ALTIBASE_HOME/bin/server start
- Connect Altibase server using iSQL and retrieve all table list
  $ALTIBASE_HOME/bin/is
  iSQL> select * from tab;
</code></pre>

tar xvf flex-2.5.38.tar.gz 
cd flex-2.5.38
./autogen.sh
./configure --build=unknown-unknown-linux; make; sudo make install
flex --version


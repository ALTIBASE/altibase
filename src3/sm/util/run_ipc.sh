if [ ! $1 ]; then
echo "input process count";
exit;
fi

CMD="./FinalIPC 3 1 1000 2 &"

echo "RUN=>$CMD"; echo ""

i=1

while [ $i -le $1 ]
do
  eval "./FinalIPC $i 1 2 10000 &"
  i=`expr $i + 1 `
done


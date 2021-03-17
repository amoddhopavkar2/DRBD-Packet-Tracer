dir=$(pwd)
echo dir
cd $dir/tracer
make clean
make
gcc tracer.c -o tracer
cd $dir
cp $dir/tracer/Modules.symvers ./drbd-8.4/drbd
cd $dir/drbd-8.4
make clean
make
insmod $dir/tracer/tracer.ko
insmod $dir/drbd-8.4/drbd/drbd.ko

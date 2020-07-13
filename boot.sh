get_script_dir () {
    SOURCE="${BASH_SOURCE[0]}"
    while [ -h "$SOURCE" ]; do
       	DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
       	SOURCE="$( readlink "$SOURCE" )" 
       	[[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
  	done
   	DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
   	echo "$DIR"
}
echo $(get_script_dir)
export RTE_SDK=$(get_script_dir)/lib/dpdk-stable-19.11.3
export RTE_TARGET=x86_64-native-linuxapp-gcc
echo $RTE_SDK
echo $RTE_TARGET
cd lib/dpdk-stable-19.11.3
make install T=x86_64-native-linuxapp-gcc -j 10
cd ../..
if [ "x$PLATFORM" == "xQT" ]
then
    export BUILD_DIR=goom.001
	export FINAL_DIR="$CONFIGURATION_BUILD_DIR/data"
elif [ "x$PLATFORM" == "xiphone" ]
then
	BUILD_DIR=goom.001
	FINAL_DIR="$CONFIGURATION_BUILD_DIR/Goom.app/data"
fi

export PYTHONPATH="../../scripts/$PLATFORM"
. "../../scripts/$PLATFORM/functions.sh"

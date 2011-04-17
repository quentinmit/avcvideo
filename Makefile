all: avctoudp

run: avctoudp
	env DYLD_FRAMEWORK_PATH=. ./avctoudp

avctoudp: avctoudp.cc
	g++ -m32 -o avctoudp avctoudp.cc -F/Developer/FireWireSDK26/Examples/Framework/ -framework AVCVideoServices -framework CoreFoundation
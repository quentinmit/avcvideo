#include <AVCVideoServices/AVCVideoServices.h>
using namespace AVS;

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/uio.h>
#include <unistd.h>

void MPEGReceiverMessageReceivedProc(UInt32 msg, UInt32 param1, UInt32 param2, void *pRefCon);
IOReturn MyStructuredDataPushProc(UInt32 CycleDataCount, MPEGReceiveCycleData *pCycleData, void *pRefCon);

#define kMicroSecondsPerSecond 1000000
#define kNumCyclesInMPEGReceiverSegment 20
#define kNumSegmentsInMPEGReceiverProgram 100

unsigned long packetCount = 0;

int main(int argc, char** argv) {
  AVCDeviceController *pAVCDeviceController = nil;
  AVCDevice *pAVCDevice;
  AVCDeviceStream *pAVCDeviceStream;
  int deviceIndex = 0;
  struct sockaddr_in socketAddr;
  int viewerSocket;

  printf("AVCtoUDP starting\n");

  // Create a AVCDeviceController
  CreateAVCDeviceController(&pAVCDeviceController);
  if (!pAVCDeviceController)
    {
      // TODO: This should never happen (unless we've run out of memory), but we should handle it cleanly anyway
      printf("Failed to create AVC device controller.\n");
      return(1);
    }

  printf("Created AVC device controller.\n");

  if (deviceIndex >= CFArrayGetCount(pAVCDeviceController->avcDeviceArray)) {
    printf("Failed to find AVC device %d\n", deviceIndex);
    return(1);
  }

  pAVCDevice = (AVCDevice*)
    CFArrayGetValueAtIndex(pAVCDeviceController->avcDeviceArray,deviceIndex);

  if (!pAVCDevice) {
    printf("Failed to find AVC device %d\n", deviceIndex);
    return(1);
  }

  printf("Found device with GUID 0x%016llX\n", pAVCDevice->guid);

  pAVCDevice->openDevice(nil,nil);

  viewerSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  inet_aton("127.0.0.1", &socketAddr.sin_addr);
  socketAddr.sin_family = AF_INET;
  socketAddr.sin_port = htons(41394);

  connect(viewerSocket, (struct sockaddr *) &socketAddr, sizeof socketAddr);

  pAVCDeviceStream = pAVCDevice->CreateMPEGReceiverForDevicePlug
    (0,
     nil, // We'll install the structured callback later (MyStructuredDataPushProc),
     nil,
     MPEGReceiverMessageReceivedProc,
     nil,
     nil,
     kNumCyclesInMPEGReceiverSegment,
     kNumSegmentsInMPEGReceiverProgram);

  pAVCDeviceStream->pMPEGReceiver->registerStructuredDataPushCallback
    (
     MyStructuredDataPushProc,
     kNumCyclesInMPEGReceiverSegment,
     (void*) viewerSocket
     );

  pAVCDevice->StartAVCDeviceStream(pAVCDeviceStream);

  while (1) {
    usleep(kMicroSecondsPerSecond);
    printf("MPEG packets received: %d\n",packetCount);
  }
}

void MPEGReceiverMessageReceivedProc(UInt32 msg, UInt32 param1, UInt32 param2, void *pRefCon)
{

}

IOReturn MyStructuredDataPushProc(UInt32 CycleDataCount, MPEGReceiveCycleData *pCycleData, void *pRefCon)
{
  int vectIndex = 0;
  struct iovec iov[kNumCyclesInMPEGReceiverSegment*5];
  int viewerSocket = (int) pRefCon;
  
  if (viewerSocket)
    {
      for (int cycle=0;cycle<CycleDataCount;cycle++)
	{
	  for (int sourcePacket=0;sourcePacket<pCycleData[cycle].tsPacketCount;sourcePacket++)
	    {
	      iov[vectIndex].iov_base = pCycleData[cycle].pBuf[sourcePacket];
	      iov[vectIndex].iov_len = kMPEG2TSPacketSize;
	      vectIndex += 1;
	    }
	}
      
      writev(viewerSocket, iov, vectIndex);
    }
  
  return 0;
}

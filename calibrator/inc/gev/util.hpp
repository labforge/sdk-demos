#ifndef __gev_util_hpp__
#define __gev_util_hpp__

#include <cstdint>
#include <list>
#include <variant>
#include <string>
#include <QString>
#include <PvStream.h>
#include <PvBuffer.h>
#include <PvDeviceGEV.h>
#include <PvSystem.h>
#include <PvStreamGEV.h>

namespace labforge::gev {

PvStream *OpenStream( const PvString &macAddress, PvResult *res );
bool ConfigureStream(PvDevice *aDevice, PvStream *aStream );
void CreateStreamBuffers(PvDevice *aDevice, PvStream *aStream, std::list<PvBuffer *> *aBufferList,
                         const size_t buffer_count);
void FreeStreamBuffers( std::list<PvBuffer *> *aBufferList );
bool TweakParameters(PvDeviceGEV *aDevice, PvStream *aStream );
bool SetParameter(PvDeviceGEV *aDevice, PvStream *aStream, const char* name, std::variant<int64_t, double, bool, std::string> value);

bool IsEBusLoaded();

}

#endif // __gev_util_hpp__
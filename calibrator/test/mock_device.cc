#include "mock_device.hpp"
#include <opencv2/opencv.hpp>
#include <QThread>
#include <bitset>
//#include "labforge/gev/chunkdata.h"


using namespace labforge::test;
using namespace cv;
using namespace std;

void MockEventSink::CheckRegisterIntegrity() {
  size_t last_address = 0;
  size_t last_size = 0;
  for(std::pair<const RegisterDefinition*,std::variant<int, float, std::string>> i : m_register_definition) {
    if(last_address > 0) {
      if((i.first->m_address - last_address) < last_size) {
        throw runtime_error("Register definitions intersect, space addresses at least regtype bytes apart.");
      }
    }
    last_address = i.first->m_address;
    last_size = 4;
    if(i.first->m_type == PvGenTypeString) {
      last_size = i.first->m_length;
    }
  }
}

const RegisterDefinition * MockEventSink::GetRegisterDefinition(const string &name) {
  // Find address for string
  for(auto i : m_register_definition) {
    if(string(i.first->m_name) == name)
      return i.first;
  }
  return nullptr;
}

void MockEventSink::OnCreateCustomRegisters(IPvSoftDeviceGEV *aDevice, IPvRegisterFactory *aFactory) {
  (void)aDevice;
  PvResult res;

  for(auto def : m_register_definition) {
    const RegisterDefinition *reg = def.first;
    variant<int, float, std::string> value = def.second;

    size_t length = 4; // anything numeric including boolean
    switch(reg->m_type) {
      case PvGenTypeString:
        length = reg->m_length;
        break;
      default:
        break;
    }

    res = aFactory->AddRegister(reg->m_name,
                                reg->m_address,
                                length,
                                reg->m_mode,
                                m_register_event_sink);
    if(!res.IsOK()) {
      throw runtime_error("Could not register parameter");
    }
  }
//
//  aFactory->AddRegister( SAMPLEINTEGERNAME, SAMPLEINTEGERADDR, 4, PvGenAccessModeReadWrite, mRegisterEventSink );
//  aFactory->AddRegister( SAMPLEFLOATNAME, SAMPLEFLOATADDR, 4, PvGenAccessModeReadWrite, mRegisterEventSink );
//  aFactory->AddRegister( SAMPLESTRINGNAME, SAMPLESTRINGADDR, 16, PvGenAccessModeReadWrite, mRegisterEventSink );
//  aFactory->AddRegister( SAMPLEBOOLEANNAME, SAMPLEBOOLEANADDR, 4, PvGenAccessModeReadWrite, mRegisterEventSink );
//  aFactory->AddRegister( SAMPLECOMMANDNAME, SAMPLECOMMANDADDR, 4, PvGenAccessModeReadWrite, mRegisterEventSink );
//  aFactory->AddRegister( SAMPLEENUMNAME, SAMPLEENUMADDR, 4, PvGenAccessModeReadWrite, mRegisterEventSink );
}

void MockEventSink::OnCreateCustomGenApiFeatures(IPvSoftDeviceGEV *aDevice, IPvGenApiFactory *aFactory ) {
  IPvRegisterMap *lMap = aDevice->GetRegisterMap();

  CreateGenicamParameters( lMap, aFactory );
  CreateChunkParameters( aFactory );
  CreateEventParameters( aFactory );
}

void MockEventSink::CreateGenicamParameters(IPvRegisterMap *aMap, IPvGenApiFactory *aFactory ) {
  PvResult res;
  for(auto def : m_register_definition) {
    const RegisterDefinition * reg = def.first;
    variant<int, float, std::string> value = def.second;

    aFactory->SetName(reg->m_name);
    if(reg->m_description)
      aFactory->SetDescription(reg->m_description);
    if(reg->m_tooltip)
      aFactory->SetToolTip(reg->m_tooltip);
    if(reg->m_category)
      aFactory->SetCategory(reg->m_category);
    if(reg->m_unit)
      aFactory->SetUnit( reg->m_unit );

    // ONLY FOR testing disable cache, so updates are always propagrated to user
    aFactory->SetCachable(PvGenCacheNone);

    IPvRegister *regval = aMap->GetRegisterByAddress(reg->m_address);;
    if(regval == nullptr) {
      throw runtime_error("Could not register parameter");
    }

    switch(reg->m_type) {
      case PvGenTypeString:
        res = aFactory->CreateString(regval);
        break;
      case PvGenTypeInteger:
        res = aFactory->CreateInteger(regval, reg->m_min, reg->m_max);
        if(!res.IsOK())
          break;
        res = regval->Write(get<int>(value));
        break;
      case PvGenTypeFloat:
        aFactory->SetRepresentation( PvGenRepresentationPureNumber );
        res = aFactory->CreateFloat(regval, reg->m_min, reg->m_max);
        if(!res.IsOK())
          break;
        res = regval->WriteFloat(get<float>(value));
        break;
//      case PvGenTypeBoolean:
//        aFactory->CreateBoolean(regval);
//        break;
      default:
        throw runtime_error("Type not implemented");
    }
    if(!res.IsOK()) {
      throw runtime_error("Could not create parameter");
    }
  }

  // FIXME: iterate over register_definition and set default values
//  // Creates sample enumeration feature mapped to SAMPLEENUMADDR register.
//  // Enumeration entries of EnumEntry1 (0), EnumEntry2 (1) and EnumEntry3 (2) are created.
//  // This feature is also defined as a selector of the sample string and sample Boolean.
//  aFactory->SetName( SAMPLEENUMNAME );
//  aFactory->SetDescription( SAMPLEENUMDESCRIPTION );
//  aFactory->SetToolTip( SAMPLEENUMTOOLTIP );
//  aFactory->SetCategory( SAMPLECATEGORY );
//  aFactory->AddEnumEntry( "EnumEntry1", 0 );
//  aFactory->AddEnumEntry( "EnumEntry2", 1 );
//  aFactory->AddEnumEntry( "EnumEntry3", 2 );
//  aFactory->AddSelected( SAMPLESTRINGNAME );
//  aFactory->AddSelected( SAMPLEBOOLEANNAME );
//  aFactory->CreateEnum( aMap->GetRegisterByAddress( SAMPLEENUMADDR ) );
//
//  // Creates sample string feature mapped to SAMPLESTRINGADDR register.
//  // This feature is selected by sample enumeration.
//  // This feature has an invalidator to the sample command: its cached value will be invalidated (reset) whenever the command is executed.
//  aFactory->SetName( SAMPLESTRINGNAME );
//  aFactory->SetDescription( SAMPLESTRINGDESCRIPTION );
//  aFactory->SetToolTip( SAMPLESTRINGTOOLTIP );
//  aFactory->SetCategory( SAMPLECATEGORY );
//  aFactory->AddInvalidator( SAMPLECOMMANDNAME );
//  aFactory->CreateString( aMap->GetRegisterByAddress( SAMPLESTRINGADDR ) );
//
//  // Creates sample Boolean feature mapped to SAMPLEBOOLEANADDR register.
//  // This feature is selected by sample enumeration.
//  aFactory->SetName( SAMPLEBOOLEANNAME );
//  aFactory->SetDescription( SAMPLEBOOLEANDESCRIPTION );
//  aFactory->SetToolTip( SAMPLEBOOLEANTOOLTIP );
//  aFactory->SetCategory( SAMPLECATEGORY );
//  aFactory->CreateBoolean( aMap->GetRegisterByAddress( SAMPLEBOOLEANADDR ) );
//
//  // Creates sample command feature mapped to SAMPLECOMMANDADDR register.
//  // The sample string has this command defined as its invalidator.
//  // Executing this command resets the controller-side GenApi cached value of the sample string.
//  aFactory->SetName( SAMPLECOMMANDNAME );
//  aFactory->SetDescription( SAMPLECOMMANDDESCRIPTION );
//  aFactory->SetToolTip( SAMPLECOMMANDTOOLTIP );
//  aFactory->SetCategory( SAMPLECATEGORY );
//  aFactory->CreateCommand( aMap->GetRegisterByAddress( SAMPLECOMMANDADDR ) );
//
//  // Creates sample integer feature mapped to SAMPLEINTEGERADDR register.
//  // Minimum is defined as -10000, maximum 100000, increment as 1 and unit as milliseconds.
//  aFactory->SetName( SAMPLEINTEGERNAME );
//  aFactory->SetDescription( SAMPLEINTEGERDESCRIPTION );
//  aFactory->SetToolTip( SAMPLEINTEGERTOOLTIP );
//  aFactory->SetCategory( SAMPLECATEGORY );
//  aFactory->SetRepresentation( PvGenRepresentationLinear );
//  aFactory->SetUnit( SAMPLEINTEGERUNITS );
//  aFactory->CreateInteger( aMap->GetRegisterByAddress( SAMPLEINTEGERADDR ), -10000, 10000, 1 );
//
//  // Creates sample float feature mapped to SAMPLEFLOATADDR register.
//  // Minimum is defined as -100.0 and maximum as 100.0 and units as inches.
//  aFactory->SetName( SAMPLEFLOATNAME );
//  aFactory->SetDescription( SAMPLEFLOATDESCRIPTION );
//  aFactory->SetToolTip( SAMPLEFLOATTOOLTIP );
//  aFactory->SetCategory( SAMPLECATEGORY );
//  aFactory->SetRepresentation( PvGenRepresentationPureNumber );
//  aFactory->SetUnit( SAMPLEFLOATUNITS );
//  aFactory->CreateFloat( aMap->GetRegisterByAddress( SAMPLEFLOATADDR ), -100.0, 100.0 );
//
//  // Creates sample pValue feature which simply links to another feature.
//  // This feature is linked to sample integer.
//  // We use "pValue" as display name as the automatically generated one is not as good.
//  aFactory->SetName( SAMPLEPVALUE );
//  aFactory->SetDisplayName( SAMPLEPVALUEDISPLAYNAME );
//  aFactory->SetDescription( SAMPLEPVALUEDESCRIPTION );
//  aFactory->SetToolTip( SAMPLEPVALUETOOLTIP );
//  aFactory->SetCategory( SAMPLECATEGORY );
//  aFactory->SetPValue( SAMPLEINTEGERNAME );
//  aFactory->SetUnit( SAMPLEPVALUEUNITS );
//  aFactory->CreateInteger();
//
//  // Creates an integer SwissKnife, a read-only expression.
//  // It only has one variable, the sample integer.
//  // The SwissKnife expression converts the millisecond sample integer to nanoseconds with a * 1000 formula".
//  aFactory->SetName( SAMPLEINTSWISSKNIFENAME );
//  aFactory->SetDescription( SAMPLEINTSWISSKNIFEDESCRIPTION );
//  aFactory->SetToolTip( SAMPLEINTSWISSKNIFETOOLTIP );
//  aFactory->SetCategory( SAMPLECATEGORY );
//  aFactory->AddVariable( SAMPLEINTEGERNAME );
//  aFactory->SetUnit( SAMPLEINTSWISSKNIFEUNITS );
//  aFactory->CreateIntSwissKnife( SAMPLEINTEGERNAME " * 1000" );
//
//  // Creates a float SwissKnife, a read-only expression.
//  // It only has one variable, the sample float.
//  // The SwissKnife expression converts the inches sample float to centimeters with a * 2.54 formula.
//  aFactory->SetName( SAMPLEFLOATSWISSKNIFENAME );
//  aFactory->SetDescription( SAMPLEFLOATSWISSKNIFEDESCRIPTION );
//  aFactory->SetToolTip( SAMPLEFLOATSWISSKNIFETOOLTIP );
//  aFactory->SetCategory( SAMPLECATEGORY );
//  aFactory->AddVariable( SAMPLEFLOATNAME );
//  aFactory->SetUnit( SAMPLEFLOATSWISSKNIFEUNITS );
//  aFactory->CreateFloatSwissKnife( SAMPLEFLOATNAME " * 2.54" );
//
//  // Create integer converter (read-write to/from expressions)
//  // The main feature the converter acts uppon is the sample integer and handles millisecond to nanosecond conversion.
//  // No additional variables are required, no need to call AddVariable on the factory.
//  aFactory->SetName( SAMPLEINTCONVERTERNAME );
//  aFactory->SetDescription( SAMPLEINTCONVERTERDESCRIPTION );
//  aFactory->SetToolTip( SAMPLEINTCONVERTERTOOLTIP );
//  aFactory->SetCategory( SAMPLECATEGORY );
//  aFactory->SetUnit( SAMPLEINTCONVERTERUNITS );
//  aFactory->CreateIntConverter( SAMPLEINTEGERNAME, "TO * 1000", "FROM / 1000" );
//
//  // Create float converter (read-write to/from expressions)
//  // The main feature the converter acts uppon is the sample float and handles inches to centimeter conversion.
//  // No additional variables are required, no need to call AddVariable on the factory.
//  aFactory->SetName( SAMPLEFLOATCONVERTERNAME );
//  aFactory->SetDescription( SAMPLEFLOATCONVERTERDESCRIPTION );
//  aFactory->SetToolTip( SAMPLEFLOATCONVERTERTOOLTIP );
//  aFactory->SetCategory( SAMPLECATEGORY );
//  aFactory->SetUnit( SAMPLEFLOATCONVETERUNITS );
//  aFactory->CreateFloatConverter( SAMPLEFLOATNAME, "TO * 2.54", "FROM * 0.3937" );
//
//  // Create hidden SwissKnife that returns non-zero (true, or 1) if the sample integer is above 5.
//  // The feature will not show up in GUI browsers as it does not have a category.
//  // We typically refer to these features as support or private features.
//  // These features typically do not have description or ToolTips as they do not show up in a GenApi user interface.
//  aFactory->SetName( SAMPLEHIDDENSWISSKNIFENAME );
//  aFactory->AddVariable( SAMPLEINTEGERNAME );
//  aFactory->CreateIntSwissKnife( SAMPLEINTEGERNAME " > 5" );
//
//  // Create integer: link to sample integer but only available when the hidden SwissKnife is non-zero.
//  // This feature is an integer that has a pValue link to our sample enumeration, displaying its integer value.
//  // We use this feature to demonstrate the pIsAvailable attribute: it links to our hidden SwissKnife and controls when it is available.
//  // When the hidden SwissKnife is true (sample integer > 5) this feature will be readable and writable.
//  // When the hidden SwissKnife is false (sample integer <= 5) this feature will be "not available".
//  // We use "pIsAvailable" as display name as the automatically generated one is not as good.
//  aFactory->SetName( SAMPLEPISAVAILABLENAME );
//  aFactory->SetDisplayName( SAMPLEPISAVAILABLEDISPLAYNAME );
//  aFactory->SetDescription( SAMPLEPISAVAILABLEDESCRIPTION );
//  aFactory->SetToolTip( SAMPLEPISAVAILABLETOOLTIP );
//  aFactory->SetCategory( SAMPLECATEGORY );
//  aFactory->SetPIsAvailable( SAMPLEHIDDENSWISSKNIFENAME );
//  aFactory->SetPValue( SAMPLEENUMNAME );
//  aFactory->CreateInteger();
}

void MockEventSink::CreateChunkParameters(IPvGenApiFactory *aFactory ) {
  (void)aFactory;
//  // Create GenApi feature used to map the chunk data count field
//  aFactory->SetName( CHUNKCOUNTNAME );
//  aFactory->SetDescription( CHUNKCOUNTDESCRIPTION );
//  aFactory->SetToolTip( CHUNKCOUNTTOOLTIP );
//  aFactory->SetCategory( CHUNKCATEGORY );
//  aFactory->MapChunk( CHUNKID, 0, 4, PvGenEndiannessLittle );
//  aFactory->CreateInteger( NULL, 0, numeric_limits<uint32_t>::max() );
//
//  // Create GenApi feature used to map the chunk data time field
//  aFactory->SetName( CHUNKTIMENAME );
//  aFactory->SetDescription( CHUNKTIMEDESCRIPTION );
//  aFactory->SetToolTip( CHUNKTIMETOOLTIP );
//  aFactory->SetCategory( CHUNKCATEGORY );
//  aFactory->MapChunk( CHUNKID, 4, 32, PvGenEndiannessLittle );
//  aFactory->CreateString( NULL );
}

void MockEventSink::CreateEventParameters(IPvGenApiFactory *aFactory ) {
  (void)aFactory;
//  // Create GenApi feature used to map the event data count field
//  aFactory->SetName( EVENTCOUNTNAME );
//  aFactory->SetDescription( EVENTCOUNTDESCRIPTION );
//  aFactory->SetToolTip( EVENTCOUNTTOOLTIP );
//  aFactory->SetCategory( EVENTCATEGORY );
//  aFactory->MapEvent( EVENTDATAID, 0, 4 );
//  aFactory->CreateInteger( NULL, 0, numeric_limits<uint32_t>::max() );
//
//  // Create GenApi feature used to map the event data time field
//  aFactory->SetName( EVENTTIMENAME );
//  aFactory->SetDescription( EVENTTIMEDESCRIPTION );
//  aFactory->SetToolTip( EVENTTIMETOOLTIP );
//  aFactory->SetCategory( EVENTCATEGORY );
//  aFactory->MapEvent( EVENTDATAID, 4, 32 );
//  aFactory->CreateString( NULL );
}

MockSource::MockSource(size_t width, size_t height, size_t buffers)
        : PvStreamingChannelSourceDefault(width, height, PvPixelYUV422_8, buffers),
          m_active(nullptr), m_stop(true), m_multipart_allowed(false), m_enabled_chunks(0),
          m_num_buffers(buffers) {
//  cout << "Created Source" << endl;
}

PvResult MockSource::MakeMultiPart(PvBuffer *buf) {
  IPvMultiPartContainer *container = buf->GetMultiPartContainer();
  PvResult res(PvResult::Code::OK);
  res = container->AddImagePart(PvMultiPart2DImage,
                                PvStreamingChannelSourceDefault::GetWidth(),
                                PvStreamingChannelSourceDefault::GetHeight(),
                                PvStreamingChannelSourceDefault::GetPixelType());
  if(!res.IsOK())
    return res;
  res = container->AddImagePart(PvMultiPart2DImage,
                                PvStreamingChannelSourceDefault::GetWidth(),
                                PvStreamingChannelSourceDefault::GetHeight(),
                                PvStreamingChannelSourceDefault::GetPixelType());
  if(!res.IsOK())
    return res;
  if(m_enabled_chunks & 0xFF000000) {
    res = container->AddChunkPart(chunklayout_size(m_enabled_chunks), m_enabled_chunks);
    if(!res.IsOK())
      return res;
  }
  res = container->AllocAllParts();
  return res;
}

bool MockSource::FillChunks(IPvChunkData *chunk) {
  if(chunk == nullptr)
    return false;

  chunk->ResetChunks();
  PvResult res;
  // Insert chunks in order of bits
  if(m_enabled_chunks & CHUNK_DNN_ID) {
    res = chunk->AddChunk(CHUNK_DNN_ID, (const uint8_t*)&m_next_dnn, sizeof(chunk_dnn_t));
//    cout << "DNN COUNT " << m_next_dnn.count << endl;
    if(!res.IsOK())
      return false;
  }
  if(m_enabled_chunks & CHUNK_FEATURES_LEFT) {
    m_next_features[0].cam_id = 0;
    res = chunk->AddChunk(CHUNK_FEATURES_LEFT, (const uint8_t*)&m_next_features[0], sizeof(chunk_features_t));
    if(!res.IsOK())
      return false;
  }
  if(m_enabled_chunks & CHUNK_FEATURES_RIGHT) {
    m_next_features[1].cam_id = 1;
    res = chunk->AddChunk(CHUNK_FEATURES_RIGHT, (const uint8_t*)&m_next_features[1], sizeof(chunk_features_t));
    if(!res.IsOK())
      return false;
  }
  if(m_enabled_chunks & CHUNK_DESCRIPTORS_LEFT) {
    m_next_descriptors[0].cam_id = 0;
    res = chunk->AddChunk(CHUNK_DESCRIPTORS_LEFT, (const uint8_t *) &m_next_descriptors[0],
                          sizeof(chunk_descriptors_t));
    if (!res.IsOK())
      return false;
  }

  if(m_enabled_chunks & CHUNK_DESCRIPTORS_RIGHT) {
    m_next_descriptors[1].cam_id = 1;
    res = chunk->AddChunk(CHUNK_DESCRIPTORS_RIGHT, (const uint8_t*)&m_next_descriptors[1], sizeof(chunk_descriptors_t));
    if(!res.IsOK())
      return false;
  }

  chunk->SetChunkLayoutID(m_enabled_chunks);

  return true;
}

PvResult MockSource::MakeImage(PvBuffer *buf) {
  buf->Reset(PvPayloadTypeImage);
  PvImage *lImage = buf->GetImage();
  PvResult res;
  res = lImage->Alloc( PvStreamingChannelSourceDefault::GetWidth(),
                       PvStreamingChannelSourceDefault::GetHeight(),
                       GetPixelType(), 0, 0, GetChunksSize());
  return res;
}

uint32_t MockSource::GetPayloadSize() const {
  uint32_t sz = 0;

  PvPixelType pix = PvStreamingChannelSourceDefault::GetPixelType();
  if(m_multipart_allowed) {
//    cout << "SZ_MP" << endl;
    sz += PvStreamingChannelSourceDefault::GetWidth() * PvStreamingChannelSourceDefault::GetHeight() * PvGetPixelBitCount(pix) * 2;
  } else {
//  cout << "SZ_SI" << endl;
    sz += PvStreamingChannelSourceDefault::GetWidth() * PvStreamingChannelSourceDefault::GetHeight() * PvGetPixelBitCount(pix);
  }
  sz += GetChunksSize();
  return sz;
}

PvBuffer *MockSource::AllocBuffer() {
//  cout << "ABUF" << endl;
  PvBuffer *aBuffer = nullptr;


  if(m_cur_buffers < m_num_buffers) {
    aBuffer = new PvBuffer(PvPayloadTypeMultiPart);
    PvResult res = MakeMultiPart(aBuffer);
    if(!res.IsOK()) {
      return nullptr;
    }

    aBuffer->SetID(m_cur_buffers++);
  }

  return aBuffer;
}

MockSource::~MockSource() {
  // Avoid mem leaks
  for(const cv::Mat *m : m_managed_images) {
    delete m;
  }
}

void MockSource::FreeBuffer(PvBuffer *aBuffer) {
//  cout << "FBUF" << endl;
  PVDELETE(aBuffer);
}

// Request to queue a buffer for acquisition.
// Return OK if the buffer is queued or any error if no more room in acquisition queue
PvResult MockSource::QueueBuffer(PvBuffer *aBuffer) {
//  cout << "QBUF" << endl;
  // Return attached buffer, regardless if used or not
  if(m_active == nullptr && !m_images.empty()) {
    m_active = aBuffer;
    if(m_multipart_allowed) {
      if(!GiveMultiPart())
        return PvResult::Code::ABORTED;
    } else {
      if(!GiveImage())
        return PvResult::Code::ABORTED;
    }

    return PvResult::Code::OK;
  }

  return PvResult::Code::BUSY;
}

bool MockSource::GiveMultiPart() {
  assert(m_active != nullptr);
  if(m_cur_image >= m_images.size())
    m_cur_image = 0;
  pair<const cv::Mat*, const cv::Mat*> pair = m_images[m_cur_image++];

  if(m_active->GetPayloadType() != PvPayloadTypeMultiPart || m_active->GetChunkDataSize() != GetChunksSize()) {
    m_active->Reset(PvPayloadTypeMultiPart);
    MakeMultiPart(m_active);
  }
//  cout << "MPART" << endl;
//  cout << (pair.first->data != nullptr) << endl;
//  cout << (pair.second->data != nullptr) << endl;
  IPvMultiPartContainer *container = m_active->GetMultiPartContainer();
//  cout << (container->GetPart(0)->GetImage()->GetDataPointer() != nullptr) << endl;
//  cout << (container->GetPart(1)->GetImage()->GetDataPointer() != nullptr) << endl;
  memcpy(container->GetPart(0)->GetImage()->GetDataPointer(), pair.first->data, container->GetPart(0)->GetImage()->GetImageSize());
//  cout << "After First" << endl;
  memcpy(container->GetPart(1)->GetImage()->GetDataPointer(), pair.second->data, container->GetPart(1)->GetImage()->GetImageSize());
//  cout << "After Second" << endl;
  if(m_enabled_chunks & 0xFF000000)
    return FillChunks(container->GetPart(2)->GetChunkData());
  return true;
}

bool MockSource::GiveImage() {
  assert(m_active != nullptr);

  if(m_cur_image >= m_images.size())
    m_cur_image = 0;
  pair<const cv::Mat*, const cv::Mat*> pair = m_images[m_cur_image++];

  if(m_active->GetPayloadType() != PvPayloadTypeImage || m_active->GetChunkDataSize() != GetChunksSize()) {
    m_active->Reset(PvPayloadTypeImage);
    MakeImage(m_active);
  }
  memcpy(m_active->GetImage()->GetDataPointer(), pair.first->data, m_active->GetImage()->GetImageSize());
  if(GetChunkModeActive()) {
    if(!FillChunks(m_active)) {
      return false;
    }
  }

  return true;
}

// Request to give back a buffer ready for transmission.
// Either block until a buffer is available or return any error
PvResult MockSource::RetrieveBuffer(PvBuffer **aBuffer) {
//  cout << "RBUF" << endl;
//  std::scoped_lock lk(m_lock);
  if(m_active == nullptr) {
    return PvResult::Code::NO_AVAILABLE_DATA;
  }
  while ( !m_stab.IsTimeToDisplay( 3 ) )
  {
    QThread::currentThread()->usleep(1000);
  }


  *aBuffer = m_active;
  m_active = nullptr;
  return PvResult::Code::OK;
}

void MockSource::OnStreamingStart() {
//  cout << "Start" << endl;
  m_stop = false;
}

void MockSource::OnStreamingStop() {
//  cout << "Stop" << endl;
  m_stop = true;
}

uint32_t MockSource::GetChunksSize() const {
  if(m_enabled_chunks & 0xFF000000) {
    return chunklayout_size(m_enabled_chunks);
  }
  return 0;
}

PvResult MockSource::SetChunkEnable(uint32_t aChunkID, bool aEnabled) {
  if(!chunklayout_size(aChunkID)) // Basically 0-size chunk indicates it's invalid
    return PvResult::Code::INVALID_PARAMETER;

  // Strip meta information from selection
  uint32_t changed_features = aChunkID & 0x00FFFFFF;
//  cout << "Enabled chunks " << aChunkID << " " << aEnabled << endl;

  if(aEnabled) {
    m_enabled_chunks = m_enabled_chunks | changed_features;
  } else {
    m_enabled_chunks = m_enabled_chunks & (~changed_features);
  }
  return PvResult::Code::OK;
}

void MockSource::SetMultiPartAllowed(bool aAllowed) {
//  cout << "Multipart Enabled" << endl;
  m_multipart_allowed = aAllowed;
}

bool MockSource::IsPayloadTypeSupported(PvPayloadType aPayloadType) {
//  cout << "Checking Multipart" << endl;
  return ( aPayloadType == PvPayloadTypeMultiPart );
}

// When this method is called for multi-part, we have to make sure we can stream valid multi-part buffers.
// Required for GigE Vision Validation Framework certification.
// If aPayloadType is PvPayloadTypeMultiPart we have to go in multi-part validation mode.
// If aPayloadType is PvPayloadTypeNone, we have to go back to the normal multi-part operation mode.
// In our case we are already using a ready-to-use test pattern, both modes are the same, there is nothing specific to do.
PvResult MockSource::SetTestPayloadFormatMode( PvPayloadType aPayloadType ) {
  switch ( aPayloadType ) {
    case PvPayloadTypeMultiPart:
//      cout << "Multi part suppoted" << endl;
      // Here we would make sure multi-part can be streamed from this source
      return PvResult::Code::OK;

    case PvPayloadTypeNone:
      // Here we would return the source in normal operation mode
      return PvResult::Code::OK;

    default:
      break;
  }
  return PvResult::Code::NOT_SUPPORTED;
}

cv::Mat *MockSource::PrepareImage(const cv::Mat *in) {
  cv::Mat resized = Mat();
  cv::resize(*in,resized,
             cv::Size(PvStreamingChannelSourceDefault::GetWidth(),
                      PvStreamingChannelSourceDefault::GetHeight()),
             cv::INTER_LINEAR);

  cvtColor(resized, resized, COLOR_BGR2YUV);
  Mat splits[3]; // y=0, u=1, v=2
  split(resized, splits);

  // New destination for packing from uncompressed YUV to YUV422
  Mat *tmp = new Mat(PvStreamingChannelSourceDefault::GetHeight(), PvStreamingChannelSourceDefault::GetWidth(), CV_8UC2);
  uint8_t *dst = tmp->data;
  uint8_t *y_src = splits[0].data;
  uint8_t *cb_src = splits[1].data;
  uint8_t *cr_src = splits[2].data;

  size_t y_len = PvStreamingChannelSourceDefault::GetHeight()*PvStreamingChannelSourceDefault::GetWidth();

  // Pack into data pointer
  // Y channel
  for(size_t i = 0; i < y_len; i++) {
    dst[i*2] = y_src[i];
  }

  // Cb (U)
  const size_t cb_cr_len = y_len/2;
  for(size_t i = 0; i < cb_cr_len; i++) {
    dst[i*4+1] = cb_src[i*2];
    dst[i*4+3] = cr_src[i*2];
  }

  m_managed_images.push_back(tmp);
  return tmp;
}

bool MockSource::QueuePair(const Mat *left, const Mat *right) {
  m_images.push_back(make_pair(PrepareImage(left), PrepareImage(right)));
  return true;
}

bool MockSource::QueueImage(const Mat *img) {
  // Resize to fit
  img = PrepareImage(img);
  m_images.push_back(make_pair(img, img));
  return true;
}

MockDevice::MockDevice(const std::string &nic,
                       const std::vector<std::pair<const RegisterDefinition *, std::variant<int, float, std::string>>> *register_definition,
                       size_t width, size_t height) {
  m_device = make_unique<PvSoftDeviceGEV>();

  // Connect the image source
  m_source = make_unique<MockSource>(width, height);
  PvResult res;
  res = m_device->AddStream(m_source.get());
  if(!res.IsOK()) {
    throw runtime_error(
            QString("Could not connect source: %1").arg(res.GetCodeString().GetAscii()).toStdString());
  }
  m_register_event_sink = make_unique<MockRegisterEventSink>();
  m_event_sink = make_unique<MockEventSink>(m_register_event_sink.get(),
                                            *register_definition);
  m_device->RegisterEventSink(m_event_sink.get());
  res = m_device->Start(nic.c_str());
  if(!res.IsOK()) {
    if ( res.GetCode() == PvResult::Code::GENICAM_XML_ERROR ) {
      PvString lXML;
      m_device->GetGenICamXMLFile( lXML );
      throw runtime_error(
              QString("The error is possibly in the dynamically generated GenICam XML file: %1\n%2")
                .arg(res.GetDescription().GetAscii())
                .arg(lXML.GetAscii()).toStdString());
    } else {
      throw runtime_error(
              QString("Unable to start device: %1").arg(res.GetDescription().GetAscii()).toStdString());
    }
  }
}

MockDevice::~MockDevice() {
  m_device->Stop();
}

bool MockDevice::GetRegisterValue(const std::string &name, std::variant<int, float, std::string> &value) const {
  const RegisterDefinition *def = m_event_sink->GetRegisterDefinition(name);
  if(def != nullptr) {
    IPvRegister *reg = m_device->GetRegisterMap()->GetRegisterByAddress(def->m_address);
    PvResult res;
    float fval;
    uint32_t ival;
    PvString sval;
    switch(def->m_type) {
      case PvGenTypeFloat:
        res = reg->ReadFloat(fval);
        value = fval;
        break;
      case PvGenTypeInteger:
        res = reg->Read(ival);
        value = (int)ival;
        break;
      case PvGenTypeString:
        res = reg->Read(sval);
        value = sval.GetAscii();
        break;
      default:
        res = PvResult::Code::NOT_IMPLEMENTED;
    }
    return res.IsOK();
  }
  return false;
}

bool MockDevice::SetRegisterValue(const std::string &name, const std::variant<int, float, std::string> &value) {
  const RegisterDefinition *def = m_event_sink->GetRegisterDefinition(name);
  if(def != nullptr) {
    IPvRegister *reg = m_device->GetRegisterMap()->GetRegisterByAddress(def->m_address);
    PvResult res;
    switch(def->m_type) {
      case PvGenTypeFloat:
        res = reg->WriteFloat(get<float>(value));
        break;
      case PvGenTypeInteger:
        res = reg->Write(get<int>(value));
        break;
      case PvGenTypeString:
        res = reg->Write(get<string>(value).c_str());
        break;
      default:
        res = PvResult::Code::NOT_IMPLEMENTED;
    }
    return res.IsOK();
  }
  return false;
}

PvResult MockSource::GetSupportedChunk(int aIndex, uint32_t &aID, PvString &aName) const {
  switch (aIndex) {
    case 0:
      aID = CHUNK_DNN_ID;
      aName = CHUNK_DNN_TEXT;
      return PvResult::Code::OK;
    case 1:
      aID = CHUNK_FEATURES_LEFT;
      aName = CHUNK_FEATURES_LEFT_TXT;
      return PvResult::Code::OK;
    case 2:
      aID = CHUNK_FEATURES_RIGHT;
      aName = CHUNK_FEATURES_RIGHT_TXT;
      return PvResult::Code::OK;
    case 3:
      aID = CHUNK_DESCRIPTORS_LEFT;
      aName = CHUNK_DESCRIPTORS_LEFT_TXT;
      return PvResult::Code::OK;
    case 4:
      aID = CHUNK_DESCRIPTORS_RIGHT;
      aName = CHUNK_DESCRIPTORS_RIGHT_TXT;
      return PvResult::Code::OK;
    default:
      break;
  }

  return PvResult::Code::NOT_FOUND;
}

void MockSource::SetDetection(chunk_dnn_t *detections) {
  memcpy(&m_next_dnn, detections, sizeof(chunk_dnn_t));
}

void MockSource::SetFeatures(size_t cam_id, chunk_features_t*features) {
  memcpy(&m_next_features[cam_id], features, sizeof(chunk_features_t));
}
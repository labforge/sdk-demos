#include <PvSoftDeviceGEVInterfaces.h>
#include <PvStreamingChannelSourceDefault.h>
#include <iostream>
#include <limits>
#include <memory>
#include <variant>
#include <vector>
#include <set>
#include <map>
#include <QString>
#include <opencv2/core/mat.hpp>
#include <PvSoftDeviceGEV.h>
#include <PvFPSStabilizer.h>

//#include "labforge/gev/chunkdata.h"

namespace labforge::test {
  /**
   * Register definition for each register.
   */
  class RegisterDefinition {
  public:
    /**
     * Create ordinal (int or double) register
     * @param address Address of register
     * @param type type of register (int or double)
     * @param name Name of register
     * @param mode Mode flags
     * @param min Min
     * @param max Max
     * @param description Description
     * @param tooltip Tooltip
     * @param category Catgeory
     */
    RegisterDefinition(uint32_t address,
                       PvGenType type,
                       const char* name,
                       PvGenAccessMode mode,
                       double min,
                       double max,
                       const char* description,
                       const char*tooltip,
                       const char* category,
                       const char* unit) :
            m_address(address),
            m_type(type),
            m_name(name),
            m_length(0),
            m_mode(mode),
            m_min(min),
            m_max(max),
            m_description(description),
            m_tooltip(tooltip),
            m_category(category),
            m_unit(unit) {
      // Sanity check value
      if(m_type == PvGenTypeInteger) {
        if(m_min < std::numeric_limits<int32_t>::min() || m_max < std::numeric_limits<int32_t>::min()) {
          throw std::runtime_error("PvGenTypeInteger cannot be negative!");
        }
        if(m_min > std::numeric_limits<int32_t>::max() || m_min > std::numeric_limits<int32_t>::max()) {
          throw std::runtime_error("PvGenTypeInteger exceeds max value!");
        }
      }
      if(m_min > m_max) {
        throw std::runtime_error("min > max");
      }
    }

    /**
     * Create a string parameter.
     * @param address Address of register
     * @param name name of register
     * @param length Maximum length of string parameter
     * @param mode Mode flags
     * @param description Description
     * @param tooltip Tooltip
     * @param category Catgeory
     */
    RegisterDefinition(uint32_t address,
                       const char* name,
                       const size_t length,
                       PvGenAccessMode mode,
                       const char* description,
                       const char*tooltip,
                       const char* category,
                       const char* unit) :
            m_address(address),
            m_type(PvGenTypeString),
            m_name(name),
            m_length(length),
            m_mode(mode),
            m_min(0),
            m_max(0),
            m_description(description),
            m_tooltip(tooltip),
            m_category(category),
            m_unit(unit) {}

    const uint32_t m_address;
    const PvGenType m_type;
    const char* m_name;
    const size_t m_length;
    const PvGenAccessMode m_mode;
    const double m_min;
    const double m_max;
    const char* m_description;
    const char* m_tooltip;
    const char* m_category;
    const char* m_unit;
  };

  /**
   * This is a mock event sink to monitor register updates and do error injection on Register updates.
   */
  class MockRegisterEventSink : public IPvRegisterEventSink {
  public:
    MockRegisterEventSink() {
      m_next_write_error = PvResult::Code::OK;
      m_next_read_error = PvResult::Code::OK;
    }
    virtual ~MockRegisterEventSink() = default;
    // Error injection
    void InjectReadError(PvResult code) { m_next_read_error = code; }
    void InjectWriteError(PvResult code) { m_next_write_error = code; }

    // Return last read registers
    IPvRegister *GetLastRead() { return m_last_read; }
    IPvRegister *GetLastWrite() { return m_last_wrote; }

    PvResult PreRead(IPvRegister *aRegister) override {
      m_last_read = aRegister;
      PvResult res = m_next_read_error;
      m_next_read_error = PvResult::Code::OK;
      return res;
    }

    PvResult PreWrite(IPvRegister *aRegister) override {
      m_last_wrote = aRegister;
      PvResult res = m_next_write_error;
      m_next_write_error = PvResult(PvResult::Code::OK);
      return res;
    }

  private:
    PvResult m_next_write_error;
    PvResult m_next_read_error;
    IPvRegister *m_last_read{};
    IPvRegister *m_last_wrote{};
  };

  /** Mock event sink to register connect and disconnect parameters.
   */
  class MockEventSink : public IPvSoftDeviceGEVEventSink {
  public:
    MockEventSink(IPvRegisterEventSink *aRegisterEventSink,
                  const std::vector<std::pair<const RegisterDefinition*,
                  std::variant<int, float, std::string>>> &register_definition)
                    : m_register_event_sink(aRegisterEventSink) {
      m_register_definition = register_definition;
      CheckRegisterIntegrity();
    }

    virtual ~MockEventSink() = default;

    void CheckRegisterIntegrity();

    const RegisterDefinition * GetRegisterDefinition(const std::string &name);

    // IPvSoftDeviceGEVEventSink implementation
    void OnApplicationConnect(IPvSoftDeviceGEV *aDevice, const PvString &aIPAddress, uint16_t aPort,
                              PvAccessType aAccessType) override {
      m_last_connect = std::make_unique<std::tuple<IPvSoftDeviceGEV*,std::string,uint16_t,PvAccessType>>
        (std::make_tuple(aDevice, aIPAddress.GetAscii(), aPort, aAccessType));
    }

    const std::tuple<IPvSoftDeviceGEV*,std::string,uint16_t,PvAccessType> *GetLastApplicationConnect() {
      return m_last_connect.get();
    }

    void OnApplicationDisconnect(IPvSoftDeviceGEV *aDevice) override {
      m_last_disconnect = aDevice;
    }

    const IPvSoftDeviceGEV* GetLastApplicationDisConnect() {
      return m_last_disconnect;
    }

    void OnControlChannelStart(IPvSoftDeviceGEV *aDevice, const PvString &aMACAddress, const PvString &aIPAddress,
                               const PvString &aMask, const PvString &aGateway, uint16_t aPort) override {
      m_last_ctrl_channel_start =
              std::make_unique<std::tuple<IPvSoftDeviceGEV*,
                std::string,
                std::string,
                std::string,
                std::string,uint16_t>>
                (std::make_tuple(aDevice,
                                 aMACAddress.GetAscii(),
                                 aIPAddress.GetAscii(),
                                 aMask.GetAscii(),
                                 aGateway.GetAscii(),
                                 aPort));
    }

    const std::tuple<IPvSoftDeviceGEV*, std::string, std::string, std::string, std::string,uint16_t> *
      GetLastControlChannelStart() { return m_last_ctrl_channel_start.get(); }

    void OnControlChannelStop(IPvSoftDeviceGEV *aDevice) override {
      m_last_ctrl_channel_stop = aDevice;
    }

    const IPvSoftDeviceGEV *GetLastControlChannelStop() {
      return m_last_ctrl_channel_stop;
    }

    void OnDeviceResetFull(IPvSoftDeviceGEV *aDevice) override {
      m_last_reset_full = aDevice;
    }
    const IPvSoftDeviceGEV* GetLastDeviceResetFull() { return m_last_reset_full; }

    void OnDeviceResetNetwork(IPvSoftDeviceGEV *aDevice) override {
      m_last_reset_network = aDevice;
    }
    const IPvSoftDeviceGEV* GetLastDeviceResetNetwork() { return m_last_reset_network; }

    void OnCreateCustomRegisters(IPvSoftDeviceGEV *aDevice, IPvRegisterFactory *aFactory) override;
    void OnCreateCustomGenApiFeatures(IPvSoftDeviceGEV *aDevice, IPvGenApiFactory *aFactory) override;

  protected:
    void CreateGenicamParameters(IPvRegisterMap *aMap, IPvGenApiFactory *aFactory);
    void CreateChunkParameters(IPvGenApiFactory *aFactory);
    void CreateEventParameters(IPvGenApiFactory *aFactory);

  private:
    std::unique_ptr<std::tuple<IPvSoftDeviceGEV*,std::string,uint16_t,PvAccessType>> m_last_connect;
    IPvSoftDeviceGEV* m_last_disconnect;
    std::unique_ptr<std::tuple<IPvSoftDeviceGEV*, std::string, std::string, std::string, std::string,uint16_t>>
      m_last_ctrl_channel_start;
    IPvSoftDeviceGEV* m_last_ctrl_channel_stop;
    IPvSoftDeviceGEV* m_last_reset_full;
    IPvSoftDeviceGEV* m_last_reset_network;
    IPvRegisterEventSink *m_register_event_sink;
    std::vector<std::pair<const RegisterDefinition*,std::variant<int, float, std::string>>> m_register_definition;
  };

  // FIXME: Add counter for given images to output
  class MockSource : public PvStreamingChannelSourceDefault {
  public:
    MockSource(size_t width, size_t height, size_t buffers=16);
    virtual ~MockSource();

    PvBuffer *AllocBuffer() override;
    void FreeBuffer(PvBuffer *aBuffer) override;
    PvResult QueueBuffer(PvBuffer *aBuffer) override;
    PvResult RetrieveBuffer(PvBuffer **aBuffer) override;
    void SetMultiPartAllowed(bool aAllowed) override;
    PvResult SetTestPayloadFormatMode(PvPayloadType aPayloadType) override;
    bool IsPayloadTypeSupported(PvPayloadType aPayloadType) override;
    uint32_t GetPayloadSize() const override;
    bool IsStreaming() const { return !m_stop; }

    void OnStreamingStart() override;
    void OnStreamingStop() override;

    // Chunk mode handling
    uint32_t GetChunksSize() const override;
    bool GetChunkModeActive() const override { return m_enabled_chunks & 0xFF000000; }
    PvResult SetChunkEnable(uint32_t aChunkID, bool aEnabled) override;
    bool GetChunkEnable( uint32_t aChunkID ) const { return m_enabled_chunks & aChunkID; }
    PvResult SetChunkModeActive(bool aEnabled) override {
      if(aEnabled) {
        m_enabled_chunks |= (LABFORGE_CHUNK_API_REV) << 24;
      } else {
        m_enabled_chunks &= ~0xFF000000;
      }


      return PvResult::Code::OK;
    }
    PvResult GetSupportedChunk(int aIndex, uint32_t &aID, PvString &aName) const override;

    PvResult MakeMultiPart(PvBuffer*buf);
    PvResult MakeImage(PvBuffer*buf);
    cv::Mat *PrepareImage(const cv::Mat *in);
    bool QueueImage(const cv::Mat *img);
    bool QueuePair(const cv::Mat *left, const cv::Mat *right);

    // Chunk data queues
    void SetDetection(chunk_dnn_t *detections);
    void SetFeatures(size_t cam_id, chunk_features_t*features);
    void SetDescriptors(size_t cam_id, chunk_descriptors_t*descriptors) { memcpy(&m_next_descriptors[cam_id], descriptors, sizeof(chunk_descriptors_t)); }

  private:
    bool GiveMultiPart();
    bool GiveImage();
    void ReturnAttachedBuffers(PvBuffer*aBuffer);
    bool FillChunks(IPvChunkData *chunk);

    PvBuffer *m_active;
    bool m_stop;
    bool m_multipart_allowed;
    const size_t m_num_buffers;
    PvFPSStabilizer m_stab;

    std::vector<std::pair<const cv::Mat*,const cv::Mat*>> m_images;
    std::vector<cv::Mat*> m_managed_images;
    uint32_t m_enabled_chunks;

    size_t m_cur_buffers{};
    size_t m_cur_image{};

    chunk_dnn_t m_next_dnn;
    chunk_features_t m_next_features[2];
    chunk_descriptors_t m_next_descriptors[2];
  };

  class MockDevice {
  public:
    MockDevice(const std::string &nic, const std::vector<std::pair<const RegisterDefinition*,
            std::variant<int, float, std::string>>> *register_definition,
            size_t width, size_t height);
    virtual ~MockDevice();

    bool QueuePair(const cv::Mat &left, const cv::Mat &right) { return m_source->QueuePair(&left, &right); }
    bool QueueImage(const cv::Mat &img) { return m_source->QueueImage(&img); }

    bool GetRegisterValue(const std::string &name, std::variant<int, float, std::string> &value) const;
    bool SetRegisterValue(const std::string &name, const std::variant<int, float, std::string> &value);

    MockEventSink*GetEventSink() const { return m_event_sink.get(); }
    MockRegisterEventSink*GetRegisterEventSink() const { return m_register_event_sink.get(); };
    MockSource*GetMockSource() const { return m_source.get(); };
  private:
    std::unique_ptr<PvSoftDeviceGEV> m_device;
    std::unique_ptr<MockEventSink> m_event_sink;
    std::unique_ptr<MockRegisterEventSink> m_register_event_sink;
    std::unique_ptr<MockSource> m_source;
  };
}
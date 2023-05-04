#pragma once

#include <esp_err.h>

#include <array>
#include <cstdint>

#include "ArtNet_Proto.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

namespace artnet {

static constexpr int DMX_BUFFER_SIZE = 512;
static constexpr int DMX_BUFFER_COUNT = 4;

static constexpr int PKT_RX_QUEUE_COUNT = 4;

class ArtNet {
 public:
  typedef struct {
    // Used to mark whether this buffer is available
    //   Marked false when dmx packet received, true when dmx packet returned with returnPacket
    bool available = true;
    uint16_t len_data;
    uint8_t data[DMX_BUFFER_SIZE];
  } dmx_data_t;

  typedef struct {
    OpCode op;
    union {
      dmx_data_t *dmx_data;  // Only valid if op == OpDmx
    };
  } pkt_artnet_rx_t;

  esp_err_t initialize();
  void loop();
  inline BaseType_t receive(pkt_artnet_rx_t *pkt_rx, TickType_t timeout) {
    return xQueueReceive(_pkt_queue, pkt_rx, timeout);
  }

  inline void returnPacket(pkt_artnet_rx_t *pkt_rx) {
    if (pkt_rx->op == OpDmx) {
      pkt_rx->dmx_data->available = true;
    }
  }

 private:
  std::array<dmx_data_t, DMX_BUFFER_COUNT>
      _dmx_packets;
  QueueHandle_t _pkt_queue;
};

}  // namespace artnet

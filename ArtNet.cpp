#include "ArtNet.hpp"

#include <esp_log.h>
#include <lwip/err.h>
#include <lwip/netdb.h>
#include <lwip/sockets.h>
#include <lwip/sys.h>

#include <cstring>

#include "ArtNet_Proto.hpp"

namespace artnet {

const char *ARTNET_ID = "Art-Net";

static const char *TAG = "artnet";
static constexpr int PACKET_BUFFER_SIZE = 1024;

esp_err_t ArtNet::initialize() {
  for (auto &p : _dmx_packets) {
    p.available = true;
  }

  _pkt_queue = xQueueCreate(PKT_RX_QUEUE_COUNT, sizeof(pkt_artnet_rx_t));
  return ESP_OK;
}

void ArtNet::loop() {
  struct sockaddr_in dest_addr;
  dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_port = htons(ARTNET_PORT);

  int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (sock < 0) {
    ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
    abort();
  }

  int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
  if (err < 0) {
    ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
    abort();
  }

  struct sockaddr source_addr;
  socklen_t socklen = sizeof(source_addr);

  while (1) {
    uint8_t buf[PACKET_BUFFER_SIZE];

    ssize_t len_rx = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr *)&source_addr, &socklen);

    if (len_rx < 0) {
      ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
      continue;
    } else if (len_rx < 12) {
      ESP_LOGW(TAG, "rx too short packet of len=%d", len_rx);
      continue;
    }

    pkt_artnet_t *pkt_artnet = (pkt_artnet_t *)buf;
    if (strcmp(pkt_artnet->ID, ARTNET_ID) != 0) {
      ESP_LOGW(TAG, "rx packet with invalid header");
      continue;
    }

    switch (pkt_artnet->OpCode) {
      case OpDmx: {
        // Validate packet data length
        pkt_artdmx_body_t *pkt_dmx = &pkt_artnet->artdmx;

        uint16_t len_dmx_data = (pkt_dmx->LengthHi << 8) | pkt_dmx->LengthLo;
        uint16_t len_expected = LEN_PKT_DMX_HEADER + sizeof(pkt_artdmx_body_t) + len_dmx_data;

        if (len_rx != len_expected) {
          ESP_LOGE(TAG, "artdmx packet length error, expected %d but got %d", len_expected, len_rx);
          break;
        }

        dmx_data_t *dmx = nullptr;
        // Find index of first available packet buffer
        for (auto &p : _dmx_packets) {
          if (p.available) {
            dmx = &p;
          }
        }

        if (dmx == nullptr) {
          ESP_LOGW(TAG, "dmx rx overflow, packet lost");
          break;
        }

        dmx->available = false;
        dmx->len_data = len_dmx_data;
        memcpy(dmx->data, pkt_artnet->artdmx.data, len_dmx_data);

        pkt_artnet_rx_t pkt_rx;
        pkt_rx.op = OpDmx;
        pkt_rx.dmx_data = dmx;

        xQueueSend(_pkt_queue, &pkt_rx, 0);
        break;
      }
      default:
        ESP_LOGW(TAG, "unhandled artdmx opcode=%04xh", pkt_artnet->OpCode);
        break;
    }
  }
}

}  // namespace artnet

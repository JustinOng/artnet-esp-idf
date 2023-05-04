#pragma once

#include <cstdint>

namespace artnet {
// Art-Net 4 Protocol Release V1.4 Document Revision 1.4df 8/3/2023

constexpr int ARTNET_PORT = 0x1936;

extern const char *ARTNET_ID;

enum OpCode : uint16_t {
  OpDmx = 0x5000
};

typedef struct __attribute__((packed)) {
  uint8_t ProtVerHi;
  uint8_t ProtVerLo;
  uint8_t Sequence;
  uint8_t Physical;
  uint8_t SubUni;
  uint8_t Net;
  uint8_t LengthHi;
  uint8_t LengthLo;
  uint8_t data[];
} pkt_artdmx_body_t;

typedef struct __attribute__((packed)) {
  char ID[8];
  uint16_t OpCode;
  union {
    pkt_artdmx_body_t artdmx;
  };
} pkt_artnet_t;

constexpr int LEN_PKT_DMX_HEADER = 8 + 2;

}  // namespace artnet

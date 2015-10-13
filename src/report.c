#include "report.h"

#include <string.h>

#include "wifi.h"
#include "rtc.h"
#include "clock.h"

#define MAX_CHUNKS 8

typedef enum {
  Chunk_Type_Battery_Level = 1,
  Chunk_Type_Bait_Level,
  Chunk_Type_Trap_Opened
} Chunk_Type;

#pragma pack(push,1)
typedef struct {
  char identifier[4];
  uint8_t protocol_version;
  uint32_t trap_id;
  uint32_t send_time;
  uint8_t n_chunks;
} Report_Header;

typedef struct {
  uint8_t type;
  uint32_t timestamp;
  uint8_t length;
} Report_Chunk_Header;

typedef struct {
  uint16_t level;
} Chunk_Battery_Level;

typedef struct {
  uint16_t bait_id;
  uint16_t level;
} Chunk_Bait_Level;

typedef struct {
  uint32_t opened_time;
} Chunk_Trap_Opened;
#pragma pack(pop)

// TODO: Resize appropriately, needs to fit biggest report we will send
char report_data[128];
char *report_data_pos;
Report_Header *report_header;

void report_init() {
  report_header = (Report_Header*)report_data;

  memcpy(report_header->identifier, "RATR", 4);
  report_header->protocol_version = 1;

  uint32_t id[2];
  rtc_read_id((uint64_t*)id);
  report_header->trap_id = id[0];

  report_header->n_chunks = 0;
  report_data_pos = &report_data[sizeof(Report_Header)];
}

void report_new() {
  // Don't really need to do anything here right now
}

static void report_add_chunk() {
  if(report_header->n_chunks >= MAX_CHUNKS) {
    Report_Chunk_Header *chunk1 =
      (Report_Chunk_Header*)&report_data[sizeof(Report_Header)];
    char *other_chunks =
      (char*)chunk1 + sizeof(Report_Chunk_Header) + chunk1->length;
    memcpy(chunk1, other_chunks, report_data_pos - other_chunks);
    report_data_pos -= other_chunks - (char*)chunk1;
  } else {
    ++report_header->n_chunks;
  }
}

void report_add_battery_level_chunk(uint16_t level) {
  report_add_chunk();

  Report_Chunk_Header *chunk_header = (Report_Chunk_Header*)report_data_pos;
  chunk_header->type = Chunk_Type_Battery_Level;
  chunk_header->timestamp = clock_get_time();
  chunk_header->length = sizeof(Chunk_Battery_Level);
  report_data_pos += sizeof(Report_Chunk_Header);

  Chunk_Battery_Level *chunk = (Chunk_Battery_Level*)report_data_pos;
  chunk->level = level;
  report_data_pos += sizeof(Chunk_Battery_Level);
}

void report_add_bait_level_chunk(uint16_t bait_id, uint16_t level) {
  report_add_chunk();

  Report_Chunk_Header *chunk_header = (Report_Chunk_Header*)report_data_pos;
  chunk_header->type = Chunk_Type_Bait_Level;
  chunk_header->timestamp = clock_get_time();
  chunk_header->length = sizeof(Chunk_Bait_Level);
  report_data_pos += sizeof(Report_Chunk_Header);

  Chunk_Bait_Level *chunk = (Chunk_Bait_Level*)report_data_pos;
  chunk->bait_id = bait_id;
  chunk->level = level;
  report_data_pos += sizeof(Chunk_Bait_Level);
}

void report_add_trap_opened_chunk(uint32_t opened_time) {
  report_add_chunk();

  Report_Chunk_Header *chunk_header = (Report_Chunk_Header*)report_data_pos;
  chunk_header->type = Chunk_Type_Trap_Opened;
  chunk_header->timestamp = clock_get_time();
  chunk_header->length = sizeof(Chunk_Trap_Opened);
  report_data_pos += sizeof(Report_Chunk_Header);

  Chunk_Trap_Opened *chunk = (Chunk_Trap_Opened*)report_data_pos;
  chunk->opened_time = opened_time;
  report_data_pos += sizeof(Chunk_Trap_Opened);
}

// NOTE: Wifi must be connected
void report_send() {
  report_header->send_time = clock_get_time();

  wifi_sendn(report_data, report_data_pos - report_data);
}

#include "spoken_data.h"


#include "esp_log.h"
#include "esp_partition.h"
#include "esp_rom_crc.h"

#include <string.h>

static const char *TAG = "spoken_audio";
static const esp_partition_t *s_partition;

bool spoken_audio_init(void)
{
    const esp_partition_t *partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "spoken_audio");
    uint32_t header[4];
    if (!partition || partition->size < SPOKEN_AUDIO_BYTES + sizeof(header) ||
        esp_partition_read(partition, 0, header, sizeof(header)) != ESP_OK ||
        memcmp(header, "SPOKEN01", 8) != 0 ||
        header[2] != SPOKEN_AUDIO_BYTES || header[3] != SPOKEN_AUDIO_CRC) {
        ESP_LOGE(TAG, "narration partition is missing or incompatible");
        return false;
    }

    uint8_t buffer[1024];
    uint32_t crc = 0;
    size_t end = sizeof(header) + SPOKEN_AUDIO_BYTES;
    for (size_t offset = sizeof(header); offset < end;) {
        size_t length = end - offset;
        if (length > sizeof(buffer)) length = sizeof(buffer);
        if (esp_partition_read(partition, offset, buffer, length) != ESP_OK) return false;
        crc = esp_rom_crc32_le(crc, buffer, length);
        offset += length;
    }
    if (crc != SPOKEN_AUDIO_CRC) {
        ESP_LOGE(TAG, "narration checksum mismatch");
        return false;
    }
    s_partition = partition;
    ESP_LOGI(TAG, "%u narrations verified: %u bytes", SPOKEN_CARD_COUNT,
             SPOKEN_AUDIO_BYTES);
    return true;
}

bool spoken_audio_read(size_t offset, void *buffer, size_t length)
{
    size_t end = 16u + SPOKEN_AUDIO_BYTES;
    if (!s_partition || offset < 16u || offset > end || length > end - offset) return false;
    return esp_partition_read(s_partition, offset, buffer, length) == ESP_OK;
}

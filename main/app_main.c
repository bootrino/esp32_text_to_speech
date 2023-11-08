/* Play mp3 file by audio pipeline using ESP32 on-board DAC

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "nvs_flash.h"

#include "sdkconfig.h"
#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_mem.h"
#include "audio_common.h"

#include "i2s_stream.h"
#include "raw_stream.h"
#include "filter_resample.h"
#include "audio_hal.h"
#include "raw_stream.h"


#include <time.h>

#include "reciter.h"
#include "sam.h"
#include "debug.h"

#include "board.h"

#include "audio_idf_version.h"

#include "bluetooth_service.h"
#include "esp_peripherals.h"



#define SAVE_FILE_RATE      22050
#define SAVE_FILE_CHANNEL   1
#define SAVE_FILE_BITS      8

#define PLAYBACK_RATE       44100
#define PLAYBACK_CHANNEL    2
#define PLAYBACK_BITS       16


int debug = 0;
static const char *TAG = "PLAY_SAM_AUDIO";

int resample_read_cb(audio_element_handle_t el, char *buf, int len, TickType_t wait_time, void *ctx)
{
    static int sam_pos = 0;
    int i;
    int read_size = GetBufferLength()/50 - sam_pos;

    if (read_size == 0) {
        return AEL_IO_DONE;
    } else if (len < read_size) {
        read_size = len;
    }
    read_size /= 2;
    char *b = GetBuffer();
    // covert 8-bits to 16-bits audio
    for (i=0; i<read_size; i++) {
        buf[i*2 + 1] = b[sam_pos];
        buf[i*2] = 0;
        sam_pos ++;
    }
    return read_size;
}

static audio_element_handle_t create_filter(int source_rate, int source_channel, int dest_rate, int dest_channel, audio_codec_type_t type)
{
    rsp_filter_cfg_t rsp_cfg = DEFAULT_RESAMPLE_FILTER_CONFIG();
    rsp_cfg.src_rate = source_rate;
    rsp_cfg.src_ch = source_channel;
    rsp_cfg.dest_rate = dest_rate;
    rsp_cfg.dest_ch = dest_channel;
    rsp_cfg.type = type;
    return rsp_filter_init(&rsp_cfg);
}

char input[255] = "HELLO MY NAME IS SAM.  AE4AE7ND- AY4 AEM DHAX LOHOW4EHST PRAY4ST- AHV DHEHM AO4UL.  QQQQ WWAW7IY1IY3IY. DHAE3T WHAHZ AH TAH4FIY.";

void app_main(void)
{
    //nvs_flash_erase();
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    // Initialize peripherals management
    // esp_periph_config_t periph_cfg = { 0 };
    // esp_periph_set_handle_t set = esp_periph_init(&periph_cfg);
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);

    audio_pipeline_handle_t pipeline;
    audio_element_handle_t resample_decoder, bt_stream_writer;

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set(TAG, ESP_LOG_INFO);

    ESP_LOGI(TAG, "[ 1 ] Create Bluetooth service");
    bluetooth_service_cfg_t bt_cfg = {
        .device_name = "ESP-ADF-SOURCE",
        .mode = BLUETOOTH_A2DP_SOURCE,
        .remote_name = CONFIG_BT_REMOTE_NAME,
    };
    bluetooth_service_start(&bt_cfg);

    ESP_LOGI(TAG, "[1.1] Get Bluetooth stream");
    bt_stream_writer = bluetooth_service_create_stream();

    ESP_LOGI(TAG, "[ 4 ] Create audio pipeline for BT Source");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    mem_assert(pipeline);

    ESP_LOGI(TAG, "[3.1] Create raw stream to write data");
    raw_stream_cfg_t raw_cfg = RAW_STREAM_CFG_DEFAULT();
    raw_cfg.type = AUDIO_STREAM_WRITER;
    audio_element_handle_t el_raw_write = raw_stream_init(&raw_cfg);

    ESP_LOGI(TAG, "[2.1] Create mp3 decoder to decode mp3 file and set custom read callback");
    resample_decoder = create_filter(22050, 1, 44100, 2, AUDIO_CODEC_TYPE_ENCODER);
    audio_element_set_read_cb(resample_decoder, resample_read_cb, NULL);

    ESP_LOGI(TAG, "[2.3] Register all elements to audio pipeline");
    audio_pipeline_register(pipeline, resample_decoder, "rsp");
    audio_pipeline_register(pipeline, bt_stream_writer,   "bt");
    audio_pipeline_register(pipeline, el_raw_write, "raw");

    ESP_LOGI(TAG, "[2.4] Link it together [resample_read_cb]-->resample_decoder-->wav_decoder-->bt_stream_writer");
    audio_pipeline_link(pipeline, (const char *[]) {"rsp", "bt"}, 2);

    ESP_LOGI(TAG, "[5.1] Create Bluetooth peripheral");
    esp_periph_handle_t bt_periph = bluetooth_service_create_periph();

    ESP_LOGI(TAG, "[5.2] Start Bluetooth peripheral");
    esp_periph_start(set, bt_periph);

    ESP_LOGI(TAG, "[ 3 ] Setup event listener");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

    ESP_LOGI(TAG, "[3.1] Listening event from all elements of pipeline");
    audio_pipeline_set_listener(pipeline, evt);

    ESP_LOGI(TAG, "[6.2] Listening event from peripherals");
    //audio_event_iface_set_listener(esp_periph_get_event_iface(), evt);
    audio_event_iface_set_listener(esp_periph_set_get_event_iface(set), evt);


    printf("text input: %s\n", input);

    if (!TextToPhonemes((unsigned char *)input)) return;

    printf("phonetic input: %s\n", input);

    /*
    DESCRIPTION          SPEED     PITCH     THROAT    MOUTH
    Elf                   72        64        110       160
    Little Robot          92        60        190       190
    Stuffy Guy            82        72        110       105
    Little Old Lady       82        32        145       145
    Extra-Terrestrial    100        64        150       200
    SAM                   72        64        128       128
    // robot
    SetSpeed(92);
    SetPitch(60);
    SetThroat(190);
    SetMouth(190);
    // Extra-Terrestrial
    SetSpeed(100);
    SetPitch(64);
    SetThroat(150);
    SetMouth(200);
    // Elf
    SetSpeed(72);
    SetPitch(64);
    SetThroat(110);
    SetMouth(160);
    */
    // robot
    SetSpeed(CONFIG_SAM_SPEED);
    SetPitch(CONFIG_SAM_PITCH);
    SetThroat(CONFIG_SAM_THROAT);
    SetMouth(CONFIG_SAM_MOUTH);

    SetInput(input);


    if (!SAMMain())
    {
        ESP_LOGE(TAG, "SAM error");
        return;
    }

    ESP_LOGI(TAG, "[ 4 ] Start audio_pipeline");
    audio_pipeline_run(pipeline);

    while (1) {
        audio_event_iface_msg_t msg;
        esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "[ * ] Event interface error : %d", ret);
            continue;
        }

        ESP_LOGI(TAG, "[ * ] Audio event");
        ESP_LOGI(TAG, "[ * ] Free heap: %u", xPortGetFreeHeapSize());
        ESP_LOGI(TAG, "[ * ] msg.cmd: %d", msg.cmd);

        /* Stop when the Bluetooth is disconnected or suspended */
        if (msg.source_type == PERIPH_ID_BLUETOOTH
                && msg.source == (void *)bt_periph) {
            if ((msg.cmd == PERIPH_BLUETOOTH_DISCONNECTED) || (msg.cmd == PERIPH_BLUETOOTH_AUDIO_SUSPENDED)) {
                ESP_LOGW(TAG, "[ * ] Bluetooth disconnected or suspended");
                periph_bluetooth_stop(bt_periph);
                break;
            }
        }


        /* Stop when the last pipeline element (bt_stream_writer in this case) receives stop event */
        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) bt_stream_writer
                && msg.cmd == AEL_MSG_CMD_REPORT_STATUS && (int) msg.data == AEL_STATUS_STATE_STOPPED) {
                ESP_LOGW(TAG, "[ * ] Audio stop event");
                
            break;
        }

    }

}

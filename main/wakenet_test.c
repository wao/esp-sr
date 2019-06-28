#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "dl_lib_coefgetter_if.h"
#include "wakenet_test.h"
#include "hilexin.h"

static const esp_wn_iface_t *wakenet = &WAKENET_MODEL;
static const model_coeff_getter_t *model_coeff_getter = &WAKENET_COEFF;

void wakenetTask(void *arg)
{
    model_iface_data_t *model_data = arg;
    int frequency = wakenet->get_samp_rate(model_data);
    int audio_chunksize = wakenet->get_samp_chunksize(model_data);
    int16_t *buffer = malloc(audio_chunksize * sizeof(int16_t));
    assert(buffer);

    int chunks = 0;
    while (1) {
        if ((chunks + 1)*audio_chunksize * sizeof(int16_t) <= sizeof(hilexin)) {
            memcpy(buffer, hilexin + chunks * audio_chunksize * sizeof(int16_t), audio_chunksize * sizeof(int16_t));
        } else {
            break;
        }
        int r = wakenet->detect(model_data, buffer);
        if (r) {
            int ms = (chunks * audio_chunksize * 1000) / frequency;
            printf("WN test successfully, %.2f: Neural network detection triggered output %d.\n", (float)ms / 1000.0, r);
        }
        chunks++;
    }
    printf("TEST FINISHED\n\n");
    vTaskDelete(NULL);
}

void wakenet_test()
{
    int start_size = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    printf("Start free RAM size: %d\n", start_size);

    //Initialize wakenet model
    model_iface_data_t *model_data = wakenet->create(model_coeff_getter, DET_MODE_90);
    int audio_chunksize = wakenet->get_samp_chunksize(model_data);
    printf("WakeNet RAM size: %d\nRAM size after WakeNet init: %d\n",
           start_size - heap_caps_get_free_size(MALLOC_CAP_8BIT), heap_caps_get_free_size(MALLOC_CAP_8BIT));

    xTaskCreatePinnedToCore(&wakenetTask, "wakenet", 2 * 1024, (void*)model_data, 5, NULL, 1);
}

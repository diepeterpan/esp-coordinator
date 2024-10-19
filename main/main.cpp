#include "esp_log.h"
#include "app.h"

extern "C" void app_main(void)
{
    ESP_ERROR_CHECK(app::init());
    ESP_ERROR_CHECK(app::start());
}

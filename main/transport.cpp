#include "transport.h"
#include "app.h"
#include "utils.h"

#include <esp_log.h>
#include "sdkconfig.h"

#if defined(CONFIG_NCP_BUS_MODE_UART)
#include <driver/uart.h>
static constexpr uart_port_t UART_PORT_NUM = static_cast<uart_port_t>(CONFIG_NCP_BUS_UART_NUM);
#elif defined(CONFIG_NCP_BUS_MODE_USB)
#include <driver/usb_serial_jtag.h>
#endif

#include <cstring>

static const char* TAG = "TRNPT";

transport::transport() {

}

transport& transport::instance() {
	static transport s_transport;
	return s_transport;
}

esp_err_t transport::write_int(const void *buffer, size_t size)
{
#if defined(CONFIG_NCP_BUS_MODE_UART)
    return (uart_write_bytes(UART_PORT_NUM, (const char*) buffer, size) == size) ? ESP_OK : ESP_ERR_INVALID_SIZE;
#elif defined(CONFIG_NCP_BUS_MODE_USB)
    return (usb_serial_jtag_write_bytes(buffer, size, pdMS_TO_TICKS(RINGBUF_TIMEOUT_MS)) == size) ? ESP_OK : ESP_ERR_INVALID_SIZE;
#endif
}


void transport::task_int() {
#if defined(CONFIG_NCP_BUS_MODE_UART)
    uart_event_t event;
#endif
   
    app::ctx_t ncp_event = {
        .event = app::EVENT_OUTPUT,
        .size = 0
    };

    while (true) {
#if defined(CONFIG_NCP_BUS_MODE_UART)
        if (xQueueReceive(m_uart_queue, (void *)&event, (TickType_t)portMAX_DELAY)) {
            memset(m_temp_buf,0, sizeof(m_temp_buf));
            switch(event.type) {
                case UART_DATA: {
                    ncp_event.size = uart_read_bytes(UART_PORT_NUM, m_temp_buf, event.size, portMAX_DELAY);
                    //ESP_LOGD(TAG, "UART READ %d", ncp_event.size );
                    //ESP_LOG_BUFFER_HEX_LEVEL(TAG, m_temp_buf, ncp_event.size, ESP_LOG_DEBUG);
                    auto stored = xStreamBufferSend(m_output_buf, m_temp_buf, ncp_event.size, 0);
                    if (stored != ncp_event.size) {
                    	ESP_LOGE(TAG,"Failed store to output buffer %d %d",stored,ncp_event.size);
                    	ncp_event.size = stored;
                    }
                    app::send_event(ncp_event);
                } break;
                case UART_FIFO_OVF:
                    ESP_LOGI(TAG, "hw fifo overflow");
                    uart_flush_input(UART_PORT_NUM);
                    xQueueReset(m_uart_queue);
                    break;
                case UART_BUFFER_FULL:
                    ESP_LOGI(TAG, "ring buffer full");
                    uart_flush_input(UART_PORT_NUM);
                    xQueueReset(m_uart_queue);
                    break;
                default:
                    ESP_LOGI(TAG, "uart event type: %d", event.type);
                    break;
            }
        }
#elif defined(CONFIG_NCP_BUS_MODE_USB)
        int readed = usb_serial_jtag_read_bytes(m_temp_buf,BUF_SIZE,pdMS_TO_TICKS(10));
        if (readed > 0) {
            ncp_event.size = readed;
            xStreamBufferSend(m_output_buf, m_temp_buf, readed, 0);
            app::send_event(ncp_event);
        }
#endif
    }
}

size_t transport::output_receive(void* buffer,size_t size) {
	return xStreamBufferReceive(instance().m_output_buf, buffer, size, pdMS_TO_TICKS(RINGBUF_TIMEOUT_MS));
}

esp_err_t transport::process_input_int(void* buffer,size_t size) {
	auto recv_size = xStreamBufferReceive(m_input_buf, buffer, size, pdMS_TO_TICKS(RINGBUF_TIMEOUT_MS));
    if (recv_size != size) {
        ESP_LOGE(TAG, "Input buffer receive error: size %d expect %d!", recv_size, size);
        return ESP_FAIL;
    } else {
        //ESP_LOGD(TAG, "Bus write len %d", ctx.size);
        return write_int(buffer, size);
    }
}

esp_err_t transport::send_int(const void* data,size_t size) {
	if (data == NULL) {
        return ESP_FAIL;
    }

    app::ctx_t ncp_event = {
        .event = app::EVENT_INPUT,
        .size = 0
    };

    int count = RINGBUF_TIMEOUT_MS / 10;
    size_t ret_size = 0;


    while (xStreamBufferSpacesAvailable(m_input_buf) < size) {
        if(count == 0) break;
        vTaskDelay(pdMS_TO_TICKS(10));
        count --;
    }

    if (count == 0) {
        ESP_LOGE(TAG, "input_buf not enough");
        return ESP_FAIL;
    }

    {
    	utils::sem_lock l(m_input_sem);
    	ret_size = xStreamBufferSend(m_input_buf, data, size, 0);
    }
    
    if (ret_size != size) {
        ESP_LOGE(TAG, "input_buf send error: size %d expect %d", ret_size, size);
        return ESP_FAIL;
    } else {
        ncp_event.size = size;
        return app::send_event(ncp_event);
    }
}

esp_err_t transport::init_int() {
	
	ESP_LOGI(TAG,"init");

    m_input_buf = xStreamBufferCreate(RINGBUF_SIZE, 8);
    if (!m_input_buf) {
        ESP_LOGE(TAG, "Input buffer create error");
        return ESP_ERR_NO_MEM;
    }

    m_output_buf = xStreamBufferCreate(RINGBUF_SIZE, 8);
    if (!m_output_buf) {
        ESP_LOGE(TAG, "Out buffer create error");
        return ESP_ERR_NO_MEM;
    }

    m_input_sem = xSemaphoreCreateMutex();
    if (!m_input_sem) {
        ESP_LOGE(TAG, "Input semaphore create error");
        return ESP_ERR_NO_MEM;
    }


#if defined(CONFIG_NCP_BUS_MODE_UART)
    uart_config_t uart_config = {
        .baud_rate = CONFIG_NCP_BUS_UART_BAUD_RATE,
        .data_bits = static_cast<uart_word_length_t>(CONFIG_NCP_BUS_UART_BYTE_SIZE),
        .parity = UART_PARITY_DISABLE,
        .stop_bits = static_cast<uart_stop_bits_t>(CONFIG_NCP_BUS_UART_STOP_BITS),
        .flow_ctrl = static_cast<uart_hw_flowcontrol_t>(CONFIG_NCP_BUS_UART_FLOW_CONTROL),
        .source_clk = UART_SCLK_DEFAULT,
    };

    auto res = uart_driver_install(UART_PORT_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 20, &m_uart_queue, 0);
    if (res != ESP_OK) {
    	return res;
    }
    uart_param_config(UART_PORT_NUM, &uart_config);
    uart_set_pin(UART_PORT_NUM, CONFIG_NCP_BUS_UART_TX_PIN, CONFIG_NCP_BUS_UART_RX_PIN, CONFIG_NCP_BUS_UART_RTS_PIN, CONFIG_NCP_BUS_UART_CTS_PIN);
    return ESP_OK;
#elif defined(CONFIG_NCP_BUS_MODE_USB)
    usb_serial_jtag_driver_config_t usb_serial_jtag_config;
    usb_serial_jtag_config.rx_buffer_size = BUF_SIZE * 2;
    usb_serial_jtag_config.tx_buffer_size = BUF_SIZE * 2;
    return usb_serial_jtag_driver_install(&usb_serial_jtag_config);
#else
    #error "unknown transport";
#endif
}

esp_err_t transport::start_int() {
	if (!m_input_buf) {
		ESP_LOGE(TAG,"need init");
		return ESP_FAIL;
	}
	ESP_LOGI(TAG,"start");
	return (xTaskCreate(&task, "transport",TASK_STACK, this,TASK_PRIORITY, NULL) == pdTRUE) ? ESP_OK : ESP_FAIL;
}
#pragma once
#include <cstdint>
#include <cstring>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace utils {

	uint8_t crc8(const void* data,size_t size);
	uint16_t crc16(const void* data,size_t size);

	class sem_lock {
		SemaphoreHandle_t m_sem;
	public:
		sem_lock(SemaphoreHandle_t sem) : m_sem(sem) {
			xSemaphoreTake(m_sem, portMAX_DELAY);
		}
		~sem_lock() {
			xSemaphoreGive(m_sem);
		}
	};

	const char* get_zdp_status_str(uint8_t status);
	const char* get_nlme_status_str(uint8_t status);


#define IEEE_ADDR_FMT "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x"
#define IEEE_ADDR_PRINT(a) \
    static_cast<unsigned int>(a[0]),static_cast<unsigned int>(a[1]),static_cast<unsigned int>(a[2]),static_cast<unsigned int>(a[3]), \
    static_cast<unsigned int>(a[4]),static_cast<unsigned int>(a[5]),static_cast<unsigned int>(a[6]),static_cast<unsigned int>(a[7])

}
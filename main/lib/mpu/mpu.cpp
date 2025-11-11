#include "mpu.h"
#include "esp_random.h"

int* mpu(void)
{
    static int data[8];
    data[0] = 100 + (esp_random() % 9901);  // 100-10000
    data[1] = 100 + (esp_random() % 9901);
    data[2] = 100 + (esp_random() % 9901);
    data[3] = 100 + (esp_random() % 9901);
    data[4] = 100 + (esp_random() % 9901);
    data[5] = 100 + (esp_random() % 9901);
    data[6] = 100 + (esp_random() % 9901);
    data[7] = 100 + (esp_random() % 9901);
    data[8] = 100 + (esp_random() % 9901);
    return data;
}
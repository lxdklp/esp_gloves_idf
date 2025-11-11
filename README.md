## esp 部分代码
C++ 开发,使用 ESP-IDF 框架,本仓库
#### 文件夹说明
- [main](./main/): 代码/cmake/依赖
- [main/lib/](./main/lib/): 库文件
- [main/lib/api/](./main/lib/api/): API HTTP 服务器
- [main/lib/mpu/](./main/lib/mpu/): MPU6050 传感器
- [main/lib/wifi/](./main/lib/wifi/): WiFi 连接

## app 部分代码
Dart 开发,使用 Flutter 框架,仓库:[esp_gloves_app](https://github.com/lxdklp/esp_gloves_app)

### API文档
| 方法 | 类型 | 说明 |
| ---- | ---- | ---- |
| / | GET | hello |
| /v1/status | GET | 状态 |
| /v1/mpu | GET | 获取 MPU 数据 |
| /v1/info | GET | 获取 网络/软件/硬件 信息 |
#define DEV

#define MOTOR_kP (0.1)
#define MOTOR_kI (0)
#define MOTOR_kD (0)

#define SPI_INTERFACE spi0
#define SPI_HZ 1000000

#define MPU_CLK 18
#define MPU_MOSI 19
#define MPU_MISO 16
#define MPU_CS 17
#define MPU_RESOLUTION (32)

#define MPU_ID_REG (0x00)
#define MPU_DATAX (0x33)
#define MPU_DATAY (0x35)
#define MPU_DATAZ (0x37)
#define MPU_OFFSET_X (0x1E)
#define MPU_OFFSET_Y (0x1F)
#define MPU_OFFSET_Z (0x20)
#define MPU_DATA_FORMAT (0x31)
#define MPU_POWER_CTL (0x2D)
#define MPU_BW (0x2C)

#define OFFSET_X (-74)
#define OFFSET_Y (66)
#define OFFSET_Z (0)

#define BUF_LEN (0x100)

#define TARGET_TIME = 73000;
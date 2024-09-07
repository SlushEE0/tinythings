#define C_GRAV (9.8)

#define ADXL_RES (256)
#define CALIB_X (0.1)
#define CALIB_Y (0.2)
#define CALIB_Z (0.3)

static const char *TAG = "MAIN";

typedef struct Velo {
  double x;
  double y;
  double z;
} Velo;

typedef struct Accel {
  double x;
  double y;
  double z;
} Accel;

typedef struct Displacement {
  double x;
  double y;
  double z;
} Displacement;

Velo velo = {0.0, 0.0, 0.0};
Accel accel = {0.0, 0.0, 0.0};
Displacement disp = {0.0, 0.0, 0.0};
Displacement pose = {0.0, 0.0, 0.0};

void setupADXL() {
  // write to callibration registers
}

Accel *updateAccel(Accel *accel) {
  double accelX_read = 0.0;
  double accelY_read = 0.0;
  double accelZ_read = 0.0;

  double accelX = ((accelX_read - CALIB_X) / ADXL_RES) * C_GRAV;
  double accelY = ((accelY_read - CALIB_Y) / ADXL_RES) * C_GRAV;
  double accelZ = ((accelZ_read - CALIB_Z) / ADXL_RES) * C_GRAV;

  accel->x = accelX;
  accel->y = accelY;
  accel->z = accelZ;

  return accel;
}

Displacement *updateDisplacement(Displacement *disp, double time) {
  

  return disp;
}

double updateVelo(double time) {
}

void app_main() {
  updateAccel(&accel);
}

//Note: Install new AdaFruit Library for VL53L0X sensor
#include <Wire.h>
#include <Adafruit_VL53L0X.h>

Adafruit_VL53L0X sensor;

const float TIP_TO_SENSOR_MM = 171.63f;


// Entries at the same position must correspond to each other.
const float tipDistanceMM[] = {
  53.89, 57.80, 61.57, 66.72, 72.69,
  78.89, 83.49, 87.10, 90.27, 94.96,
  98.31, 101.88, 106.24, 109.43, 112.62
};

const float density[] = {
  0.990, 0.996, 1.002, 1.010, 1.020,
  1.030, 1.038, 1.044, 1.050, 1.058,
  1.064, 1.070, 1.078, 1.084, 1.090
};


const size_t STABLE_COUNT = 10;
const float MAX_DENSITY_SPREAD = 0.004f;

float recentDensities[STABLE_COUNT];
size_t readingCount = 0;
size_t nextIndex = 0;

bool densityLocked = false;
float lockedDensity = 0.0f;

const size_t POINT_COUNT = sizeof(tipDistanceMM) / sizeof(tipDistanceMM[0]);

void performNextTask(float stableDensity) {
  // Calculate your conversion factor from stableDensity here,
  // then perform the task that uses it.
  Serial.print("Next task can use density: ");
  Serial.println(stableDensity, 4);
}

bool interpolateDensity(float distanceFromTip, float &result) {
  // Do not estimate beyond the calibrated range.
  if (distanceFromTip < tipDistanceMM[0] ||
      distanceFromTip > tipDistanceMM[POINT_COUNT - 1]) {
    return false;
  }

  for (size_t i = 0; i < POINT_COUNT - 1; i++) {
    if (distanceFromTip <= tipDistanceMM[i + 1]) {
      float fraction = (distanceFromTip - tipDistanceMM[i]) /
                       (tipDistanceMM[i + 1] - tipDistanceMM[i]);

      result = density[i] +
               fraction * (density[i + 1] - density[i]);
      return true;
    }
  }

  return false;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  if (!sensor.begin()) {
    Serial.println("VL53L0X not found.");
    while (true) {}
  }
}

void loop() {
  if (densityLocked) {
    return;  // Keep the locked value until the Arduino is reset.
  }

  VL53L0X_RangingMeasurementData_t measurement;
  sensor.rangingTest(&measurement, false);

  if (measurement.RangeStatus != 0) {
    delay(500);
    return;  // Skip an occasional invalid sensor reading.
  }

  float distanceFromTipMM =
      TIP_TO_SENSOR_MM - measurement.RangeMilliMeter;

  float currentDensity;
  if (!interpolateDensity(distanceFromTipMM, currentDensity)) {
    readingCount = 0;  // Start a fresh window after an out-of-range reading.
    nextIndex = 0;
    delay(500);
    return;
  }

  Serial.print("Current density: ");
  Serial.println(currentDensity, 4);

  recentDensities[nextIndex] = currentDensity;
  nextIndex = (nextIndex + 1) % STABLE_COUNT;

  if (readingCount < STABLE_COUNT) {
    readingCount++;
  }

  if (readingCount == STABLE_COUNT) {
    float minimum = recentDensities[0];
    float maximum = recentDensities[0];
    float sum = 0.0f;

    for (size_t i = 0; i < STABLE_COUNT; i++) {
      if (recentDensities[i] < minimum) minimum = recentDensities[i];
      if (recentDensities[i] > maximum) maximum = recentDensities[i];
      sum += recentDensities[i];
    }

    if (maximum - minimum <= MAX_DENSITY_SPREAD) {
      lockedDensity = sum / STABLE_COUNT;
      densityLocked = true;

      Serial.print("LOCKED density: ");
      Serial.println(lockedDensity, 4);

      performNextTask(lockedDensity);  // Runs once, upon locking.
    }
  }

  delay(500);
}





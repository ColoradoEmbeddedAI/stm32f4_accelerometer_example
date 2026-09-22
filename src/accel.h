/* accel.h -- accelerometer source interface, implemented by exactly one of:
 *
 *   accel_real.c       (ACCEL_SOURCE=real, the default)      -- SPI1 driver
 *                       for the onboard LIS3DSH/LIS302DL, real hardware only.
 *   accel_synthetic.c  (ACCEL_SOURCE=synthetic, for Renode)   -- replays a
 *                       CSV-derived sample array (src/generated/
 *                       synthetic_accel_data.h) instead of touching SPI1;
 *                       Renode has no model for either accelerometer chip.
 *
 * main.c calls only this interface and does not know which implementation
 * it is linked against -- see ../CMakeLists.txt's ACCEL_SOURCE option. */
#ifndef ACCEL_H
#define ACCEL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { ACCEL_NONE = 0, ACCEL_LIS3DSH, ACCEL_LIS302DL, ACCEL_SYNTHETIC } accel_t;

/* Brings up whatever hardware this build's source needs (SPI1 + CS pin and
 * WHO_AM_I detection for accel_real.c; nothing for accel_synthetic.c) and
 * prints a one-line UART diagnostic describing what it found. Returns
 * ACCEL_NONE if no supported chip was detected (accel_real.c only -- the
 * synthetic build always succeeds). Assumes hal_init() already ran. */
accel_t accel_init(void);

/* Short chip name used in the READY/START ACK protocol lines (see main.c),
 * e.g. "LIS3DSH", "LIS302DL", "SYNTHETIC". */
const char *accel_name(accel_t chip);

/* Reads (accel_real.c) or produces (accel_synthetic.c) one X/Y/Z sample in
 * mg. Called once per print interval by main.c's capture loop, which is
 * what paces the 200 ms sample rate -- neither implementation needs its own
 * timing logic. */
void accel_read_mg(accel_t chip, int32_t *mx, int32_t *my, int32_t *mz);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ACCEL_H */

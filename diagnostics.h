/**
 * @file    diagnostics.h
 * @brief   Transport-agnostic diagnostic service layer, loosely modeled on
 *          ISO 14229 (UDS). Handles session control, DTC read/clear, PID
 *          reads, ECU reset and a toy security-access seed/key exchange.
 *
 *          The same Diag_ProcessRequest() is called whether the request
 *          arrived as a CAN frame or as a parsed UART console command —
 *          see diagnostics.c for the two thin adapter functions.
 */
#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <stdint.h>
#include <stdbool.h>
#include "can_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Service IDs (subset of ISO 14229-1) -------------------------------- */
#define SID_DIAGNOSTIC_SESSION_CONTROL   0x10U
#define SID_ECU_RESET                    0x11U
#define SID_CLEAR_DIAGNOSTIC_INFO        0x14U
#define SID_READ_DTC_INFORMATION         0x19U
#define SID_READ_DATA_BY_IDENTIFIER      0x22U
#define SID_SECURITY_ACCESS              0x27U
#define SID_TESTER_PRESENT               0x3EU
#define SID_NEGATIVE_RESPONSE            0x7FU

/* Negative response codes (subset) */
#define NRC_SERVICE_NOT_SUPPORTED        0x11U
#define NRC_SUBFUNCTION_NOT_SUPPORTED    0x12U
#define NRC_CONDITIONS_NOT_CORRECT       0x22U
#define NRC_REQUEST_OUT_OF_RANGE         0x31U
#define NRC_SECURITY_ACCESS_DENIED       0x33U

/* Session types (sub-function of 0x10) */
typedef enum {
    SESSION_DEFAULT    = 0x01,
    SESSION_PROGRAMMING = 0x02,
    SESSION_EXTENDED    = 0x03
} DiagSession_t;

/* ---- Live data identifiers (subset, loosely OBD-II PID inspired) ------- */
#define PID_ENGINE_RPM        0x0C00U
#define PID_COOLANT_TEMP_C    0x0500U
#define PID_VEHICLE_SPEED     0x0D00U
#define PID_BATTERY_VOLTAGE   0x0F00U

/* ---- DTCs ---------------------------------------------------------------*/
#define DTC_MAX_COUNT 8U

typedef struct {
    uint32_t code;          /* e.g. 0xP0128 packed as 0x00P0128 */
    const char *name;
    uint8_t  status;        /* bit0=testFailed, bit1=confirmed, bit2=pending ... */
    bool     active;
} DtcEntry_t;

/* ---- Public API ---------------------------------------------------------*/
void Diag_Init(void);

/* Generic entry point: request/response as raw byte buffers (payload only,
 * no transport framing). Returns response length, 0 on internal error. */
uint8_t Diag_ProcessRequest(const uint8_t *req, uint8_t req_len,
                             uint8_t *resp, uint8_t resp_max_len);

/* Transport adapters */
void Diag_HandleCanFrame(const CanFrame_t *frame);
void Diag_HandleConsoleLine(const char *line);

/* Called periodically (e.g. every FAULT_SIM_PERIOD_MS) to simulate sensor
 * drift and set/clear DTCs, so the diagnostic flow has something to report. */
void Diag_FaultSimulatorTick(void);

/* Session timeout housekeeping — call once per ms from the scheduler. */
void Diag_SessionTick(uint32_t elapsed_ms);

#ifdef __cplusplus
}
#endif

#endif /* DIAGNOSTICS_H */

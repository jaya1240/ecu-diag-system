/**
 * @file diagnostics.c
 * @brief Core diagnostic service dispatcher. See diagnostics.h for the
 *        service table this implements.
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "diagnostics.h"
#include "uart_driver.h"
#include "can_driver.h"
#include "main.h"

/* ---- Session / security state ------------------------------------------*/
static DiagSession_t s_session = SESSION_DEFAULT;
static uint32_t      s_session_idle_ms = 0U;
static bool           s_security_unlocked = false;
static uint16_t       s_pending_seed = 0U;

/* ---- Simulated live data (fault simulator perturbs these) --------------*/
static uint16_t s_rpm            = 800U;   /* idle RPM            */
static int8_t   s_coolant_temp_c = 85;     /* deg C                */
static uint8_t  s_vehicle_speed  = 0U;     /* km/h                 */
static uint16_t s_battery_mv     = 12600U; /* mV                   */

/* ---- DTC table -----------------------------------------------------------
 * status bit0 = testFailed, bit1 = confirmedDTC, bit2 = pendingDTC,
 * bit3 = testNotCompletedSinceLastClear (kept simple on purpose)
 */
static DtcEntry_t s_dtc_table[DTC_MAX_COUNT] = {
    { 0x000128, "P0128 - Coolant Thermostat (below regulating temp)", 0x00, false },
    { 0x000562, "P0562 - System Voltage Low",                          0x00, false },
    { 0x000420, "P0420 - Catalyst System Efficiency Below Threshold",  0x00, false },
};
static const uint8_t s_dtc_table_len = 3U;

/* ==========================================================================
 * Init
 * ========================================================================*/
void Diag_Init(void)
{
    s_session = SESSION_DEFAULT;
    s_session_idle_ms = 0U;
    s_security_unlocked = false;
    s_pending_seed = 0U;
}

/* ==========================================================================
 * Helpers
 * ========================================================================*/
static void set_dtc(uint8_t index, bool active)
{
    if (index >= s_dtc_table_len) {
        return;
    }
    s_dtc_table[index].active = active;
    if (active) {
        s_dtc_table[index].status |= 0x03U; /* testFailed + confirmed */
    } else {
        s_dtc_table[index].status = 0x00U;
    }
}

static uint16_t simple_seed(void)
{
    /* Not cryptographically meaningful — this is a bench/demo gate, not a
     * real security concept. Replace with a proper challenge if this ever
     * talks to anything that matters. */
    return (uint16_t)(0xA5A5U ^ (HAL_GetTick() & 0xFFFFU));
}

static bool key_matches(uint16_t seed, uint16_t key)
{
    return key == (uint16_t)(seed ^ 0x1234U);
}

/* ==========================================================================
 * Service handlers — operate on payload bytes only (SID + params),
 * no transport framing. Each returns the number of response bytes written.
 * ========================================================================*/
static uint8_t handle_session_control(const uint8_t *req, uint8_t len, uint8_t *resp)
{
    if (len < 2U) {
        resp[0] = SID_NEGATIVE_RESPONSE; resp[1] = SID_DIAGNOSTIC_SESSION_CONTROL; resp[2] = NRC_SUBFUNCTION_NOT_SUPPORTED;
        return 3U;
    }
    uint8_t sub = req[1];
    if (sub != SESSION_DEFAULT && sub != SESSION_PROGRAMMING && sub != SESSION_EXTENDED) {
        resp[0] = SID_NEGATIVE_RESPONSE; resp[1] = SID_DIAGNOSTIC_SESSION_CONTROL; resp[2] = NRC_SUBFUNCTION_NOT_SUPPORTED;
        return 3U;
    }
    s_session = (DiagSession_t)sub;
    s_session_idle_ms = 0U;
    if (s_session == SESSION_DEFAULT) {
        s_security_unlocked = false; /* dropping back to default relocks */
    }
    resp[0] = (uint8_t)(SID_DIAGNOSTIC_SESSION_CONTROL + 0x40U); /* positive resp */
    resp[1] = sub;
    return 2U;
}

static uint8_t handle_ecu_reset(const uint8_t *req, uint8_t len, uint8_t *resp)
{
    (void)req; (void)len;
    resp[0] = (uint8_t)(SID_ECU_RESET + 0x40U);
    resp[1] = 0x01U; /* hard reset sub-function echoed */
    /* Real firmware would flush state then call HAL_NVIC_SystemReset() after
     * the response is flushed out. Left as a hook for the caller. */
    return 2U;
}

static uint8_t handle_clear_dtc(const uint8_t *req, uint8_t len, uint8_t *resp)
{
    (void)req; (void)len;
    for (uint8_t i = 0U; i < s_dtc_table_len; i++) {
        set_dtc(i, false);
    }
    resp[0] = (uint8_t)(SID_CLEAR_DIAGNOSTIC_INFO + 0x40U);
    return 1U;
}

static uint8_t handle_read_dtc(const uint8_t *req, uint8_t len, uint8_t *resp)
{
    (void)req; (void)len;
    uint8_t count = 0U;
    uint8_t idx = 2U;
    resp[0] = (uint8_t)(SID_READ_DTC_INFORMATION + 0x40U);
    resp[1] = 0x02U; /* reportDTCByStatusMask sub-function echoed */

    for (uint8_t i = 0U; i < s_dtc_table_len && idx + 4U <= 62U; i++) {
        if (!s_dtc_table[i].active) {
            continue;
        }
        resp[idx++] = (uint8_t)((s_dtc_table[i].code >> 16) & 0xFFU);
        resp[idx++] = (uint8_t)((s_dtc_table[i].code >> 8) & 0xFFU);
        resp[idx++] = (uint8_t)(s_dtc_table[i].code & 0xFFU);
        resp[idx++] = s_dtc_table[i].status;
        count++;
    }
    (void)count;
    return idx;
}

static uint8_t handle_read_data_by_id(const uint8_t *req, uint8_t len, uint8_t *resp)
{
    if (len < 3U) {
        resp[0] = SID_NEGATIVE_RESPONSE; resp[1] = SID_READ_DATA_BY_IDENTIFIER; resp[2] = NRC_REQUEST_OUT_OF_RANGE;
        return 3U;
    }
    uint16_t pid = (uint16_t)((req[1] << 8) | req[2]);
    resp[0] = (uint8_t)(SID_READ_DATA_BY_IDENTIFIER + 0x40U);
    resp[1] = req[1];
    resp[2] = req[2];

    switch (pid) {
        case PID_ENGINE_RPM:
            resp[3] = (uint8_t)(s_rpm >> 8);
            resp[4] = (uint8_t)(s_rpm & 0xFFU);
            return 5U;
        case PID_COOLANT_TEMP_C:
            resp[3] = (uint8_t)(s_coolant_temp_c + 40); /* offset like real OBD PID 0x05 */
            return 4U;
        case PID_VEHICLE_SPEED:
            resp[3] = s_vehicle_speed;
            return 4U;
        case PID_BATTERY_VOLTAGE:
            resp[3] = (uint8_t)(s_battery_mv >> 8);
            resp[4] = (uint8_t)(s_battery_mv & 0xFFU);
            return 5U;
        default:
            resp[0] = SID_NEGATIVE_RESPONSE; resp[1] = SID_READ_DATA_BY_IDENTIFIER; resp[2] = NRC_REQUEST_OUT_OF_RANGE;
            return 3U;
    }
}

static uint8_t handle_security_access(const uint8_t *req, uint8_t len, uint8_t *resp)
{
    if (len < 2U) {
        resp[0] = SID_NEGATIVE_RESPONSE; resp[1] = SID_SECURITY_ACCESS; resp[2] = NRC_SUBFUNCTION_NOT_SUPPORTED;
        return 3U;
    }
    uint8_t sub = req[1];

    if (sub == 0x01U) { /* requestSeed */
        s_pending_seed = simple_seed();
        resp[0] = (uint8_t)(SID_SECURITY_ACCESS + 0x40U);
        resp[1] = 0x01U;
        resp[2] = (uint8_t)(s_pending_seed >> 8);
        resp[3] = (uint8_t)(s_pending_seed & 0xFFU);
        return 4U;
    }
    if (sub == 0x02U) { /* sendKey */
        if (len < 4U) {
            resp[0] = SID_NEGATIVE_RESPONSE; resp[1] = SID_SECURITY_ACCESS; resp[2] = NRC_REQUEST_OUT_OF_RANGE;
            return 3U;
        }
        uint16_t key = (uint16_t)((req[2] << 8) | req[3]);
        if (key_matches(s_pending_seed, key)) {
            s_security_unlocked = true;
            resp[0] = (uint8_t)(SID_SECURITY_ACCESS + 0x40U);
            resp[1] = 0x02U;
            return 2U;
        }
        resp[0] = SID_NEGATIVE_RESPONSE; resp[1] = SID_SECURITY_ACCESS; resp[2] = NRC_SECURITY_ACCESS_DENIED;
        return 3U;
    }
    resp[0] = SID_NEGATIVE_RESPONSE; resp[1] = SID_SECURITY_ACCESS; resp[2] = NRC_SUBFUNCTION_NOT_SUPPORTED;
    return 3U;
}

static uint8_t handle_tester_present(const uint8_t *req, uint8_t len, uint8_t *resp)
{
    (void)req; (void)len;
    s_session_idle_ms = 0U;
    resp[0] = (uint8_t)(SID_TESTER_PRESENT + 0x40U);
    resp[1] = 0x00U;
    return 2U;
}

/* ==========================================================================
 * Dispatcher
 * ========================================================================*/
uint8_t Diag_ProcessRequest(const uint8_t *req, uint8_t req_len,
                             uint8_t *resp, uint8_t resp_max_len)
{
    if (req == NULL || resp == NULL || req_len == 0U || resp_max_len < 3U) {
        return 0U;
    }
    uint8_t sid = req[0];

    switch (sid) {
        case SID_DIAGNOSTIC_SESSION_CONTROL: return handle_session_control(req, req_len, resp);
        case SID_ECU_RESET:                  return handle_ecu_reset(req, req_len, resp);
        case SID_CLEAR_DIAGNOSTIC_INFO:      return handle_clear_dtc(req, req_len, resp);
        case SID_READ_DTC_INFORMATION:       return handle_read_dtc(req, req_len, resp);
        case SID_READ_DATA_BY_IDENTIFIER:    return handle_read_data_by_id(req, req_len, resp);
        case SID_SECURITY_ACCESS:            return handle_security_access(req, req_len, resp);
        case SID_TESTER_PRESENT:             return handle_tester_present(req, req_len, resp);
        default:
            resp[0] = SID_NEGATIVE_RESPONSE;
            resp[1] = sid;
            resp[2] = NRC_SERVICE_NOT_SUPPORTED;
            return 3U;
    }
}

/* ==========================================================================
 * Transport adapter: CAN
 *   Single-frame ISO-TP-style layout: byte0 = length (of SID+params),
 *   bytes[1..] = payload. Good enough for payloads <= 7 bytes; longer
 *   requests need real ISO-TP (see README roadmap).
 * ========================================================================*/
void Diag_HandleCanFrame(const CanFrame_t *frame)
{
    if (frame == NULL || frame->id != CAN_DIAG_REQUEST_ID || frame->dlc < 2U) {
        return;
    }
    uint8_t payload_len = frame->data[0];
    if (payload_len == 0U || payload_len > (frame->dlc - 1U) || payload_len > 7U) {
        return;
    }

    uint8_t resp_payload[64];
    uint8_t resp_len = Diag_ProcessRequest(&frame->data[1], payload_len,
                                            resp_payload, sizeof(resp_payload));
    if (resp_len == 0U) {
        return;
    }

    CanFrame_t resp_frame = {0};
    resp_frame.id  = CAN_DIAG_RESPONSE_ID;
    resp_frame.data[0] = resp_len;
    uint8_t copy_len = (resp_len <= 7U) ? resp_len : 7U; /* single-frame cap */
    memcpy(&resp_frame.data[1], resp_payload, copy_len);
    resp_frame.dlc = (uint8_t)(copy_len + 1U);

    (void)Can_Driver_Send(&resp_frame);
}

/* ==========================================================================
 * Transport adapter: UART console
 *   Human-typed commands mapped onto the same service layer so the exact
 *   same diagnostic logic runs whether the request came over CAN or the
 *   bench console.
 * ========================================================================*/
static void print_dtc_report(void)
{
    uint8_t active = 0U;
    for (uint8_t i = 0U; i < s_dtc_table_len; i++) {
        if (s_dtc_table[i].active) {
            active++;
        }
    }
    Uart_Driver_Printf("OK: %u DTC(s)\r\n", active);
    for (uint8_t i = 0U; i < s_dtc_table_len; i++) {
        if (s_dtc_table[i].active) {
            Uart_Driver_Printf("  %s (status: 0x%02X)\r\n", s_dtc_table[i].name, s_dtc_table[i].status);
        }
    }
}

void Diag_HandleConsoleLine(const char *line)
{
    char cmd[32] = {0};
    char arg[32] = {0};
    sscanf(line, "%31s %31s", cmd, arg);

    if (strcmp(cmd, "session") == 0) {
        uint8_t sub = SESSION_DEFAULT;
        if (strcmp(arg, "extended") == 0)      sub = SESSION_EXTENDED;
        else if (strcmp(arg, "programming") == 0) sub = SESSION_PROGRAMMING;
        else if (strcmp(arg, "default") == 0)  sub = SESSION_DEFAULT;
        else { Uart_Driver_SendString("ERR: usage session <default|extended|programming>\r\n"); return; }

        uint8_t req[2] = { SID_DIAGNOSTIC_SESSION_CONTROL, sub };
        uint8_t resp[8];
        uint8_t n = Diag_ProcessRequest(req, 2U, resp, sizeof(resp));
        if (n >= 2U && resp[0] != SID_NEGATIVE_RESPONSE) {
            Uart_Driver_Printf("OK: session=%s\r\n", arg);
        } else {
            Uart_Driver_SendString("ERR: session change rejected\r\n");
        }
    }
    else if (strcmp(cmd, "readdtc") == 0) {
        print_dtc_report();
    }
    else if (strcmp(cmd, "cleardtc") == 0) {
        uint8_t req[1] = { SID_CLEAR_DIAGNOSTIC_INFO };
        uint8_t resp[4];
        (void)Diag_ProcessRequest(req, 1U, resp, sizeof(resp));
        Uart_Driver_SendString("OK: DTCs cleared\r\n");
    }
    else if (strcmp(cmd, "readpid") == 0) {
        uint16_t pid = (uint16_t)strtol(arg, NULL, 0);
        uint8_t req[3] = { SID_READ_DATA_BY_IDENTIFIER, (uint8_t)(pid >> 8), (uint8_t)(pid & 0xFF) };
        uint8_t resp[8];
        uint8_t n = Diag_ProcessRequest(req, 3U, resp, sizeof(resp));
        if (n == 0U || resp[0] == SID_NEGATIVE_RESPONSE) {
            Uart_Driver_SendString("ERR: unknown PID\r\n");
        } else {
            switch (pid) {
                case PID_ENGINE_RPM:
                    Uart_Driver_Printf("OK: RPM = %u rpm\r\n", (uint16_t)((resp[3] << 8) | resp[4])); break;
                case PID_COOLANT_TEMP_C:
                    Uart_Driver_Printf("OK: Coolant = %d C\r\n", (int)resp[3] - 40); break;
                case PID_VEHICLE_SPEED:
                    Uart_Driver_Printf("OK: Speed = %u km/h\r\n", resp[3]); break;
                case PID_BATTERY_VOLTAGE:
                    Uart_Driver_Printf("OK: Battery = %u mV\r\n", (uint16_t)((resp[3] << 8) | resp[4])); break;
                default:
                    Uart_Driver_SendString("OK: (raw bytes)\r\n"); break;
            }
        }
    }
    else if (strcmp(cmd, "reset") == 0) {
        Uart_Driver_SendString("OK: resetting...\r\n");
        HAL_Delay(50U);
        NVIC_SystemReset();
    }
    else if (strcmp(cmd, "help") == 0 || cmd[0] == '\0') {
        Uart_Driver_SendString(
            "Commands: session <default|extended|programming> | readpid <0xNN> |\r\n"
            "          readdtc | cleardtc | reset | help\r\n");
    }
    else {
        Uart_Driver_Printf("ERR: unknown command '%s' (try 'help')\r\n", cmd);
    }
}

/* ==========================================================================
 * Fault simulator — nudges live signals and flips DTCs so the diagnostic
 * flow has something realistic to report without real sensors attached.
 * ========================================================================*/
void Diag_FaultSimulatorTick(void)
{
    /* Coolant creeps toward an over-temp fault, then recovers */
    static int8_t drift = 1;
    s_coolant_temp_c += drift;
    if (s_coolant_temp_c > 118) drift = -1;
    if (s_coolant_temp_c < 82)  drift = 1;
    set_dtc(0U, s_coolant_temp_c > 110);

    /* Battery sags occasionally */
    static uint8_t sag_counter = 0U;
    sag_counter++;
    if (sag_counter % 5U == 0U) {
        s_battery_mv = (s_battery_mv > 11600U) ? (uint16_t)(s_battery_mv - 300U) : 12600U;
    }
    set_dtc(1U, s_battery_mv < 11800U);

    /* RPM wanders a little around idle */
    s_rpm = (uint16_t)(800 + (HAL_GetTick() % 200));
}

/* ==========================================================================
 * Session timeout — extended session drops back to default if no
 * TesterPresent / request is seen within TESTER_PRESENT_TIMEOUT_MS.
 * ========================================================================*/
void Diag_SessionTick(uint32_t elapsed_ms)
{
    if (s_session == SESSION_DEFAULT) {
        return;
    }
    s_session_idle_ms += elapsed_ms;
    if (s_session_idle_ms >= TESTER_PRESENT_TIMEOUT_MS) {
        s_session = SESSION_DEFAULT;
        s_security_unlocked = false;
        s_session_idle_ms = 0U;
    }
}

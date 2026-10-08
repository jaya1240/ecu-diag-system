/**
 * @file can_driver.c
 * @brief bxCAN setup (500 kbps typical HS-CAN timing @ 36MHz APB1 on
 *        STM32F103) plus a small RX queue so the ISR never blocks.
 *
 *        Only a single filter is installed, accepting the diagnostic
 *        request ID (CAN_DIAG_REQUEST_ID). Extend Can_Driver_Init() with
 *        more filter banks if you want to listen on the broadcast bus too.
 */
#include <string.h>
#include "can_driver.h"

#define CAN_RX_QUEUE_LEN 8U

static CAN_HandleTypeDef *s_hcan = NULL;
static CanFrame_t s_rx_queue[CAN_RX_QUEUE_LEN];
static volatile uint8_t s_rx_head = 0U;
static volatile uint8_t s_rx_tail = 0U;

static bool rx_queue_push(const CanFrame_t *frame)
{
    uint8_t next = (uint8_t)((s_rx_head + 1U) % CAN_RX_QUEUE_LEN);
    if (next == s_rx_tail) {
        return false; /* queue full, drop frame */
    }
    s_rx_queue[s_rx_head] = *frame;
    s_rx_head = next;
    return true;
}

static bool rx_queue_pop(CanFrame_t *frame)
{
    if (s_rx_head == s_rx_tail) {
        return false;
    }
    *frame = s_rx_queue[s_rx_tail];
    s_rx_tail = (uint8_t)((s_rx_tail + 1U) % CAN_RX_QUEUE_LEN);
    return true;
}

CanStatus_t Can_Driver_Init(CAN_HandleTypeDef *hcan)
{
    if (hcan == NULL) {
        return CAN_ERR_INIT;
    }
    s_hcan = hcan;
    s_rx_head = 0U;
    s_rx_tail = 0U;

    /* NOTE: hcan->Init (prescaler/BS1/BS2/SJW for 500 kbps) is expected to
     * be configured by CubeMX-generated MX_CAN1_Init() before this call.
     * We only install the diagnostic filter and start the peripheral. */
    CAN_FilterTypeDef filter = {0};
    filter.FilterBank           = 0;
    filter.FilterMode           = CAN_FILTERMODE_IDMASK;
    filter.FilterScale          = CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh         = (CAN_DIAG_REQUEST_ID << 5);
    filter.FilterIdLow          = 0x0000;
    filter.FilterMaskIdHigh     = (0x7FFU << 5); /* exact match, standard ID */
    filter.FilterMaskIdLow      = 0x0000;
    filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    filter.FilterActivation     = ENABLE;
    filter.SlaveStartFilterBank = 14;

    if (HAL_CAN_ConfigFilter(hcan, &filter) != HAL_OK) {
        return CAN_ERR_FILTER;
    }
    if (HAL_CAN_Start(hcan) != HAL_OK) {
        return CAN_ERR_INIT;
    }
    if (HAL_CAN_ActivateNotification(hcan, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) {
        return CAN_ERR_INIT;
    }
    return CAN_OK;
}

CanStatus_t Can_Driver_Send(const CanFrame_t *frame)
{
    if (s_hcan == NULL || frame == NULL) {
        return CAN_ERR_INIT;
    }
    CAN_TxHeaderTypeDef txHeader = {0};
    uint32_t txMailbox;

    txHeader.StdId = frame->id;
    txHeader.IDE   = CAN_ID_STD;
    txHeader.RTR   = CAN_RTR_DATA;
    txHeader.DLC   = frame->dlc;

    if (HAL_CAN_GetTxMailboxesFreeLevel(s_hcan) == 0U) {
        return CAN_ERR_TX_BUSY;
    }
    if (HAL_CAN_AddTxMessage(s_hcan, &txHeader, (uint8_t *)frame->data, &txMailbox) != HAL_OK) {
        return CAN_ERR_TX_BUSY;
    }
    return CAN_OK;
}

bool Can_Driver_Available(void)
{
    return s_rx_head != s_rx_tail;
}

CanStatus_t Can_Driver_Receive(CanFrame_t *frame)
{
    if (frame == NULL) {
        return CAN_ERR_INIT;
    }
    return rx_queue_pop(frame) ? CAN_OK : CAN_ERR_NO_DATA;
}

/**
 * Called from HAL_CAN_RxFifo0MsgPendingCallback(). Kept short: copy the
 * frame into the RX queue and return — actual diagnostic parsing happens
 * in the main loop, never in interrupt context.
 */
void Can_Driver_RxCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef rxHeader;
    CanFrame_t frame = {0};

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rxHeader, frame.data) != HAL_OK) {
        return;
    }
    frame.id  = rxHeader.StdId;
    frame.dlc = (uint8_t)rxHeader.DLC;

    (void)rx_queue_push(&frame);
}

#include "../intest_config.h"
#if (INTER_TEST_MODE == TEST_ULL_HID_DEVICE)


    #include "tl_common.h"
    #include "drivers.h"
    #include "stack/ble/ble.h"

    #include "app_ull_hid.h"
    #include "app_parse_char.h"
    #include "stack/ble/host/gatt/tlk_timer_stack.h"
    #include "app.h"

static int app_input_report_data_timeout(void *arg);

static struct soft_timer inputReportTimer = {
    .next = NULL,
    .cb   = app_input_report_data_timeout,
    .arg  = NULL,
};

static int app_input_report_data_timeout(void *arg)
{
    reportIndex = 0;
    memset(sduData, 0, sizeof(sduData));

    timer_stop(TIMER0);
    app_parse_printf("report timeout\r\n");
    return 0;
}

void app_ullhid_report_data(char *argv[], int argc, void *user_data)
{
    if (app_ull_hid_get_cis_handle() == 0x00 || app_ull_hid_get_acl_handle() == 0x00) {
        app_parse_printf("must ACL connected, CIS connect\r\n");
        return;
    }

    if (argc != 1) {
        app_parse_printf("should input reportData [timeout] \r\n");
        app_parse_printf("timeout unit is ms.\r\n");
        return;
    }

    if (ullhidParam.reportType != ULL_HID_REPORT_TYPE_INPUT) {
        app_parse_printf("No Input Report Type in Hybrid Mode.\r\n");
        return;
    }

    timer_set_cap_tick(TIMER0, sys_clk.pclk * ullhidParam.cisSduInterval);

    inputReportTimer.timer = app_parse_str2n(argv[0]);

    soft_timer_add(&inputReportTimer);

    plic_interrupt_enable(IRQ_TIMER0);
    timer_set_init_tick(TIMER0, 0);
    timer_set_mode(TIMER0, TIMER_MODE_SYSCLK);
    timer_start(TIMER0);
}

static _attribute_ram_code_ void timer0_irq_handler(void)
{
    if (timer_get_irq_status(TMR_STA_TMR0)) {
        timer_clr_irq_status(TMR_STA_TMR0); //clear irq must come after irq process.
        if (ullhidParam.repetition == 0 && ullhidParam.powerSavingCfm == 0) {
            u8 testData[5] = {0x02, ullhidParam.sequenceNumber, ullhidParam.reportID};

            ullhidParam.sequenceNumber++;
            testData[3]      = rand();
            testData[4]      = rand();
            ble_sts_t status = blc_iso_sendData(app_ull_hid_get_cis_handle(), &testData[0], sizeof(testData));

            if (status != BLE_SUCCESS) {
                app_parse_printf("notify report data, handle:0x%x, result is %d\r\n", app_ull_hid_get_cis_handle(), status);
            }
        }
        if (ullhidParam.repetition == 0 && ullhidParam.powerSavingCfm == 1) {
            static u8 resendCnt     = 0;
            static u8 resendBuf[11] = {0x08};
            resendCnt++;
            if (resendCnt == 1) {
                ullhidParam.sequenceNumber++;
                resendBuf[1] = ullhidParam.sequenceNumber;
                resendBuf[2] = ullhidParam.reportID;
                resendBuf[3] = 0x00;
                resendBuf[4] = 0x00;
                resendBuf[5] = 0x04;
                //              timer_stop(TIMER0);
            }
            //          else if(resendCnt == 2)
            //          {
            //              ullhidParam.sequenceNumber ++;
            //              resendBuf[1] = ullhidParam.sequenceNumber;
            //              resendBuf[2] = ullhidParam.reportID;
            //              resendBuf[3] = 0x00;
            //              resendBuf[4] = 0x00;
            //              resendBuf[5] = 0x00;
            //              resendCnt = 0;
            //              timer_stop(TIMER0);
            //          }
            else if (resendCnt == 100) {
                ullhidParam.sequenceNumber++;
                resendBuf[1] = ullhidParam.sequenceNumber;
                resendBuf[5] = 0x00;
                resendCnt    = 0;
            } else if (resendCnt == 200) {
                resendBuf[1] = ullhidParam.sequenceNumber;
                resendBuf[2] = ullhidParam.reportID;
            } else {
                resendBuf[1] = ullhidParam.sequenceNumber;
                resendBuf[2] = ullhidParam.reportID;
            }

            if (ullhidParam.recvAckSeqNum != ullhidParam.sequenceNumber) {
                //              app_parse_printf("send seq is %d %d %d.\r\n", resendBuf[1]);
                ble_sts_t status = blc_iso_sendData(app_ull_hid_get_cis_handle(), &resendBuf[0], sizeof(resendBuf));

                if (status != BLE_SUCCESS) {
                    app_parse_printf("notify report data, handle:0x%x, result is %d\r\n", app_ull_hid_get_cis_handle(), status);
                }
            }
        }

        if (ullhidParam.repetition == 1 && ullhidParam.powerSavingCfm == 0) {
            ullhid_sdu_data_t *txSdu = &sduData[0];
            if (reportIndex < ullhidParam.retryCount) {
                txSdu = &sduData[reportIndex];
                reportIndex++;
            } else {
                memcpy(&sduData[0], &sduData[1], sizeof(ullhid_sdu_data_t) * (ullhidParam.retryCount - 1));
                txSdu = &sduData[ullhidParam.retryCount - 1];
            }
            txSdu->length         = 2;
            txSdu->sequenceNumber = ullhidParam.sequenceNumber;
            txSdu->reportId       = ullhidParam.reportID;
            txSdu->data[0]        = rand();
            txSdu->data[1]        = rand();
            ullhidParam.sequenceNumber++;
            ble_sts_t status = blc_iso_sendData(app_ull_hid_get_cis_handle(), (u8 *)&sduData[0], sizeof(ullhid_sdu_data_t) * reportIndex);

            if (status != BLE_SUCCESS) {
                app_parse_printf("notify ISO mouse data, handle:0x%x, result is %d\r\n", app_ull_hid_get_cis_handle(), status);
            }
        }
    }
}

PLIC_ISR_REGISTER(timer0_irq_handler, IRQ_TIMER0)

#endif

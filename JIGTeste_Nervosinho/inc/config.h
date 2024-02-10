#define PROTOCOL_VERSION    0
#define BUFFER_SIZE         100

#define MSG_TYPE_GET        0
#define MSG_TYPE_SET        1
#define MSG_TYPE_RESP       2
#define MSG_TYPE_ERROR      3
#define MSG_TYPE_TIMEOUT    4

#define MSG_CMD_GET_WATCHDOG            0
#define MSG_CMD_POE_RESET               1
#define MSG_CMD_SIM_TYPE                2
#define MSG_CMD_SIM_ICCID               3
#define MSG_CMD_SIM_CARRIER             4
#define MSG_CMD_MODEM_SIGNAL            5 
#define MSG_CMD_MODEM_RESET             6   
#define MSG_CMD_SERIAL_NUMBER           7
#define MSG_CMD_BATCH_NUMBER            8
#define MSG_CMD_ANATEL_NUMBER           9
#define MSG_CMD_OS_VERSION              10
#define MSG_CMD_FIRMWARE_VERSION        11
#define MSG_CMD_BUZZER_ENABLE           12
#define MSG_CMD_BUZZER_DISABLE          13
#define MSG_CMD_IS_UPGRADING            14
#define MSG_CMD_PCB_REV                 15
#define MSG_CMD_TEST_START              16
#define MSG_CMD_TEST_MODEM_RESULT       17
#define MSG_CMD_TEST_SWITCH_RESULT      18
#define MSG_CMD_TEST_ETHERNET_RESULT    19
#define MSG_CMD_BMS_CONNECTION          20
#define MSG_CMD_BMS_STATUS              21
#define MSG_CMD_SOCKETXP_STATUS         22
#define MSG_CMD_TAMPER_EVENT            23
#define MSG_CMD_TEST_EEPROM_RESULT      24
#define MSG_CMD_TEST_DISPLAY_RESULT     25
#define MSG_CMD_EEPROM_DISABLE_WRITE_PROTECTION      26
#define MSG_CMD_EEPROM_ENABLE_WRITE_PROTECTION       27

#define CALLBACK_FUNCTIONS_LEN  30
#define MESSAGES_LEN            21

#define MSG_STATUS_UNUSED           0
#define MSG_STATUS_WAITING_TO_SEND  1
#define MSG_STATUS_WAITING_RESPONSE 2
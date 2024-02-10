#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include "../inc/config.h"

FILE *logfile = NULL;

char *serialPort = "/dev/ttyS1";
char *usbPath = "/dev/ttyUSB2";
int flagRejectedTest = 0;

struct Message
{
    int firmwareVersion;
    int messageType;
    int messageCommand;
    int messageID;
    int messageDataLen;
    char messageData[100];
};

int flash_STM();
int open_serial(const char *);
int open_modem(const char *);
int tests_STM(int, char *, struct Message *, struct Message *);
int parse_message(char *, struct Message *);
int read_incoming_message(int, struct Message *);
int parse_message(char *, struct Message *);
int DecodeCharlesMessage(const char* message, struct Message *decodedMessage);
void setParameters(struct Message *MessagePtr, int version, int type, int command, int id, char *str);
int test_eeprom(int, char *, struct Message *, struct Message *);
int test_ports();
int test_eth();
void construct_message(struct Message *, char *);
void sendAndReceiveMessage( struct Message *sentMessage, struct Message *receivedMessage, char *messageBuffer, int serialport, int firmware_version, int messageType, int messageCommand, int messageID, char *data );
int write_eeprom(int serialport, char *message, struct Message *sentMessage, struct Message *receivedMessage);



int main(void)
{
    flash_STM();
    system("sleep 30");

    logfile = fopen("/root/logHardwareTest.txt", "w");

    struct Message receivedMessage[6];
    struct Message sentMessage[6];

    fprintf(logfile, "\n\t------- LOG ERRORS -------\n\n");

    int usbModem = open_modem(usbPath);
    int serialSTM = open_serial(serialPort);

    char message[256];

    if (tests_STM(serialSTM, message, sentMessage, receivedMessage))
    {
        fprintf(logfile,"\t- STM32 not functioning\n");
        flagRejectedTest = 1;
        return 1;
    }

    system("sleep 2");

    if (usbModem)
    {
        sendAndReceiveMessage(  sentMessage, receivedMessage, message,
                                serialSTM, PROTOCOL_VERSION, MSG_TYPE_SET,
                                MSG_CMD_TEST_MODEM_RESULT, 0, "False"       );
        fprintf(logfile,"\t- MODEM FAILED\n");
        flagRejectedTest = 1;

    }
    else
    {
        sendAndReceiveMessage(  sentMessage, receivedMessage, message,
                                serialSTM, PROTOCOL_VERSION, MSG_TYPE_SET,
                                MSG_CMD_TEST_MODEM_RESULT, 0, "True"       );
    }

    system("sleep 2");

    if (test_eth())
    {
        sendAndReceiveMessage(  sentMessage, receivedMessage, message,
                                serialSTM, PROTOCOL_VERSION, MSG_TYPE_SET,
                                MSG_CMD_TEST_ETHERNET_RESULT, 0, "False"       );
        fprintf(logfile,"\t- ETH FAILED\n");
        flagRejectedTest = 1;

    }
    else
    {
        sendAndReceiveMessage(  sentMessage, receivedMessage, message,
                                serialSTM, PROTOCOL_VERSION, MSG_TYPE_SET,
                                MSG_CMD_TEST_ETHERNET_RESULT, 0, "True"       );
    }

    system("sleep 2");

    if (test_ports())
    {
        sendAndReceiveMessage(  sentMessage, receivedMessage, message,
                                serialSTM, PROTOCOL_VERSION, MSG_TYPE_SET,
                                MSG_CMD_TEST_SWITCH_RESULT, 0, "False"               );
        fprintf(logfile,"\t- PORTs FAILED\n");
        flagRejectedTest = 1;
    }
    else
    {
        sendAndReceiveMessage(  sentMessage, receivedMessage, message,
                                serialSTM, PROTOCOL_VERSION, MSG_TYPE_SET,
                                MSG_CMD_TEST_SWITCH_RESULT, 0, "True"       );
    }

    system("sleep 1");

    write_eeprom(serialSTM, message, sentMessage, receivedMessage);

    if(flagRejectedTest){
        fprintf(logfile,"\n\t******** REJECTED ********\n");
    }
    else{
        fprintf(logfile,"\n\t******** APPROVED ********\n");
    }

    close(serialSTM);
    return 0;
}

void construct_message(struct Message *MessagePtr, char *message)
{
    char formatted_message[256];
    snprintf(formatted_message, sizeof(formatted_message),
             "[version:%d;type:%d;command:%d;message_id:%d;data_len:%d;data:%s]",
             MessagePtr->firmwareVersion, MessagePtr->messageType, MessagePtr->messageCommand, MessagePtr->messageID, MessagePtr->messageDataLen, MessagePtr->messageData);
    strcpy(message, formatted_message);
}

void setParameters(struct Message *MessagePtr, int version, int type, int command, int id, char *str)
{
    MessagePtr->firmwareVersion = version;
    MessagePtr->messageType = type;
    MessagePtr->messageCommand = command;
    MessagePtr->messageID = id;
    MessagePtr->messageDataLen = strlen(str)+1;
    strcpy(MessagePtr->messageData, str);
}

int flash_STM()
{
    system("echo \"default-on\" > /sys/class/leds/BOOT_STM32/trigger && sleep 1 && echo \"none\" > /sys/class/leds/RESET_STM32/trigger && sleep 1 && echo \"default-on\" > /sys/class/leds/RESET_STM32/trigger && sleep 1");
    for (int number_of_retries = 0; number_of_retries < 3; number_of_retries++)
    {
        if (system("stm32flash -v -w /opt/gabriel/bin/firmware_stm32.bin -b 115200 /dev/ttyS1"))
        {
        }
        else
        {
            break;
        }
        if(number_of_retries == 2){
            printf("Couldn't flash STM\n");
            return 1;
        }
    }
    system("echo \"none\" > /sys/class/leds/BOOT_STM32/trigger && sleep 1 && echo \"none\" > /sys/class/leds/RESET_STM32/trigger && sleep 1 && echo \"default-on\" > /sys/class/leds/RESET_STM32/trigger");
    return 0;
}

int tests_STM(int serialSTM, char *message, struct Message *sentMessage, struct Message *receivedMessage)
{
    sendAndReceiveMessage(  sentMessage, receivedMessage, message,
                            serialSTM, PROTOCOL_VERSION, MSG_TYPE_SET,
                            MSG_CMD_TEST_START, 0, ""/*"True"*/);

    if (serialSTM == -1)
    {
        printf("Could not open Serial port\n");
        return -1;
    }
    if (receivedMessage->messageType != MSG_TYPE_RESP)
    {
        fprintf(logfile, "STM32 ERROR:\n");
        fprintf(logfile, "\t- Not expected response from STM32\n");
        return -1;
    }

    system("sleep 1");

    read_incoming_message(serialSTM, receivedMessage);

    if (strcmp(receivedMessage->messageData, "PASSED"))
    {
        fprintf(logfile, "DISPLAY ERROR:\n");
        fprintf(logfile, "\t- Display not working\n");
    }

    read_incoming_message(serialSTM, receivedMessage);
    if (strcmp(receivedMessage->messageData, "PASSED"))
    {
        fprintf(logfile, "EEPROM ERROR\n");
        fprintf(logfile, "\t- EEPROM not working\n");
    }

    return 0;
}

int write_eeprom(int serialport, char *message, struct Message *sentMessage, struct Message *receivedMessage)
{
    char batch[20];

    FILE *file = fopen("/opt/gabriel/bin/lote.txt", "r");
    if (file)
    {
        char line[255];
        while (fgets(line, sizeof(line), file))
        {
            sscanf(line, "%s", batch);
        }
    }
    else
    {
        perror("Failed to open lote.txt");
        strcpy(batch, "20230924");
    }

    sendAndReceiveMessage(  sentMessage, receivedMessage, message,
                            serialport, PROTOCOL_VERSION, MSG_TYPE_SET,
                            MSG_CMD_EEPROM_DISABLE_WRITE_PROTECTION, 0, "True" );

    system("sleep 1");

    sendAndReceiveMessage(  sentMessage, receivedMessage, message,
                            serialport, PROTOCOL_VERSION, MSG_TYPE_SET,
                            MSG_CMD_BATCH_NUMBER, 0, batch                     );

    system("sleep 1");

    sendAndReceiveMessage(  sentMessage, receivedMessage, message,
                            serialport, PROTOCOL_VERSION, MSG_TYPE_SET,
                            MSG_CMD_EEPROM_ENABLE_WRITE_PROTECTION, 0, "True" );
                    
    system("sleep 3");
    return 0;
}

int test_eth()
{
    bool flagError = false;

    if(system("ping -c 4 baidu.com")){
        if(!flagError){
            fprintf(logfile,"ETH ERRORS:\n");
            flagError = true;
        }
        fprintf(logfile, "\t- Couldn't ping DNS server (either blocked or no available network)\n");
        return 1;
    }

    if(system("wget http://raw.githubusercontent.com/leonancfr/lote-nervosinho/main/lote.txt -O /opt/gabriel/bin/lote.txt")){
        if(!flagError){
            fprintf(logfile,"ETH ERRORS:\n");
            flagError = true;
        }
        fprintf(logfile, "\t- Couldn't download batch information text file\n");
    }

    return 0;
}

int test_ports()
{
    int ipsGiven = 0;
    bool flagError = false;

    FILE *file = fopen("/tmp/dhcp.leases", "r");
    if (!file)
    {
        perror("Failed to open dhcp.leases");
        fprintf(logfile, "\t- Failed to open dhcp.leases file (check openWrt image)");
        return 1;
    }

    char line[256];
    while (fgets(line, sizeof(line), file))
    {
        ipsGiven++;
        char lease_expiry[50], mac_address[50], ip_address[50], hostname[50], client_id[50];

        if (sscanf(line, "%s %s %s %s %s", lease_expiry, mac_address, ip_address, hostname, client_id) == 5)
        {
            // Construct the ping command
            char command[128];
            snprintf(command, sizeof(command), "ping -c 4 %s", ip_address);
            printf("Executing: %s\n", command);

            // Run the ping command
            if (system(command) > 0)
            {
                if(!flagError){
                    fprintf(logfile,"SWITCH ERRORS:\n");
                    flagError = true;
                }
                fprintf(logfile, "\t- Couldn't ping all devices (check connections and power)\n");
                return 1;
            };
        }
    }

    if (ipsGiven != 3)
    {
        if(!flagError){
            fprintf(logfile,"SWITCH ERRORS:\n");
            flagError = true;
        }
        fprintf(logfile, "\t- Expected 3 devices\n");
        return 1;
    };
    fclose(file);
    return 0;
}

int read_incoming_message(int serialport, struct Message *receivedMessage)
{
    if (tcflush(serialport, TCIFLUSH) == 1)
    {
        perror("Failed to flush the input buffer");
        close(serialport);
        return 1;
    }

    char buf[256];
    int bytes_read = read(serialport, buf, sizeof(buf));

    if (bytes_read > 0)
    {
        buf[bytes_read] = '\0';
        parse_message(buf, receivedMessage);
        printf("%s\n", buf);
    }
    else
    {
        return 1;
    }

    return 0;
}

int parse_message(char *_buffer, struct Message *receivedMessage)
{
    int values[5];
    char dataBuffer[20];
    int count = sscanf(_buffer, "[version:%d;type:%d;command:%d;message_id:%d;data_len:%d;data:%20[^]]",
                       &values[0], &values[1], &values[2], &values[3], &values[4], dataBuffer);

    if (count >= 5)
    {
        if (values[0] >= 0 && values[0] <= 255 && values[1] >= 0 && values[1] <= 255 &&
            values[2] >= 0 && values[2] <= 255 && values[3] >= 0 && values[3] <= 65535 &&
            values[4] >= 0 && values[4] <= 255)
        {
            receivedMessage->firmwareVersion = values[0];
            receivedMessage->messageType = values[1];
            receivedMessage->messageCommand = values[2];
            receivedMessage->messageID = values[3];
            receivedMessage->messageDataLen = values[4];
            strncpy(receivedMessage->messageData, dataBuffer, sizeof(receivedMessage->messageData));
            receivedMessage->messageData[sizeof(receivedMessage->messageData) - 1] = '\0';
            return 1;
        }
    }
    return 0;
}

void sendAndReceiveMessage( struct Message *sentMessage, struct Message *receivedMessage, char *messageBuffer,
                            int serialport, int firmware_version, int messageType, 
                            int messageCommand, int messageID, char *data ){

    setParameters(sentMessage, firmware_version, messageType, messageCommand, messageID, data);
    construct_message(sentMessage, messageBuffer);

    write(serialport, messageBuffer, strlen(messageBuffer));

    read_incoming_message(serialport, receivedMessage);
}

int open_modem(const char *usbPath)
{
    int fd;
    struct termios options;
    char buffer[255];
    bool flagError = false;

    // Open the modem device file
    fd = open(usbPath, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1)
    {
        perror("open_port: Unable to open /dev/ttyUSB2");
        return 1;
    }

    // Get the current options for the port
    tcgetattr(fd, &options);

    // Set the baud rates to 9600
    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);

    options.c_cc[VTIME] = 50;
    options.c_cc[VMIN] = 0; // Minimum number of characters to read

    // 8N1 Mode
    options.c_cflag &= ~PARENB;          // No parity
    options.c_cflag &= ~CSTOPB;          // 1 stop bit
    options.c_cflag &= ~CSIZE;           // Clears the Mask
    options.c_cflag |= CS8;              // Set the data bits = 8
    options.c_cflag |= (CLOCAL | CREAD); // Enable receiver and set local mode

    // No flow control
    options.c_cflag &= ~CRTSCTS;

    // Set the new options for the port
    tcsetattr(fd, TCSANOW, &options);

    tcflush(fd, TCIOFLUSH);

    system("sleep 1");

    write(fd, "AT\r\n", 4);

    system("sleep 1");

    // tcflush(fd, TCIFLUSH);
    //  Read the response from the modem
    int n = 0;
    for (int i = 0; i < 4; i++)
    {
        n = read(fd, buffer, sizeof(buffer) - 1);
    }
    if (n < 0)
    {
        perror("Read failed");
        return 1;
    }
    buffer[n] = '\0';
    printf("Received: %s", buffer);

    system("sleep 3");

    if (!strstr(buffer, "OK"))
    {
        if(!flagError){
            fprintf(logfile,"MODEM ERRORS:\n");
            flagError = true;
        }
        fprintf(logfile, "\t- Modem response not expected\n");
        return 1;
    }

    tcflush(fd, TCIOFLUSH);

    write(fd, "AT+CPIN?\r\n", 10);

    system("sleep 1");

    n = 0;
    for (int i = 0; i < 4; i++)
    {
        n = read(fd, buffer, sizeof(buffer) - 1);
    }
    if (n < 0)
    {
        perror("Read failed");
        return 1;
    }
    buffer[n] = '\0';
    printf("Received: %s", buffer);

    close(fd);

    if (strstr(buffer, "+CPIN: READY"))
    {
        return 0;
    }
    else
    {
        if (!flagError){
            fprintf(logfile,"MODEM ERRORS:\n");
            flagError = true;
        }
        fprintf(logfile, "\t- SIM Card not detected\n");
        return 1;
    }
}

int open_serial(const char *device)
{
    int fd = open(device, O_RDWR | O_NOCTTY);

    if (fd == 1)
    {
        perror("open_serial: Unable to open device\n");
        return 1;
    }

    struct termios options;
    tcgetattr(fd, &options);

    // set timeout

    options.c_cc[VTIME] = 5;
    options.c_cc[VMIN] = 50; // Minimum number of characters to read

    // Set baud rate
    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);

    // 8N1 Mode
    options.c_cflag &= ~PARENB; // No parity
    options.c_cflag &= ~CSTOPB; // 1 stop bit
    options.c_cflag &= ~CSIZE;  // Clears the Mask
    options.c_cflag |= CS8;     // Set the data bits = 8

    //options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    // No flow control
    options.c_cflag &= ~CRTSCTS;

    // Apply settings
    if (tcsetattr(fd, TCSANOW, &options) < 0)
    {
        perror("open_serial: Couldn't set term attributes\n");
        return 1;
    }

    return fd;
}

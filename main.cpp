#ifdef __ARM_FP
#undef  __ARM_FP
#endif

#include "mbed.h"
#include "MotionControl.h"
#include "Vector.hpp"
#include "StdMotor.h"

#include <cstring>
#include <cstdio>
#include <cstdint>

const int period = 50; // unit: us

/*
Old definition of the motors
StdMotor FL(p21, p19, p18, period);
StdMotor FR(p22, p17, p16, period);
StdMotor BL(p23, p15, p14, period);
StdMotor BR(p24, p13, p12, period);
*/

StdMotor BR(p21, p19, p18, period);
StdMotor BL(p22, p17, p16, period);
StdMotor FR(p23, p15, p14, period);
StdMotor FL(p24, p13, p12, period);
MotionControl carMotion(FL, FR, BL, BR);

// Commands read from the bluetooth UART
struct CmdInfo{
    char text[32];
    int len;
    CmdInfo() {memset(text, 0, sizeof(text)); len = 0;}
};

// Activated status for variable flag
const uint32_t BT_READABLE = (1 << 0);
const uint32_t RPI_READABLE = (1 << 1);

BufferedSerial pc(USBTX, USBRX);
BufferedSerial bt(p9, p10);
BufferedSerial rpi(p28, p27);

EventFlags flag;
// The thread used to read the command from the bluetooth UART
Thread inputThread;
// Mailbox is used to transfer messages between threads,
// ensuring the safety of threads.
Mail<CmdInfo, 16> cmdMail;

void btInterrupt();
void rpiInterrupt();
void inputProcess();
void cmdProcess(const char*, int len);


// DigitalOut hb_led(LED1); 
// Timer hb_timer;
// unsigned long hb_count = 0;
// void send_heartbeat() {
//     if (hb_timer.read_ms() >= 1000) {
//         hb_timer.reset();
//         char buffer[32];
//         int len = snprintf(buffer, sizeof(buffer), "HB:%lu\n", hb_count++);
//         char msg[32];
//         if(rpi.writable()) bt.write(buffer, len);
//         else bt.write("No Con.\n", 6);
//         if (rpi.writable()) {
//             rpi.write(buffer, len);
//             hb_led = !hb_led; // 翻转 LED，表示单片机还活着
//         }
//     }
// }

int main() {
    // hb_timer.start();
    // Register an interrupt without blocking for the bluetooth UART
    bt.sigio(callback(btInterrupt));
    bt.set_blocking(0);
    bt.set_baud(9600);

    rpi.sigio(callback(rpiInterrupt));
    rpi.set_blocking(1);
    rpi.set_baud(115200);

    inputThread.start(callback(inputProcess));

    printf("Successfully started.\n");
    while (true) {
        // send_heartbeat();
        osEvent evt = cmdMail.get(10); 
        // If there is messages in the mailbox, take out it and parse the texts.
        if(evt.status == osEventMail){
            CmdInfo* msg = (CmdInfo*)evt.value.p;
            cmdProcess(msg->text, msg->len);
            cmdMail.free(msg);
            // When the processing is finished, release the memory of the mail
        }
    }
}

// The interrupt will be triggered,
// when there is a message on the bluetooth UART.
void btInterrupt() {
    // Set the flag to be activated,
    // which will let inputProcess to read the message.
    flag.set(BT_READABLE);
}

void rpiInterrupt() {
    flag.set(RPI_READABLE);
}

void inputProcess() {
    const int BUF_SIZE = 32;

    char btBuf[BUF_SIZE], rpiBuf[BUF_SIZE], ch;
    int btPtr = 0, rpiPtr = 0;

    auto charProcess = [&] (char buffer[], int &ptr) {
        // Input processing ends with the string stored in the mailbox
        // when reaching the end of the string
        // or the buffer overflows.
        // Otherwise, just put the character into the buffer.
        if(ch == '\n' || ch == '\r' || ch == '\0' || ptr == 31) {
            buffer[ptr] = '\0';
            CmdInfo *msg = cmdMail.alloc();
            if(msg){
                std::memcpy(msg->text, buffer, ptr + 1);
                msg->len = ptr;
                cmdMail.put(msg);
            }
            memset(buffer, 0, BUF_SIZE);
            ptr = 0;
            
        }
        else buffer[ptr++] = ch;
    };

    while(true) {
        flag.wait_any(BT_READABLE | RPI_READABLE);

        while(bt.readable()) {
            bt.read(&ch, 1);
            charProcess(btBuf, btPtr);
        }

        while(rpi.readable()) {
            rpi.read(&ch, 1);
            charProcess(rpiBuf, rpiPtr);
        }
    }
}

void cmdProcess(const char* text, int len){
    printf("Len: %d, %s\r\n", len, text);
    bt.write(text, strlen(text));
    fflush(stdout);
    // The detailed instructions on the commands can be referred in the mannual.
    // '#': Commands with s single alphabet
    if(text[0] == '#'){
        char command = text[2];
        if(isupper(command)){
            Direction tmp(0, 0, 0, 0);
            auto update = [&]{
                carMotion.addDir(tmp);
                carMotion.updateMotion();
            };
            switch(command){
                case 'U': tmp.f = 1; update(); break;
                case 'D': tmp.b = 1; update(); break;
                case 'L': tmp.l = 1; update(); break;
                case 'R': tmp.r = 1; update(); break;
                case 'B': carMotion.setRotation(-1); break;
                case 'E': carMotion.setRotation(1); break;
                case 'S': carMotion.stop(); break;
            }
        }
        if(islower(command)){
            Direction tmp(1, 1, 1, 1);
            auto update = [&]{
                carMotion.delDir(tmp);
                carMotion.updateMotion();
            };
            switch(command){
                case 'u': tmp.f = 0; update(); break;
                case 'd': tmp.b = 0; update(); break;
                case 'l': tmp.l = 0; update(); break;
                case 'r': tmp.r = 0; update(); break;
                case 'b': 
                case 'e': carMotion.setRotation(0); break;
                case 's': carMotion.stop(); break;
            }
        }
    }
    // ':': Commands with a group of phrases
    if(text[0] == ':') {
        char command[16];
        sscanf(text + 2, "%15s", command);
        if(strcmp(command, "SPD") == 0) {
            int speed;
            sscanf(text + 6, "%d", &speed);
            carMotion.setBaseSpeed(speed);
        }
        if(strcmp(command, "V") == 0) {
            double vx, vy;
            sscanf(text + 4, "%lf%lf", &vx, &vy);
            carMotion.updateMotion(Vector<double>(vx, vy));
        }
        if(strcmp(command, "SM") == 0) {
            char mode[8];
            sscanf(text + 5, "%7s", mode);
            if(strcmp(mode, "JS") == 0) carMotion.changeMode(JOYSTICK);
            if(strcmp(mode, "BT") == 0) carMotion.changeMode(BUTTON);
        }

        bool isRpi = ((strcmp(command, "SVO") == 0) ||
            (strcmp(command, "RGZ") == 0) ||
            (strcmp(command, "TRK") == 0));
        
        if(isRpi) {
            char msg[32];
            sprintf(msg, "Sent \"%s\"\n", text);
            bt.write(msg, strlen(msg));
            memset(msg, 0, sizeof(msg));
            sprintf(msg, "%s\n", text);
            rpi.write(msg, strlen(msg));
        }
    }
}
#ifdef __ARM_FP
#undef  __ARM_FP
#endif

#include "mbed.h"
#include "MotionControl.h"
#include "Vector.hpp"
#include "StdMotor.h"
// #include "Servo.h"

#include <cstring>
#include <cstdio>
#include <cstdint>

const int period = 50; // unit: us

/* Old definition of the motors
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
const uint32_t BT_READABLE = 0x3f;
BufferedSerial pc(USBTX, USBRX);
BufferedSerial bt(p9, p10);
EventFlags flag;
// The thread used to read the command from the bluetooth UART
Thread inputThread;
// Mailbox is used to transfer messages between threads,
// ensuring the safety of threads.
Mail<CmdInfo, 16> cmdMail;

void rxInterupt();
void inputProcess();
void cmdProcess(const char*, int len);

int main() {
    // Register an interrupt without blocking for the bluetooth UART
    bt.sigio(callback(rxInterupt));
    bt.set_blocking(0);
    bt.set_baud(9600);
    inputThread.start(callback(inputProcess));

    printf("Successfully started.\n");
    while (true) {
        osEvent evt = cmdMail.get(0); 
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
void rxInterupt() {
    // Set the flag to be activated,
    // which will let inputProcess to read the message.
    flag.set(BT_READABLE);
}

void inputProcess() {
    char buffer[32];
    int ptr = 0;
    while(true) {
        flag.wait_any(BT_READABLE);
        while(bt.readable()) {
            char ch;
            bt.read(&ch, 1);
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
                memset(buffer, 0, sizeof(buffer));
                ptr = 0;
                
            }
            else buffer[ptr++] = ch;
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
        char command[8];
        sscanf(text + 2, "%s", command);
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
            sscanf(text + 5, "%s", mode);
            if(strcmp(mode, "JS") == 0) carMotion.changeMode(JOYSTICK);
            if(strcmp(mode, "BT") == 0) carMotion.changeMode(BUTTON);
        }
        if(strcmp(command, "SVO") == 0) {
            char dir[4];
            int level;
            sscanf(text + 6, "%s%d", dir, &level);
            // if(strcmp(dir, "UD")) servoUD.setAngle(level * 10);
            // if(strcmp(dir, "LR")) servoLR.setAngle(level * 10);
        }
    }
}